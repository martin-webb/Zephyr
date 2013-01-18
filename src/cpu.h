#include <stdbool.h>

#include "gameboy.h"
#include "memory.h"

#define CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED 4194304
#define CLOCK_CYCLE_TIME_SECS_NORMAL_SPEED (1.0 / CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED)

#define CLOCK_CYCLE_FREQUENCY_DOUBLE_SPEED 8388608
#define CLOCK_CYCLE_TIME_SECS_DOUBLE_SPEED (1.0 / CLOCK_CYCLE_FREQUENCY_DOUBLE_SPEED)

#define CLOCK_CYCLE_FREQUENCY_SGB 4295454
#define CLOCK_CYCLE_TIME_SECS_SGB (1.0 / CLOCK_CYCLE_FREQUENCY_SGB)

#define FLAG_REGISTER_C_BIT_SHIFT 4
#define FLAG_REGISTER_H_BIT_SHIFT 5
#define FLAG_REGISTER_N_BIT_SHIFT 6
#define FLAG_REGISTER_Z_BIT_SHIFT 7

#define FLAG_REGISTER_C_BIT (0x1 << FLAG_REGISTER_C_BIT_SHIFT)
#define FLAG_REGISTER_H_BIT (0x1 << FLAG_REGISTER_H_BIT_SHIFT)
#define FLAG_REGISTER_N_BIT (0x1 << FLAG_REGISTER_N_BIT_SHIFT)
#define FLAG_REGISTER_Z_BIT (0x1 << FLAG_REGISTER_Z_BIT_SHIFT)

#define BIT_0_SHIFT 0
#define BIT_0 (0x1 << BIT_0_SHIFT)

#define BIT_7_SHIFT 7
#define BIT_7 (0x1 << BIT_7_SHIFT)

#define VBLANK_INTERRUPT_START_ADDRESS 0x40
#define LCDC_STATUS_INTERRUPT_START_ADDRESS 0x48
#define TIMER_OVERFLOW_INTERRUPT_START_ADDRESS 0x50
#define SERIAL_TRANSFER_COMPLETION_INTERRUPT_START_ADDRESS 0x58
#define HIGH_TO_LOW_P10_TO_P13_INTERRUPT_START_ADDRESS 0x60

#define CPU_MIN_CYCLES_PER_SET 100
#define SECONDS_TO_NANOSECONDS 1000000000

typedef struct {
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t sp;
  uint16_t pc;
} CPURegisters;

typedef struct {
  CPURegisters registers;
  bool halt;
  bool stop;
  bool ime;
  uint8_t di; // Control value to trigger a disable of interrupts "after the instruction after DI is executed"
  uint8_t ei; // Control value to trigger an enable of interrupts "after the instruction after EI is executed"
} CPU;

void cpuReset(CPU* cpu, MemoryController* m, GameBoyType gameBoyType);
void cpuPrintState(CPU* cpu);
uint32_t cpuRunAtLeastNCycles(CPU* cpu, MemoryController* m, GameBoyType gameBoyType, SpeedMode speedMode, uint32_t targetCycles);