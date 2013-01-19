#ifndef TIMER_H_
#define TIMER_H_

#include "cpu.h"
#include "gbtype.h"
#include "memory.h"

#define IO_REG_ADDRESS_DIV 0xFF04
#define IO_REG_ADDRESS_TIMA 0xFF05
#define IO_REG_ADDRESS_TMA 0xFF06
#define IO_REG_ADDRESS_TAC 0xFF07

#define DIV_INCREMENT_CLOCK_CYCLES (CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / 16384)
// NOTE: No need for an SGB specific value here - both the base clock frequency and div increment
// frequency are higher but yield practically the same number of clock cycles for a single increment.

typedef struct {
  uint32_t dividerCounter;
  uint32_t timerCounter;
} TimerState;

void timerUpdateDivider(TimerState* t, MemoryController* m, GameBoyType gameBoyType, uint8_t cyclesExecuted);

#endif // TIMER_H_