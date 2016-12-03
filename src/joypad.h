#ifndef JOYPAD_H_
#define JOYPAD_H_

#include <stdbool.h>
#include <stdint.h>

#define IO_REG_ADDRESS_P1 0xFF00

typedef struct {
  uint8_t p1; // FF00 - Joypad (R/W)

  bool _a;
  bool _b;
  bool _up;
  bool _down;
  bool _left;
  bool _right;
  bool _start;
  bool _select;

} JoypadController;

void initJoypadController(JoypadController* joypadController);

uint8_t joypadReadByte(JoypadController* joypadController, uint16_t address);
void joypadWriteByte(JoypadController* joypadController, uint16_t address, uint8_t value);

#endif // JOYPAD_H_
