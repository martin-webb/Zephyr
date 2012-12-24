#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "memory.h"

#define CARTRIDGE_SIZE 0x8000

bool addressIsROMSpace(uint16_t address)
{
  // NOTE: Comparison of "address > 0x0000" is always true with an unsigned type
  return address < CARTRIDGE_SIZE;
}

uint8_t readByte(uint16_t address, MemoryController* memoryController)
{
  return memoryController->readByteImpl(address, memoryController);
}

uint16_t readWord(uint16_t address, MemoryController* memoryController)
{
  uint8_t lsByte = memoryController->readByteImpl(address, memoryController);
  uint8_t msByte = memoryController->readByteImpl(address + 1, memoryController);
  return (msByte << 8) | lsByte;
}

void writeByte(uint16_t address, uint8_t value, MemoryController* memoryController)
{
  memoryController->writeByteImpl(address, value, memoryController);
}

MemoryController InitMemoryController(uint8_t cartridgeType, uint8_t* memory, uint8_t* cartridge)
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

uint8_t ROMOnlyReadByte(uint16_t address, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
    return memoryController->cartridge[address];
  } else {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void ROMOnlyWriteByte(uint16_t address, uint8_t value, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
    printf("[FATAL]: Attempt to write to ROM space on ROM Only cartridge");
    exit(1);
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // Additional write to support the echo of 7.5KB of internal RAM
    if ((address >= 0xC000) && (address < 0xDE00)) {
      memoryController->memory[address - CARTRIDGE_SIZE + 0x2000];
    } else if ((address >= 0xE000) && (address < 0xFE00)) {
      memoryController->memory[address - CARTRIDGE_SIZE - 0x2000];
    }
  }
}

MemoryController InitROMOnlyMemoryController(uint8_t* memory, uint8_t* cartridge)
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

uint8_t MBC1ReadByte(uint16_t address, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
    return memoryController->cartridge[address];
  } else {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void MBC1WriteByte(uint16_t address, uint8_t value, MemoryController* memoryController)
{
  if (addressIsROMSpace(address)) {
    printf("[FATAL]: Attempt to write to ROM space on ROM Only cartridge");
    exit(1);
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // Additional write to support the echo of 7.5KB of internal RAM
    if ((address >= 0xC000) && (address < 0xDE00)) {
      memoryController->memory[address - CARTRIDGE_SIZE + 0x2000];
    } else if ((address >= 0xE000) && (address < 0xFE00)) {
      memoryController->memory[address - CARTRIDGE_SIZE - 0x2000];
    }
  }
}

MemoryController InitMBC1MemoryController(uint8_t* memory, uint8_t* cartridge)
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