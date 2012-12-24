#include <stdbool.h>

typedef struct MemoryController MemoryController;

struct MemoryController {
  unsigned char* memory;
  unsigned char* cartridge;
  unsigned char (*readByte)(unsigned short address, MemoryController* memoryController);
  void (*writeByte)(unsigned short address, unsigned char value, MemoryController* memoryController);
};

bool AddressIsROMSpace(unsigned short address);

unsigned char ROMOnlyReadByte(unsigned short address, MemoryController* memoryController);
void ROMOnlyWriteByte(unsigned short address, unsigned char value, MemoryController* memoryController);

MemoryController InitROMOnlyMemoryController(unsigned char* memory, unsigned char* cartridge);