#include "timercontroller.h"


void initTimerController(TimerController* timerController, InterruptController* interruptController)
{
  timerController->dividerCounter = 0;
  timerController->timerCounter = 0;
  timerController->interruptController = interruptController;
}
