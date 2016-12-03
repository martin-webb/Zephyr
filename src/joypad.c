#include "joypad.h"

void initJoypadController(JoypadController* joypadController)
{
  joypadController->p1 = 0x3F;
  joypadController->_a = false;
  joypadController->_b = false;
  joypadController->_up = false;
  joypadController->_down = false;
  joypadController->_left = false;
  joypadController->_right = false;
  joypadController->_start = false;
  joypadController->_select = false;
}


uint8_t joypadReadByte(JoypadController* joypadController, uint16_t address)
{
  return joypadController->p1;
}


void joypadWriteByte(JoypadController* joypadController, uint16_t address, uint8_t value)
{
  joypadController->p1 = (value & 0x30) | 0x0F; // TODO: Do we need to preserve or reset (set to 1) the low 4 bits here?
  if ((value & (1 << 5)) == 0) {
    if (joypadController->_start)  joypadController->p1 &= ~(1 << 3);
    if (joypadController->_select) joypadController->p1 &= ~(1 << 2);
    if (joypadController->_b)      joypadController->p1 &= ~(1 << 1);
    if (joypadController->_a)      joypadController->p1 &= ~(1 << 0);
  } else if ((value & (1 << 4)) == 0) {
    if (joypadController->_down)  joypadController->p1 &= ~(1 << 3);
    if (joypadController->_up)    joypadController->p1 &= ~(1 << 2);
    if (joypadController->_left)  joypadController->p1 &= ~(1 << 1);
    if (joypadController->_right) joypadController->p1 &= ~(1 << 0);
  }
}
