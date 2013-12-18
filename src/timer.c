#include "timer.h"

static const uint32_t INPUT_CLOCKS[] = {4096, 262144, 65536, 16384};

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

void timerUpdateTimer(TimerController* timerController, InterruptController* interruptController, uint8_t cyclesExecuted)
{
  // Only update the timer if the enable bit is set in the TAC I/O register
  if (timerController->tac & TAC_TIMER_STOP_BIT) {
    // Determine the clock frequency for timer updates
    uint32_t selectedInputClock = INPUT_CLOCKS[timerController->tac & TAC_INPUT_CLOCK_SELECT_BITS];

    // Work out the number of cycles that should pass before we update the internal counter that ends with an increment to the TIMA register
    // NOTE: Double the required number of clock cyles if double-speed mode is enabled, because while
    // the base clock speed is twice as fast the timer frequency is fixed.
    uint32_t timerIncrementClockCycles = (CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / selectedInputClock) * 1;

    int remainingCycles = cyclesExecuted;
    while (remainingCycles > 0) {
      int updateCycles = ((remainingCycles > timerIncrementClockCycles) ? timerIncrementClockCycles : remainingCycles);
      // Check for updates
      if ((timerController->timerCounter + updateCycles) >= timerIncrementClockCycles) {
        // Increment TIMA and trigger an interrupt and load of TMA if an overflow occurred
        timerController->tima++;
        // debug("TIMA: %u\n", timerController->tima);
        if (timerController->tima == 0) {
          // debug("TIMA INTERRUPT! TAC=0x%02X\n", timerController->tac);
          timerController->tima = timerController->tma;
          interruptFlag(interruptController, TIMER_OVERFLOW_INTERRUPT_BIT);
        }
      }
      timerController->timerCounter = (timerController->timerCounter + updateCycles) % timerIncrementClockCycles;
      remainingCycles -= updateCycles;
    }
  }
}