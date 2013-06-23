#include "gameboy.h"

#include "cartridge.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE_BYTES (32 * 1024)

void gbInitialise(GameBoy* gameBoy, GameBoyType gameBoyType, uint8_t* cartridgeData, uint8_t* frameBuffer, const char* romFilename)
{
  gameBoy->memory = (uint8_t*)malloc(MEMORY_SIZE_BYTES * sizeof(uint8_t));
  assert(gameBoy->memory);

  memset(gameBoy->memory, 0, MEMORY_SIZE_BYTES);

  initLCDController(&gameBoy->lcdController, &(gameBoy->memory[0]), &(gameBoy->memory[0xFE00 - CARTRIDGE_SIZE]), &(frameBuffer[0]));
  initJoypadController(&gameBoy->joypadController);
  initTimerController(&gameBoy->timerController);
  initInterruptController(&gameBoy->interruptController);

  uint8_t cartridgeType = cartridgeGetType(cartridgeData);

  uint8_t ramSize = cartridgeData[RAM_SIZE_ADDRESS];
  uint32_t externalRAMSizeBytes = RAMSizeInBytes(ramSize);

  gameBoy->memoryController = InitMemoryController(
    cartridgeType,
    gameBoy->memory,
    cartridgeData,
    &gameBoy->joypadController,
    &gameBoy->lcdController,
    &gameBoy->timerController,
    &gameBoy->interruptController,
    externalRAMSizeBytes,
    romFilename
  );

  gameBoy->gameBoyType = gameBoyType;
  gameBoy->speedMode = NORMAL;

  cpuReset(&gameBoy->cpu, &gameBoy->memoryController, gameBoy->gameBoyType);
}

void gbFinalise(GameBoy* gameBoy)
{
  free(gameBoy->memory);
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
  GameBoyType gameBoyType = gameBoy->gameBoyType;
  SpeedMode speedMode = gameBoy->speedMode;

  // Execute instructions until we have reached the minimum required number of cycles that would have occurred
  uint32_t totalCyclesExecuted = 0;
  while (totalCyclesExecuted < targetCycles) {
    uint8_t cyclesExecuted = cpuRunSingleOp(cpu, memoryController);
    totalCyclesExecuted += cyclesExecuted;
    cpuUpdateIME(cpu);
    cartridgeUpdate(memoryController, cyclesExecuted, speedMode);
    dmaUpdate(memoryController, cyclesExecuted);
    timerUpdateDivider(timerController, cyclesExecuted);
    timerUpdateTimer(timerController, interruptController, speedMode, cyclesExecuted);
    lcdUpdate(lcdController, interruptController, speedMode, cyclesExecuted);
    cpuHandleInterrupts(cpu, interruptController, memoryController, gameBoyType);
  }
}