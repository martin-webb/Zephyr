#include <stdbool.h>

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

typedef enum {
  GB,
  GBP,
  GBC,
  SGB
} GBType;