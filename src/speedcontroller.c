#include "speedcontroller.h"

void initSpeedController(SpeedController* speedController)
{
  speedController->key1 = 0;
}


uint8_t speedReadByte(SpeedController* speedController, uint16_t address)
{
  return speedController->key1;
}


void speedWriteByte(SpeedController* speedController, uint16_t address, uint8_t value)
{
  speedController->key1 = (speedController->key1 | (value & 1));
}
