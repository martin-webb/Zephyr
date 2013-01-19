#include <stdio.h>

#include "timer.h"

void timerUpdateDivider(TimerState* t, MemoryController* m, uint8_t cyclesExecuted)
{
  // If we have executed enough clock cycles-worth of time since the last update then increment the DIV register
  // (taking into account any 'excess' clock cycles that we should include), otherwise simply update the clock cycle count.
  if (t->dividerCounter + cyclesExecuted >= DIV_INCREMENT_CLOCK_CYCLES) {
    incrementByte(m, IO_REG_ADDRESS_DIV); // NOTE: Not using readByte() and writeByte() here because writeByte() will cause a value of 0x00 to be written
    t->dividerCounter = DIV_INCREMENT_CLOCK_CYCLES - t->dividerCounter;
  } else {
    t->dividerCounter += cyclesExecuted;
  }
}