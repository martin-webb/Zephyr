#include "gameboy.h"

#include "cartridge.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VRAM_SIZE_BYTES (8 * 1024)
#define WRAM_SIZE_BYTES (8 * 1024)
#define OAM_SIZE_BYTES 160
#define HRAM_SIZE_BYTES 127

void gbInitialise(GameBoy* gameBoy, GameBoyType gameBoyType, uint8_t* cartridgeData, Pixel* frameBuffer, const char* romFilename)
{
  CGBMode cgbMode;
  uint8_t cgbFlag = cartridgeGetCGBMode(cartridgeData);
  if (gameBoyType == CGB && (cgbFlag == 0x80 || cgbFlag == 0xC0)) {
    cgbMode = COLOUR;
  } else {
    cgbMode = MONOCHROME;
  }

  gameBoy->vram = (uint8_t*)malloc(VRAM_SIZE_BYTES * ((cgbMode == COLOUR) ? 2 : 1) * sizeof(uint8_t));
  gameBoy->wram = (uint8_t*)malloc(WRAM_SIZE_BYTES * ((cgbMode == COLOUR) ? 4 : 1) * sizeof(uint8_t));
  gameBoy->oam  = (uint8_t*)malloc(OAM_SIZE_BYTES * sizeof(uint8_t));
  gameBoy->hram = (uint8_t*)malloc(HRAM_SIZE_BYTES * sizeof(uint8_t));

  assert(gameBoy->vram);
  assert(gameBoy->wram);
  assert(gameBoy->oam);
  assert(gameBoy->hram);

  memset(gameBoy->vram, 0, VRAM_SIZE_BYTES);
  memset(gameBoy->wram, 0, WRAM_SIZE_BYTES);
  memset(gameBoy->oam,  0, OAM_SIZE_BYTES);
  memset(gameBoy->hram, 0, HRAM_SIZE_BYTES);

  initLCDController(&gameBoy->lcdController, gameBoy->vram, gameBoy->oam, frameBuffer);
  initJoypadController(&gameBoy->joypadController);
  initTimerController(&gameBoy->timerController);
  initInterruptController(&gameBoy->interruptController);
  initSpeedController(&gameBoy->speedController);

  uint8_t cartridgeType = cartridgeGetType(cartridgeData);

  uint8_t ramSize = cartridgeData[RAM_SIZE_ADDRESS];
  uint32_t externalRAMSizeBytes = RAMSizeInBytes(ramSize);

  gameBoy->memoryController = InitMemoryController(
    cartridgeType,
    gameBoy->vram,
    gameBoy->wram,
    gameBoy->oam,
    gameBoy->hram,
    cartridgeData,
    cgbMode,
    &gameBoy->joypadController,
    &gameBoy->lcdController,
    &gameBoy->timerController,
    &gameBoy->interruptController,
    &gameBoy->speedController,
    externalRAMSizeBytes,
    romFilename
  );

  gameBoy->gameBoyType = gameBoyType;
  gameBoy->cgbMode = cgbMode;
  gameBoy->speedMode = NORMAL;

  cpuReset(&gameBoy->cpu, &gameBoy->memoryController, gameBoy->gameBoyType);
}

void gbFinalise(GameBoy* gameBoy)
{
  free(gameBoy->vram);
  free(gameBoy->wram);
  free(gameBoy->oam);
  free(gameBoy->hram);
}

GameBoyType gbGetGameType(uint8_t* cartridgeData)
{
  if (cartridgeData[SGB_FLAG_ADDRESS] == 0x03) {
    return SGB;
  } else {
    if (cartridgeData[CGB_FLAG_ADDRESS] == 0x80) {
      return CGB;
    } else {
      return GB;
    }
  }
}

void gbRunNFrames(GameBoy* gameBoy, const int frames)
{
  const int targetCycles = frames * FULL_FRAME_CLOCK_CYCLES * ((gameBoy->speedMode == DOUBLE) ? 2 : 1);

  CPU* cpu = &gameBoy->cpu;
  LCDController* lcdController = &gameBoy->lcdController;
  InterruptController* interruptController = &gameBoy->interruptController;
  TimerController* timerController = &gameBoy->timerController;
  MemoryController* memoryController = &gameBoy->memoryController;
  SpeedController* speedController = &gameBoy->speedController;
  GameBoyType gameBoyType = gameBoy->gameBoyType;

  // Execute instructions until we have reached the minimum required number of cycles that would have occurred
  uint32_t totalCyclesExecuted = 0;
  while (totalCyclesExecuted < targetCycles) {
    uint8_t cyclesExecuted = cpuRunSingleOp(cpu, memoryController);

    // Detect speed switch
    gameBoy->speedMode = ((speedController->key1 & (1 << 7)) ? DOUBLE : NORMAL);

    totalCyclesExecuted += cyclesExecuted;
    cpuUpdateIME(cpu);
    cartridgeUpdate(memoryController, cyclesExecuted, gameBoy->speedMode);
    dmaUpdate(memoryController, cyclesExecuted);
    timerUpdateDivider(timerController, cyclesExecuted);
    timerUpdateTimer(timerController, interruptController, gameBoy->speedMode, cyclesExecuted);
    lcdUpdate(lcdController, interruptController, gameBoy->speedMode, cyclesExecuted);
    cpuHandleInterrupts(cpu, interruptController, memoryController, gameBoyType);
  }
}