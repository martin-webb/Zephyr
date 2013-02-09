#ifndef GAMEBOY_H_
#define GAMEBOY_H_

#include <stdint.h>

#include "cpu.h"
#include "gbtype.h"
#include "interrupts.h"
#include "memory.h"
#include "speed.h"
#include "timer.h"

GameBoyType gbGetGameType(uint8_t* cartridgeData);
uint32_t gbRunAtLeastNCycles(
  CPU* cpu,
  MemoryController* m,
  TimerController* timerController,
  InterruptController* interruptController,
  GameBoyType gameBoyType,
  SpeedMode speedMode,
  uint32_t targetCycles
);

#endif // GAMEBOY_H_