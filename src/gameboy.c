#include "gameboy.h"

#include "cartridge.h"

#import <AudioToolbox/AudioToolbox.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VRAM_SIZE_BYTES (8 * 1024)
#define WRAM_SIZE_BYTES (8 * 1024)
#define OAM_SIZE_BYTES 160
#define HRAM_SIZE_BYTES 127

#define AUDIO_SAMPLE_RATE 44100

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

  initCPU(&gameBoy->cpu, &gameBoy->memoryController, &gameBoy->interruptController, gameBoyType);
  initLCDController(&gameBoy->lcdController, &gameBoy->interruptController, gameBoy->vram, gameBoy->oam, frameBuffer, gameBoyType, cgbMode);
  initSoundController(&gameBoy->soundController);
  initJoypadController(&gameBoy->joypadController);
  initTimerController(&gameBoy->timerController, &gameBoy->interruptController);
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
    &gameBoy->soundController,
    &gameBoy->timerController,
    &gameBoy->interruptController,
    &gameBoy->speedController,
    externalRAMSizeBytes,
    romFilename
  );

  gameBoy->gameBoyType = gameBoyType;
  gameBoy->cgbMode = cgbMode;

  gameBoy->cyclesBeforeNextAudioSample = 0;

  cpuReset(&gameBoy->cpu);
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

void gbRunNFrames(GameBoy* gameBoy, AudioSampleBuffer* audioSampleBuffer, const int frames)
{
  CPU* cpu = &gameBoy->cpu;
  LCDController* lcdController = &gameBoy->lcdController;
  SoundController* soundController = &gameBoy->soundController;
  TimerController* timerController = &gameBoy->timerController;
  MemoryController* memoryController = &gameBoy->memoryController;
  SpeedController* speedController = &gameBoy->speedController;

  const int cyclesBetweenAudioSamples = CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / AUDIO_SAMPLE_RATE;

  // Execute instructions until we have reached the minimum required number of cycles that would have occurred
  const int targetCycles = frames * FULL_FRAME_CLOCK_CYCLES;
  uint32_t totalCyclesExecuted = 0;
  uint32_t audioSampleCycles = gameBoy->cyclesBeforeNextAudioSample;

  while (totalCyclesExecuted < targetCycles) {
    uint8_t cpuCyclesExecuted = cpuRunSingleOp(cpu);

    // Component timings are based off clock cycles instead of "real time", but because most components aren't
    // affected by the CGB's double speed mode (because they are driven by a real timer) we have to take this
    // into account when updating them based on clock cycles.
    uint8_t baseCyclesExecuted = cpuCyclesExecuted / ((speedController->key1 & (1 << 7)) ? 2 : 1);

    cpuUpdateIME(cpu);
    cartridgeUpdate(memoryController, baseCyclesExecuted);
    dmaUpdate(memoryController, cpuCyclesExecuted); // Not using speed adjusted cycles because the DMA transfer runs twice as fast in double speed mode
    hdmaUpdate(memoryController, baseCyclesExecuted);
    timerUpdateDivider(timerController, cpuCyclesExecuted); // Not using speed adjusted cycles because the divider runs twice as fast in double speed mode
    timerUpdateTimer(timerController, cpuCyclesExecuted); // Not using speed adjusted cycles because the timer runs twice as fast in double speed mode
    lcdUpdate(lcdController, baseCyclesExecuted);
    soundUpdate(soundController, baseCyclesExecuted);
    cpuHandleInterrupts(cpu);

    if (audioSampleCycles + baseCyclesExecuted >= cyclesBetweenAudioSamples) {
      AudioSample sample = soundGetCurrentSample(soundController);

      // A Core Audio'ism - don't do this inside the render callback because we might run out of time to fill the buffer
      // TODO: Move this out of here, perhaps to a callback that allows the host app to "transform" the data
      // (preferably as one big chunk to avoid repeated function calls)
      sample.so1 = CFSwapInt16HostToBig(sample.so1);
      sample.so2 = CFSwapInt16HostToBig(sample.so2);

      sampleBufferPut(audioSampleBuffer, sample);
    }
    audioSampleCycles = (audioSampleCycles + baseCyclesExecuted) % cyclesBetweenAudioSamples;

    totalCyclesExecuted += baseCyclesExecuted;
  }

  // Store the current number of cycles before the next audio sample, so the next run loop can take this into account
  gameBoy->cyclesBeforeNextAudioSample = (cyclesBetweenAudioSamples - audioSampleCycles);
}