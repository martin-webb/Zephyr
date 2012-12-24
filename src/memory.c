#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "memory.h"

#define CARTRIDGE_SIZE 0x8000

bool addressIsROMSpace(unsigned short address)
{
  return ((address & 0xFF00) >= 0x0000) && ((address & 0xFF00) < CARTRIDGE_SIZE);
}

unsigned char readByte(unsigned short address, MemoryController* memoryController)
{
  return memoryController->readByteImpl(address, memoryController);
}

unsigned short readWord(unsigned short address, MemoryController* memoryController)
{
  unsigned char lsByte = memoryController->readByteImpl(address, memoryController);
  unsigned char msByte = memoryController->readByteImpl(address + 1, memoryController);
  return (msByte << 8) | lsByte;
}

void writeByte(unsigned short address, unsigned char value, MemoryController* memoryController)
{
  memoryController->writeByteImpl(address, value, memoryController);
}

MemoryController InitMemoryController(unsigned char cartridgeType, unsigned char* memory, unsigned char* cartridge)
{
  switch (cartridgeType) {
    case CARTRIDGE_TYPE_ROM_ONLY:
      return InitROMOnlyMemoryController(memory, cartridge);
      break;
    case CARTRIDGE_TYPE_MBC1:
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM_PLUS_BATTERY:
      return InitMBC1MemoryController(memory, cartridge);
      break;
    case CARTRIDGE_TYPE_MBC2:
    case CARTRIDGE_TYPE_MBC2_PLUS_BATTERY:
    case CARTRIDGE_TYPE_ROM_PLUS_RAM:
    case CARTRIDGE_TYPE_ROM_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MMM01:
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM:
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC3:
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC4:
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC5:
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE:
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_POCKET_CAMERA:
    case CARTRIDGE_TYPE_BANDAI_TAMA5:
    case CARTRIDGE_TYPE_HuC3:
    case CARTRIDGE_TYPE_HuC1_PLUS_RAM_PLUS_BATTERY:
      printf("[FATAL]: Error in InitMemoryController() - cartridge type UNSUPPORTED\n");
      exit(1);
      break;
    default:
      printf("[FATAL]: Error in InitMemoryController() - cartridge type UNKNOWN\n");
      exit(1);
      break;
  }
}

/****************************************************************************/

unsigned char ROMOnlyReadByte(unsigned short address, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
    return memoryController->cartridge[address];
  } else {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void ROMOnlyWriteByte(unsigned short address, unsigned char value, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
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
    None,
    &ROMOnlyReadByte,
    &ROMOnlyWriteByte
  };
  return memoryController;
}

/****************************************************************************/

unsigned char MBC1ReadByte(unsigned short address, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
    return memoryController->cartridge[address];
  } else {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void MBC1WriteByte(unsigned short address, unsigned char value, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
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

MemoryController InitMBC1MemoryController(unsigned char* memory, unsigned char* cartridge)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    ROM16RAM8,
    &MBC1ReadByte,
    &MBC1WriteByte
  };
  return memoryController;
}