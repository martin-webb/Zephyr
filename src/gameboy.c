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
    cartridgeUpdate(memoryController, cyclesExecuted);
    dmaUpdate(memoryController, cyclesExecuted);
    timerUpdateDivider(timerController, cyclesExecuted);
    timerUpdateTimer(timerController, interruptController, speedMode, cyclesExecuted);
    lcdUpdate(lcdController, interruptController, speedMode, cyclesExecuted);
    cpuHandleInterrupts(cpu, interruptController, memoryController, gameBoyType);
  }

  return totalCyclesExecuted;
}