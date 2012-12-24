#include <stdbool.h>

typedef struct MemoryController MemoryController;

typedef enum {
  None,
  ROM16RAM8,
  ROM4RAM32
} MBCMemoryModel;

struct MemoryController {
  unsigned char* memory;
  unsigned char* cartridge;
  MBCMemoryModel memoryModel;
  unsigned char (*readByteImpl)(unsigned short address, MemoryController* memoryController);
  void (*writeByteImpl)(unsigned short address, unsigned char value, MemoryController* memoryController);
};

bool addressIsROMSpace(unsigned short address);

unsigned char readByte(unsigned short address, MemoryController* memoryController);
unsigned short readWord(unsigned short address, MemoryController* memoryController);
void writeByte(unsigned short address, unsigned char value, MemoryController* memoryController);

MemoryController InitMemoryController(unsigned char cartridgeType, unsigned char* memory, unsigned char* cartridge);

/****************************************************************************/

unsigned char ROMOnlyReadByte(unsigned short address, MemoryController* memoryController);
void ROMOnlyWriteByte(unsigned short address, unsigned char value, MemoryController* memoryController);

MemoryController InitROMOnlyMemoryController(unsigned char* memory, unsigned char* cartridge);

/****************************************************************************/

unsigned char ROMOnlyReadByte(unsigned short address, MemoryController* memoryController);
void ROMOnlyWriteByte(unsigned short address, unsigned char value, MemoryController* memoryController);

MemoryController InitMBC1MemoryController(unsigned char* memory, unsigned char* cartridge);