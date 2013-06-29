#ifndef GAMEBOY_H_
#define GAMEBOY_H_

#include "cpu.h"
#include "timer.h"

#include <stdint.h>

typedef struct {
  CPU cpu;
  JoypadController joypadController;
  LCDController lcdController;
  TimerController timerController;
  InterruptController interruptController;
  MemoryController memoryController;
  GameBoyType gameBoyType;
  SpeedMode speedMode;

  uint8_t* vram;
  uint8_t* wram;
  uint8_t* oam;
  uint8_t* hram;
} GameBoy;

void gbInitialise(GameBoy* gameBoy, GameBoyType gameBoyType, uint8_t* cartridgeData, uint8_t* frameBuffer, const char* romFilename);
void gbFinalise(GameBoy* gameBoy);

GameBoyType gbGetGameType(uint8_t* cartridgeData);

void gbRunNFrames(GameBoy* gameBoy, const int frames);

#endif // GAMEBOY_H_