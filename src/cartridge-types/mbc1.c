#include "mbc1.h"

#include "../logging.h"
#include "../memory.h"
#include "../ram.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
  uint8_t* externalRAM;
  uint32_t externalRAMSize;
  uint8_t bankSelect; // 2-bit register to select EITHER RAM Bank 00-03h or to specify the upper two bits (5 and 6, 0-based) of the ROM bank mapped to 0x4000-0x7FFF
  uint8_t modeSelect; // 1-bit register to select whether the above 2-bit applies to ROM/RAM bank selection
  uint8_t romBank;
  bool ramEnabled; // TODO: Store a bool value or the actual value written to 0x0000-0x1FFFF?
  const char* romFilename;
} MBC1;

uint8_t mbc1ReadByte(MemoryController* memoryController, uint16_t address)
{
  MBC1* mbc1 = (MBC1*)memoryController->mbc;

  if (address <= 0x3FFF) { // Read from ROM Bank 0
    return memoryController->cartridge[address];
  } else if (address >= 0x4000 && address <= 0x7FFF) { // Read from ROM Banks 01-7F
    uint8_t bankNumHigh = 0x00;
    if (mbc1->modeSelect == 0x00) {
      bankNumHigh = mbc1->bankSelect << 2;
    }
    uint8_t bankNum = bankNumHigh | mbc1->romBank;
    uint16_t offsetInBank = address - 0x4000;
    uint32_t romAddress = (bankNum * 1024 * 16) + offsetInBank;
    return memoryController->cartridge[romAddress];
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Read from external cartridge RAM
    if (mbc1->ramEnabled) {
      return mbc1->externalRAM[address - 0xA000];
    } else {
      warning("MBC1: Read from external RAM at address 0x%04X failed because RAM is DISABLED.\n", address);
      return 0; // TODO: What value should be returned here?
    }
  } else {
    return commonReadByte(memoryController, address);
  }
}

void mbc1WriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  MBC1* mbc1 = (MBC1*)memoryController->mbc;

  if (address <= 0x1FFF) { // External RAM Enable/Disable
    if ((value & 0xF) == 0xA) {
      if (!mbc1->ramEnabled) {
        ramLoad(mbc1->externalRAM, mbc1->externalRAMSize, mbc1->romFilename);
        info("External RAM was ENABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc1->ramEnabled = true;
    } else {
      if (mbc1->ramEnabled) {
        ramSave(mbc1->externalRAM, mbc1->externalRAMSize, mbc1->romFilename);
        info("External RAM was DISABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc1->ramEnabled = false;
    }
  } else if (address >= 0x2000 && address <= 0x3FFF) { // ROM Bank Number
    if (value == 0x00) {
      value = 0x01;
    }
    mbc1->romBank = value & 0x1F; // Select the lower 5 bits of the value
    // TODO: Add cycle cost here??
  } else if (address >= 0x4000 && address <= 0x5FFF) { // RAM Bank Number or Upper Bits of ROM Bank Number
    mbc1->bankSelect = value & 0x02;
  } else if (address >= 0x6000 && address <= 0x7FFF) { // ROM/RAM Mode Select
    mbc1->modeSelect = (address & 0x1);
    // TODO: Add cycle cost here??
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Write to external cartridge RAM
    if (mbc1->ramEnabled) {
      mbc1->externalRAM[address - 0xA000] = value;
    } else {
      warning("MBC1: Write of value 0x%02X to external RAM at address 0x%04X failed because RAM is DISABLED.\n", value, address);
    }
  } else {
    commonWriteByte(memoryController, address, value);
  }
}

void mbc1InitialiseMemoryController(
  MemoryController* memoryController,
  uint32_t externalRAMSizeBytes,
  const char* romFilename
)
{
  MBC1* mbc1 = (MBC1*)malloc(sizeof(MBC1));
  assert(mbc1);

  uint8_t* externalRAM = (uint8_t*)malloc(externalRAMSizeBytes * sizeof(uint8_t));
  assert(externalRAM);

  mbc1->externalRAM = externalRAM;
  mbc1->externalRAMSize = externalRAMSizeBytes;
  mbc1->bankSelect = 0;
  mbc1->modeSelect = 0;
  mbc1->romBank = 1;
  mbc1->ramEnabled = false;
  mbc1->romFilename = romFilename;

  memoryController->readByteImpl = &mbc1ReadByte;
  memoryController->writeByteImpl = &mbc1WriteByte;
  memoryController->mbc = mbc1;
}

void mbc1FinaliseMemoryController(MemoryController* memoryController)
{
  MBC1* mbc1 = (MBC1*)memoryController->mbc;

  free(mbc1->externalRAM);
  free(memoryController->mbc);
}