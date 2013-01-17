#include <stdint.h>

// Enum with dual functionality
typedef enum {
  GB,
  GBP,
  CGB,
  SGB
} GBType;

typedef enum {
  CGBModeGB,
  CGBModeCGB
} CGBMode;

GBType gbGetGameType(uint8_t* cartridgeData);