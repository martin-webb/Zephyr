#include <stdbool.h>

typedef struct MemoryController MemoryController;

struct MemoryController {
  unsigned char* memory;
  unsigned char* cartridge;
  unsigned char (*readByteImpl)(unsigned short address, MemoryController* memoryController);
  void (*writeByteImpl)(unsigned short address, unsigned char value, MemoryController* memoryController);
};

bool addressIsROMSpace(unsigned short address);

unsigned char readByte(unsigned short address, MemoryController* memoryController);
unsigned short readWord(unsigned short address, MemoryController* memoryController);
void writeByte(unsigned short address, unsigned char value, MemoryController* memoryController);

unsigned char ROMOnlyReadByte(unsigned short address, MemoryController* memoryController);
void ROMOnlyWriteByte(unsigned short address, unsigned char value, MemoryController* memoryController);

MemoryController InitROMOnlyMemoryController(unsigned char* memory, unsigned char* cartridge);