#ifndef TIMERCONTROLLER_H_
#define TIMERCONTROLLER_H_

#include <stdint.h>

typedef struct {
  uint8_t div;  // FF04 - Divider Register (R/W)
  uint8_t tima; // FF05 - Timer Counter (R/W)
  uint8_t tma;  // FF06 - Timer Modulo (R/W)
  uint8_t tac;  // FF07 - Timer Control (R/W)

  uint32_t dividerCounter;
  uint32_t timerCounter;
} TimerController;

void initTimerController(TimerController* timerController);

#endif // TIMERCONTROLLER_H_