#include <stdint.h>

// Enum with dual functionality
typedef enum {
  GB,
  GBP,
  CGB,
  SGB
} GameBoyType;

typedef enum {
  NORMAL,
  DOUBLE
} SpeedMode;

GameBoyType gbGetGameType(uint8_t* cartridgeData);