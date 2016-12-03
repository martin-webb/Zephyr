#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdbool.h>
#include <stdint.h>

#define IO_REG_ADDRESS_DMA 0xFF46

#define IO_REG_ADDRESS_HDMA1 0xFF51
#define IO_REG_ADDRESS_HDMA2 0xFF52
#define IO_REG_ADDRESS_HDMA3 0xFF53
#define IO_REG_ADDRESS_HDMA4 0xFF54
#define IO_REG_ADDRESS_HDMA5 0xFF55

#include "sound/soundcontroller.h"
#include "speedcontroller.h"
#include "memorycontroller.h"

MemoryController InitMemoryController(
  uint8_t cartridgeType,
  uint8_t* vram,
  uint8_t* wram,
  uint8_t* oam,
  uint8_t* hram,
  uint8_t* cartridge,
  CGBMode cgbMode,
  JoypadController* joypadController,
  LCDController* lcdController,
  SoundController* soundController,
  TimerController* timerController,
  InterruptController* interruptController,
  SpeedController* speedController,
  uint32_t externalRAMSizeBytes,
  const char* romFilename
);

uint8_t readByte(MemoryController* memoryController, uint16_t address);
uint16_t readWord(MemoryController* memoryController, uint16_t address);
void writeByte(MemoryController* memoryController, uint16_t address, uint8_t value);
void writeWord(MemoryController* memoryController, uint16_t address, uint16_t value);

uint8_t commonReadByte(MemoryController* memoryController, uint16_t address);
void commonWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value);

void cartridgeUpdate(MemoryController* memoryController, uint8_t cyclesExecuted);
void dmaUpdate(MemoryController* memoryController, uint8_t cyclesExecuted);
void hdmaUpdate(MemoryController* memoryController, uint8_t cyclesExecuted);

bool generalPurposeDMAIsActive(MemoryController* memoryController);

#endif // MEMORY_H_
