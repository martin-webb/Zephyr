#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "memory.h"

#define CARTRIDGE_SIZE 0x8000

uint8_t readByte(MemoryController* memoryController, uint16_t address)
{
  return memoryController->readByteImpl(memoryController, address);
}

uint16_t readWord(MemoryController* memoryController, uint16_t address)
{
  uint8_t lsByte = memoryController->readByteImpl(memoryController, address);
  uint8_t msByte = memoryController->readByteImpl(memoryController, address + 1);
  return (msByte << 8) | lsByte;
}

void writeByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  memoryController->writeByteImpl(memoryController, address, value);
}

void writeWord(MemoryController* memoryController, uint16_t address, uint16_t value)
{
  memoryController->writeByteImpl(memoryController, address, value & 0x00FF);
  memoryController->writeByteImpl(memoryController, address + 1, (value & 0xFF00) >> 8);
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
      // return InitMBC3MemoryController(memory, cartridge);
      // TODO: REMOVE THIS HACK WHEN MBC3 IS IMPLEMENTED
      return InitROMOnlyMemoryController(memory, cartridge);
      break;
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

uint8_t ROMOnlyReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address < CARTRIDGE_SIZE) {
    return memoryController->cartridge[address];
  } else {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void ROMOnlyWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address < CARTRIDGE_SIZE) {
    printf("[FATAL]: Attempt to write to ROM space on ROM Only cartridge");
    exit(1);
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // Additional write to support the echo of 7.5KB of internal RAM
    if (address >= 0xC000 && address < 0xDE00) {
      memoryController->memory[address - CARTRIDGE_SIZE + 0x2000] = value;
    } else if (address >= 0xE000 && address < 0xFE00) {
      memoryController->memory[address - CARTRIDGE_SIZE - 0x2000] = value;
    }
  }
}

MemoryController InitROMOnlyMemoryController(uint8_t* memory, uint8_t* cartridge)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    0x00, // NOTE: Unused in ROM Only cartridges
    0x00, // NOTE: Unused in ROM Only cartridges
    0x01, // NOTE: Unused in ROM Only cartridges
    false, // NOTE: Unused in ROM Only cartridges
    &ROMOnlyReadByte,
    &ROMOnlyWriteByte
  };
  return memoryController;
}

/****************************************************************************/

uint8_t MBC1ReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address <= 0x3FFF) { // Read from ROM Bank 0
    return memoryController->cartridge[address];
  } else if (address >= 0x4000 && address <= 0x7FFF) { // Read from ROM Banks 01-7F
    uint8_t bankNumHigh = 0x00;
    if (memoryController->modeSelect == 0x00) {
      bankNumHigh = memoryController->bankSelect << 2;
    }
    uint8_t bankNum = bankNumHigh | memoryController->romBank;
    uint16_t offsetInBank = address - 0x4000;
    uint32_t romAddress = (bankNum * 1024 * 16) + offsetInBank;
  } else if (address >= 0xA000 && address <= 0xBFFF) { // External RAM Read
    // TODO: DO THIS
    printf("[WARNING]: MBC1ReadByte() - read from external RAM (0xA000-0xBFFF) at 0x%02X", address);
  } else { // Reads from remaining addresses are standard
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void MBC1WriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address <= 0x1FFF) { // External RAM Enable/Disable    
    if (value & 0xF == 0xA) {
      printf("[INFO]: External RAM was ENABLED by value 0x%02X", value);
    } else {
      printf("[INFO]: External RAM was DISABLED by value 0x%02X", value);
    }
  } else if (address >= 0x2000 && address <= 0x3FFF) { // ROM Bank Number
    if (value == 0x00) {
      value = 0x01;
    }
    memoryController->romBank = value & 0x1F; // Select the lower 5 bits of the value
    // TODO: Add cycle cost here??
  } else if (address >= 0x4000 && address <= 0x5FFF) { // RAM Bank Number or Upper Bits of ROM Bank Number
    memoryController->bankSelect = value & 0x02;
  } else if (address >= 0x6000 && address <= 0x7FFF) { // ROM/RAM Mode Select
    memoryController->modeSelect = (address & 0x1);
    // TODO: Add cycle cost here??
  } else if (address >= 0xA000 && address <= 0xBFFF) { // External RAM Write
    // TODO: DO THIS
    printf("[WARNING]: MBC1WriteByte() - write to external RAM (0xA000-0xBFFF) at 0x%02X (RAM Status: %s)", address, (memoryController->ramEnabled) ? "ENABLED" : "DISABLED");
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // Additional write to support the echo of 7.5KB of internal RAM
    if (address >= 0xC000 && address < 0xDE00) {
      memoryController->memory[address - CARTRIDGE_SIZE + 0x2000] = value;
    } else if (address >= 0xE000 && address < 0xFE00) {
      memoryController->memory[address - CARTRIDGE_SIZE - 0x2000] = value;
    }
  }
}

MemoryController InitMBC1MemoryController(uint8_t* memory, uint8_t* cartridge)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    0x00, // TODO: Correct default?
    0x00, // TODO: Correct default?
    0x01,
    false,
    &MBC1ReadByte,
    &MBC1WriteByte
  };
  return memoryController;
}