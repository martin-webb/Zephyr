#include <stdio.h>

#include "timer.h"

void timerUpdateDivider(TimerState* t, MemoryController* m, uint8_t cyclesExecuted)
{
  // If we have executed enough clock cycles-worth of time since the last update then increment the DIV register
  // (taking into account any 'excess' clock cycles that we should include), otherwise simply update the clock cycle count.
  // NOTE: In double-speed mode the divider increments twice as fast at 32768Hz, however because we
  // increment based on clock cycles instead of real time values we don't need to do anything else here,
  // because when double-speed mode is enabled (and the clock is effectively twice as fast) the same
  // number of increments happens in the same period of time. 
  // Alternatively, in double-speed mode the base clock is twice as fast, but the update frequency sis also twice as fast,
  // so the same number of clock cycles are executed before the register is incremented, so we don't have to do anything special here.
  if (t->dividerCounter + cyclesExecuted >= DIV_INCREMENT_CLOCK_CYCLES) {
    incrementByte(m, IO_REG_ADDRESS_DIV); // NOTE: Not using readByte() and writeByte() here because writeByte() will cause a value of 0x00 to be written
    t->dividerCounter = DIV_INCREMENT_CLOCK_CYCLES - t->dividerCounter;
  } else {
    t->dividerCounter += cyclesExecuted;
  }
}

void timerUpdateTimer(TimerState* t, MemoryController* m, SpeedMode speedMode, uint8_t cyclesExecuted)
{
  // Get timer status and clock frequency from TAC
  uint8_t timerControl = readByte(m, IO_REG_ADDRESS_TAC);
  
  // Only update the timer if the enable bit is set in the TAC I/O register
  if (timerControl & TAC_TIMER_STOP_BIT) {
    
    // Determine the clock frequency for timer updates
    const uint32_t inputClocks[] = {4096, 262144, 65536, 16384};
    uint32_t selectedInputClock = inputClocks[timerControl & TAC_INPUT_CLOCK_SELECT_BITS];
    
    // Work out the number of cycles that should pass before we update the internal counter that ends with an increment to the TIMA register
    // NOTE: Double the required number of clock cyles if double-speed mode is enabled, because while
    // the base clock speed is twice as fast the timer frequency is fixed.
    uint32_t timerIncrementClockCycles = (CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / selectedInputClock) * ((speedMode == DOUBLE) ? 2 : 1);
    
    // Check for updates
    if (t->timerCounter + cyclesExecuted >= timerIncrementClockCycles) {
      
      // Get the actual value of TIMA
      uint8_t timaValue = readByte(m, IO_REG_ADDRESS_TIMA);
      
      // Trigger an interrupt and load of TMA if an overflow occurred, otherwise just increment
      if ((uint8_t)(timaValue + 1) == 0) {
        uint8_t timerModulo = readByte(m, IO_REG_ADDRESS_TMA);
        writeByte(m, IO_REG_ADDRESS_TIMA, timerModulo);
        cpuFlagInterrupt(m, TIMER_OVERFLOW_INTERRUPT_BIT);
      } else {
        writeByte(m, IO_REG_ADDRESS_TIMA, timaValue + 1);
      }
      
      // Set the clock cycles counter to the unused number of cycles actually executed, as if the update had been triggered at exactly the right clock pulse
      t->timerCounter = timerIncrementClockCycles - t->timerCounter;
    } else {
      t->timerCounter += cyclesExecuted;
    }
    
  }
}