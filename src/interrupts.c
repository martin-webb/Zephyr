#include "interrupts.h"

void initInterruptController(InterruptController* interruptController)
{
  interruptController->f = 0;
  interruptController->e = 0;
}


uint8_t interruptReadByte(InterruptController* interruptController, uint16_t address)
{
  if (address == IO_REG_ADDRESS_IF) { // 0xFF0F
    return interruptController->f;
  } else if (address == IO_REG_ADDRESS_IE) { // 0xFFFF
    return interruptController->e;
  } else {
    return 0x00;
  }
}


void interruptWriteByte(InterruptController* interruptController, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_IF) { // 0xFF0F
    interruptController->f = value;
  } else if (address == IO_REG_ADDRESS_IE) { // 0xFFFF
    interruptController->e = value;
  }
}


void interruptFlag(InterruptController* interruptController, uint8_t interruptBit)
{
  interruptController->f |= interruptBit;
}

void interruptReset(InterruptController* interruptController, uint8_t interruptBit)
{
  // TODO: Check this use of XOR to reset a single bit under the assumption that "interruptBit" is the correct value for the single bit being reset
  interruptController->f ^= interruptBit;
}
