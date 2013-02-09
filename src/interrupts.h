#ifndef INTERRUPTS_H_
#define INTERRUPTS_H_

#include <stdint.h>

#define IO_REG_ADDRESS_IF 0xFF0F
#define IO_REG_ADDRESS_IE 0xFFFF

typedef struct {
  uint8_t f; // FF0F - Interrupt Flag (R/W)
  uint8_t e; // FFFF - Interrupt Enable (R/W)
} InterruptController;

void interruptFlag(InterruptController* interruptController, uint8_t interruptBit);
void interruptReset(InterruptController* interruptController, uint8_t interruptBit);

#endif // INTERRUPTS_H_