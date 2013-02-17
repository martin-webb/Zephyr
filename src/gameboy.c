#include <stdlib.h>
#include <sys/time.h>

#include "gameboy.h"

#include "cartridge.h"

GameBoyType gbGetGameType(uint8_t* cartridgeData)
{
  if (cartridgeData[SGB_FLAG_ADDRESS] == 0x03) {
    return SGB;
  } else {
    if (cartridgeData[CGB_FLAG_ADDRESS] == 0x80) {
      return CGB;
    } else {
      return GB;
    }
  }
}

uint32_t gbRunAtLeastNCycles(
  CPU* cpu,
  MemoryController* memoryController,
  LCDController* lcdController,
  TimerController* timerController,
  InterruptController* interruptController,
  GameBoyType gameBoyType,
  SpeedMode speedMode,
  uint32_t targetCycles
)
{
  uint32_t totalCyclesExecuted = 0;
  uint32_t totalOpsExecuted = 0;
  
  if (speedMode == DOUBLE) {
    targetCycles *= 2; // TODO: Modifying the function parameter seems a nasty way of doing this
  }
  
  // Execute instructions until we have reached the minimum required number of cycles that would have occurred
  while (totalCyclesExecuted < targetCycles) {
    uint8_t cyclesExecuted = cpuRunSingleOp(cpu, memoryController);
    totalCyclesExecuted += cyclesExecuted;
    totalOpsExecuted++;
    cpuUpdateIME(cpu);
    timerUpdateDivider(timerController, cyclesExecuted);
    timerUpdateTimer(timerController, interruptController, speedMode, cyclesExecuted);
    lcdUpdate(lcdController, interruptController, cyclesExecuted);
    cpuHandleInterrupts(cpu, interruptController, memoryController);
  }
  
  // Determine the correct amount of time to sleep for based on the Game Boy type and (for CGB) the speed mode
  float clockCycleTimeSecs;
  if (gameBoyType == GB || gameBoyType == GBP) {
    clockCycleTimeSecs = CLOCK_CYCLE_TIME_SECS_NORMAL_SPEED;
  } else if (gameBoyType == CGB) {
    if (speedMode == NORMAL) {
      clockCycleTimeSecs = CLOCK_CYCLE_TIME_SECS_NORMAL_SPEED;
    } else if (speedMode == DOUBLE) {
      clockCycleTimeSecs = CLOCK_CYCLE_TIME_SECS_DOUBLE_SPEED;
    } else {
      fprintf(stderr, "%s: Unknown value for speedMode '%i'", __func__, speedMode);
      exit(EXIT_FAILURE);
    }
  } else if (gameBoyType == SGB) {
    clockCycleTimeSecs = CLOCK_CYCLE_TIME_SECS_SGB;
  } else {
    fprintf(stderr, "%s: Unknown value for gameBoyType '%i'", __func__, gameBoyType);
    exit(EXIT_FAILURE);
  }
  
  // Sleep for the amount of time needed to approximate the operating speed of the Game Boy
  struct timespec sleepRequested = {0, totalCyclesExecuted * clockCycleTimeSecs * SECONDS_TO_NANOSECONDS};
  struct timespec sleepRemaining;
  nanosleep(&sleepRequested, &sleepRemaining);
  
  // debug("Executed %u ops in %u cycles - sleeping for %luns\n", totalOpsExecuted, totalCyclesExecuted, sleepRequested.tv_nsec);
  
  return totalCyclesExecuted; 
}