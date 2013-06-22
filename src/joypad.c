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