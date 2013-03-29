#include <stdio.h>

#include "timer.h"

void timerUpdateDivider(TimerController* timerController, uint8_t cyclesExecuted)
{
  // If we have executed enough clock cycles-worth of time since the last update then increment the DIV register
  // (taking into account any 'excess' clock cycles that we should include), otherwise simply update the clock cycle count.
  // NOTE: In double-speed mode the divider increments twice as fast at 32768Hz, however because we
  // increment based on clock cycles instead of real time values we don't need to do anything else here,
  // because when double-speed mode is enabled (and the clock is effectively twice as fast) the same
  // number of increments happens in the same period of time. 
  // Alternatively, in double-speed mode the base clock is twice as fast, but the update frequency sis also twice as fast,
  // so the same number of clock cycles are executed before the register is incremented, so we don't have to do anything special here.
  if (timerController->dividerCounter + cyclesExecuted >= DIV_INCREMENT_CLOCK_CYCLES) {
    timerController->div++;
  }
  timerController->dividerCounter = (timerController->dividerCounter + cyclesExecuted) % DIV_INCREMENT_CLOCK_CYCLES;
}

void timerUpdateTimer(TimerController* timerController, InterruptController* interruptController, SpeedMode speedMode, uint8_t cyclesExecuted)
{
  // Only update the timer if the enable bit is set in the TAC I/O register
  if (timerController->tac & TAC_TIMER_STOP_BIT) {
    // NOTE: Processing cycles individually is the simplest way of ensuring that the timer can increment
    // multiple times per update if required (if, for example, after the last update we were 4 cycles
    // away from the next increment (in 262144 Hz mode) and we then executed a 24-cycle instruction,
    // putting us at 36 cycles-worth of updates, 2 full timer increments).
    
    // NOTE: We could reduce the number of loop iterations here if we knew that cyclesExecuted would
    // always be divisible by 4 by incrementing in blocks of 4, but if a CPU in the HALT/STOP state
    // can tick in a single cycle (because in this emulator design the CPU "drives" the other components)
    // then this approach isn't safe.
    
    // NOTE: Alternatively, we could increment by a value up to that required for the next TIMA increment
    // (if the value of cyclesExecuted allowed for this), and the again by the remainder, if needed,
    // knowing that for a single CPU instruction and with the timer set to increment every 16 cycles
    // (at 262144 Hz) there could be at MOST one TIMA increment plus some remainder clock cycles.
    
    for (int c = 0; c < cyclesExecuted; c++) {
      // Determine the clock frequency for timer updates
      const uint32_t inputClocks[] = {4096, 262144, 65536, 16384};
      uint32_t selectedInputClock = inputClocks[timerController->tac & TAC_INPUT_CLOCK_SELECT_BITS];

      // Work out the number of cycles that should pass before we update the internal counter that ends with an increment to the TIMA register
      // NOTE: Double the required number of clock cyles if double-speed mode is enabled, because while
      // the base clock speed is twice as fast the timer frequency is fixed.
      uint32_t timerIncrementClockCycles = (CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / selectedInputClock) * 1;

      // Check for updates
      if ((timerController->timerCounter + 1) >= timerIncrementClockCycles) {
        // Increment TIMA and trigger an interrupt and load of TMA if an overflow occurred
        timerController->tima++;
        // debug("TIMA: %u\n", timerController->tima);
        if (timerController->tima == 0) {
          // debug("TIMA INTERRUPT! TAC=0x%02X\n", timerController->tac);
          timerController->tima = timerController->tma;
          interruptFlag(interruptController, TIMER_OVERFLOW_INTERRUPT_BIT);
        }
      }
      timerController->timerCounter = (timerController->timerCounter + 1) % timerIncrementClockCycles;
    }
    
  }
}