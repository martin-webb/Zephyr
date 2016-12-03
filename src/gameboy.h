#ifndef GAMEBOY_H_
#define GAMEBOY_H_

#include "cgbmode.h"
#include "cpu.h"
#include "timer.h"
#include "pixel.h"
#include "sound/audiosamplebuffer.h"

#include <stdint.h>

typedef struct {
  CPU cpu;
  JoypadController joypadController;
  LCDController lcdController;
  SoundController soundController;
  TimerController timerController;
  InterruptController interruptController;
  MemoryController memoryController;
  SpeedController speedController;
  GameBoyType gameBoyType;
  CGBMode cgbMode;

  uint8_t* vram;
  uint8_t* wram;
  uint8_t* oam;
  uint8_t* hram;

  int cyclesBeforeNextAudioSample;
} GameBoy;

void gbInitialise(GameBoy* gameBoy, GameBoyType gameBoyType, uint8_t* cartridgeData, Pixel* frameBuffer, const char* romFilename);
void gbFinalise(GameBoy* gameBoy);

GameBoyType gbGetGameType(uint8_t* cartridgeData);

void gbRunNFrames(GameBoy* gameBoy, AudioSampleBuffer* audioSampleBuffer, const int frames);

#endif // GAMEBOY_H_
