#include <stdbool.h>
#include <stdint.h>

typedef struct MemoryController MemoryController;

struct MemoryController {
  uint8_t* memory;
  uint8_t* cartridge;
  uint8_t bankSelect; // 2-bit register to select EITHER RAM Bank 00-03h or to specify the upper two bits (5 and 6, 0-based) of the ROM bank mapped to 0x4000-0x7FFF
  uint8_t modeSelect; // 1-bit register to select whether the above 2-bit applies to ROM/RAM bank selection
  uint8_t romBank;
  bool ramEnabled; // TODO: Store a bool value or the actual value written to 0x0000-0x1FFFF?
  uint8_t (*readByteImpl)(uint16_t address, MemoryController* memoryController);
  void (*writeByteImpl)(uint16_t address, uint8_t value, MemoryController* memoryController);
};

uint8_t readByte(uint16_t address, MemoryController* memoryController);
uint16_t readWord(uint16_t address, MemoryController* memoryController);
void writeByte(uint16_t address, uint8_t value, MemoryController* memoryController);

MemoryController InitMemoryController(uint8_t cartridgeType, uint8_t* memory, uint8_t* cartridge);

/****************************************************************************/

uint8_t ROMOnlyReadByte(uint16_t address, MemoryController* memoryController);
void ROMOnlyWriteByte(uint16_t address, uint8_t value, MemoryController* memoryController);

MemoryController InitROMOnlyMemoryController(uint8_t* memory, uint8_t* cartridge);

/****************************************************************************/

uint8_t ROMOnlyReadByte(uint16_t address, MemoryController* memoryController);
void ROMOnlyWriteByte(uint16_t address, uint8_t value, MemoryController* memoryController);

MemoryController InitMBC1MemoryController(uint8_t* memory, uint8_t* cartridge);