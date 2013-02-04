#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdbool.h>
#include <stdint.h>

#include "timercontroller.h"

typedef struct MemoryController MemoryController;

struct MemoryController {
  uint8_t* memory;
  uint8_t* cartridge;
  uint8_t bankSelect; // 2-bit register to select EITHER RAM Bank 00-03h or to specify the upper two bits (5 and 6, 0-based) of the ROM bank mapped to 0x4000-0x7FFF
  uint8_t modeSelect; // 1-bit register to select whether the above 2-bit applies to ROM/RAM bank selection
  uint8_t romBank;
  bool ramEnabled; // TODO: Store a bool value or the actual value written to 0x0000-0x1FFFF?
  uint8_t (*readByteImpl)(MemoryController* memoryController, uint16_t address);
  void (*writeByteImpl)(MemoryController* memoryController, uint16_t address, uint8_t value);
  TimerController* timerController;
};

uint8_t readByte(MemoryController* memoryController, uint16_t address);
uint16_t readWord(MemoryController* memoryController, uint16_t address);
void writeByte(MemoryController* memoryController, uint16_t address, uint8_t value);
void writeWord(MemoryController* memoryController, uint16_t address, uint16_t value);

MemoryController InitMemoryController(uint8_t cartridgeType, uint8_t* memory, uint8_t* cartridge, TimerController* timerController);

/****************************************************************************/

uint8_t ROMOnlyReadByte(MemoryController* memoryController, uint16_t address);
void ROMOnlyWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value);

MemoryController InitROMOnlyMemoryController(uint8_t* memory, uint8_t* cartridge, TimerController* timerController);

/****************************************************************************/

uint8_t ROMOnlyReadByte(MemoryController* memoryController, uint16_t address);
void ROMOnlyWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value);

MemoryController InitMBC1MemoryController(uint8_t* memory, uint8_t* cartridge, TimerController* timerController);

#endif // MEMORY_H_