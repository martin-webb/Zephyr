#include "memory.h"

#define CARTRIDGE_SIZE 0x8000

bool AddressIsROMSpace(unsigned short address)
{
  return ((address & 0xFF00) >= 0x0000) && ((address & 0xFF00) < CARTRIDGE_SIZE);
}

unsigned char ROMOnlyReadByte(unsigned short address, MemoryController* memoryController)
{
  if (AddressIsROMSpace(address)) {
    return memoryController->cartridge[address];
  } else {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void ROMOnlyWriteByte(unsigned short address, unsigned char value, MemoryController* memoryController)
{
  if (AddressIsROMSpace(address)) {
    printf("[FATAL]: Attempt to write to ROM space on ROM Only cartridge");
    exit(1);
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // Additional write to support the echo of 7.5KB of internal RAM
    if (((address & 0xFF00) >= 0xC000) && ((address & 0xFF00) < 0xDE00)) {
      memoryController->memory[address - CARTRIDGE_SIZE + 0x2000];
    } else if (((address & 0xFF00) >= 0xE000) && ((address & 0xFF00) < 0xFE00)) {
      memoryController->memory[address - CARTRIDGE_SIZE - 0x2000];
    }
  }
}

MemoryController InitROMOnlyMemoryController(unsigned char* memory, unsigned char* cartridge)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    &ROMOnlyReadByte,
    &ROMOnlyWriteByte
  };
  return memoryController;
}