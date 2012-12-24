#include <stdbool.h>
#include <stdint.h>

typedef struct MemoryController MemoryController;

typedef enum {
  None,
  ROM16RAM8,
  ROM4RAM32
} MBCMemoryModel;

struct MemoryController {
  uint8_t* memory;
  uint8_t* cartridge;
  MBCMemoryModel memoryModel;
  uint8_t (*readByteImpl)(uint16_t address, MemoryController* memoryController);
  void (*writeByteImpl)(uint16_t address, uint8_t value, MemoryController* memoryController);
};

bool addressIsROMSpace(uint16_t address);

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