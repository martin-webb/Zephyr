#include "interrupts.h"

#include <stdio.h>

void interruptFlag(InterruptController* interruptController, uint8_t interruptBit)
{
  interruptController->f |= interruptBit;
}

void interruptReset(InterruptController* interruptController, uint8_t interruptBit)
{
  // TODO: Check this use of XOR to reset a single bit under the assumption that "interruptBit" is the correct value for the single bit being reset
  interruptController->f ^= interruptBit;
}