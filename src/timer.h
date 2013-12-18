#ifndef TIMER_H_
#define TIMER_H_

#include "cpu.h"
#include "timercontroller.h"

#define IO_REG_ADDRESS_DIV 0xFF04
#define IO_REG_ADDRESS_TIMA 0xFF05
#define IO_REG_ADDRESS_TMA 0xFF06
#define IO_REG_ADDRESS_TAC 0xFF07

#define TAC_TIMER_STOP_BIT_SHIFT 2
#define TAC_TIMER_STOP_BIT (1 << TAC_TIMER_STOP_BIT_SHIFT)

#define TAC_INPUT_CLOCK_SELECT_BITS 3

#define DIV_INCREMENT_CLOCK_CYCLES (CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / 16384)
// NOTE: No need for an SGB specific value here - both the base clock frequency and div increment
// frequency are higher but yield practically the same number of clock cycles for a single increment.
// This also applies to CGB double-speed mode, where the base clock speed is doubled, but so is the
// frequency of the divider clock speed, resulting in the same number of clock cycles passing before
// an increment occurs.

#define TIMER_OVERFLOW_INTERRUPT_BIT (1 << 2)

void timerUpdateDivider(TimerController* timerController, uint8_t cyclesExecuted);
void timerUpdateTimer(TimerController* timerController, InterruptController* interruptController, uint8_t cyclesExecuted);

#endif // TIMER_H_