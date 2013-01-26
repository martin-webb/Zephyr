#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "memory.h"
#include "timer.h"

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

void incrementByte(MemoryController* memoryController, uint16_t address)
{
  if (address < CARTRIDGE_SIZE) {
    memoryController->cartridge[address]++;
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE]++;
  }
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
      fprintf(stderr, "[FATAL]: Error in %s - cartridge type UNSUPPORTED\n", __func__);
      exit(EXIT_FAILURE);
      break;
    default:
      fprintf(stderr, "[FATAL]: Error in %s - cartridge type UNKNOWN\n", __func__);
      exit(EXIT_FAILURE);
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
    printf("[WARNING]: Write of value 0x%02X to address 0x%04X in ROM space (0x%04X-0x%04X) on ROM Only cartridge\n", value, address, 0, CARTRIDGE_SIZE);
  } else if (address == IO_REG_ADDRESS_DIV) {
    memoryController->memory[address - CARTRIDGE_SIZE] = 0x00; // Writes to DIV are always 0, regardless of the actual value
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // TODO: Add to formal logging strategy that can be removed for release builds (for example)
    if (address >= 0xFF00 && address < 0xFF4C) {
      printf("[MEMORYLOG]: Write to I/O reg - address=0x%04X value=0x%02X\n", address, value);
    }
    
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
    return memoryController->cartridge[romAddress];
  } else if (address >= 0xA000 && address <= 0xBFFF) { // External RAM Read
    // TODO: DO THIS
    printf("[WARNING]: %s - read from external RAM (0xA000-0xBFFF) at 0x%04X\n", __func__, address);
    return 0; // TODO: Remove this with full implementation
  } else { // Reads from remaining addresses are standard
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void MBC1WriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address <= 0x1FFF) { // External RAM Enable/Disable    
    if ((value & 0xF) == 0xA) {
      printf("[INFO]: External RAM was ENABLED by value 0x%02X written to address 0x%04X\n", value, address);
    } else {
      printf("[INFO]: External RAM was DISABLED by value 0x%02X written to address 0x%04X\n", value, address);
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
    printf("[WARNING]: %s - write of value 0x%02X to external RAM (0xA000-0xBFFF) at address 0x%04X (RAM Status: %s)\n", __func__, value, address, (memoryController->ramEnabled) ? "ENABLED" : "DISABLED");
  } else if (address == IO_REG_ADDRESS_DIV) {
    memoryController->memory[address - CARTRIDGE_SIZE] = 0x00; // Writes to DIV are always 0, regardless of the actual value
  } else {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    
    // TODO: Add to formal logging strategy that can be removed for release builds (for example)
    if (address >= 0xFF00 && address < 0xFF4C) {
      printf("[MEMORYLOG]: Write to I/O reg - address=0x%04X value=0x%02X\n", address, value);
    }
    
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