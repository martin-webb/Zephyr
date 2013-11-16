#ifndef SPEEDCONTROLLER_H_
#define SPEEDCONTROLLER_H_

#include <stdint.h>

#define IO_REG_ADDRESS_KEY1 0xFF4D

typedef struct {
  uint8_t key1; // FF4D - Prepare Speed Switch
} SpeedController;

void initSpeedController(SpeedController* speedController);

#endif // SPEEDCONTROLLER_H_