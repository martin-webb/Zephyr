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
} CPU;

typedef enum {
  GB,
  GBP,
  GBC,
  SGB
} GBType;