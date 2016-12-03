#include "mbc1.h"

#include "../battery.h"
#include "../logging.h"
#include "../memory.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
  uint8_t* externalRAM;
  uint32_t externalRAMSize;
  bool ramEnabled;

  uint8_t romBank;
  uint8_t bankSelect; // 2-bit register to select EITHER RAM Bank 00-03h or to specify the upper two bits (5 and 6, 0-based) of the ROM bank mapped to 0x4000-0x7FFF
  uint8_t modeSelect; // 1-bit register to select whether the above 2-bit register applies to ROM/RAM bank selection

  FILE* batteryFile;
} MBC1;

uint8_t mbc1ReadByte(MemoryController* memoryController, uint16_t address)
{
  MBC1* mbc1 = (MBC1*)memoryController->mbc;

  if (address <= 0x3FFF) { // Read from ROM Bank 0
    return memoryController->cartridge[address];
  } else if (address >= 0x4000 && address <= 0x7FFF) { // Read from ROM Banks 01-7F
    uint8_t bankNumber = ((mbc1->modeSelect == 0) ? (mbc1->bankSelect << 5) : 0) | mbc1->romBank;
    uint32_t romAddress = (bankNumber * 16 * 1024) + (address - 0x4000);
    return memoryController->cartridge[romAddress];
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Read from external cartridge RAM
    if (mbc1->ramEnabled) {
      uint8_t bankNumber = ((mbc1->modeSelect == 1) ? mbc1->bankSelect : 0);
      uint16_t ramAddress = (bankNumber * 8 * 1024) + (address - 0xA000);
      assert(ramAddress < mbc1->externalRAMSize);
      return mbc1->externalRAM[ramAddress];
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

  if (address <= 0x1FFF) { // External RAM enable/disable
    if ((value & 0xF) == 0xA) {
      if (!mbc1->ramEnabled) {
        info("External RAM was ENABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc1->ramEnabled = true;
    } else {
      if (mbc1->ramEnabled) {
        info("External RAM was DISABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc1->ramEnabled = false;
    }
  } else if (address >= 0x2000 && address <= 0x3FFF) { // ROM Bank Number
    if (value == 0) {
      value = 1;
    }
    mbc1->romBank = value & 0x1F; // Select the lower 5 bits of the value
  } else if (address >= 0x4000 && address <= 0x5FFF) { // RAM Bank Number or Upper Bits of ROM Bank Number
    mbc1->bankSelect = value & 3;
  } else if (address >= 0x6000 && address <= 0x7FFF) { // ROM/RAM Mode Select
    mbc1->modeSelect = value & 1;
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Write to external cartridge RAM
    if (mbc1->ramEnabled) {
      uint8_t bankNumber = ((mbc1->modeSelect == 1) ? mbc1->bankSelect : 0);
      uint16_t ramAddress = (bankNumber * 8 * 1024) + (address - 0xA000);
      assert(ramAddress < mbc1->externalRAMSize);
      mbc1->externalRAM[ramAddress] = value;
      if (mbc1->batteryFile != NULL) {
        batteryFileWriteByte(mbc1->batteryFile, ramAddress, value);
      }
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
  const char* romFilename,
  bool ram,
  bool battery
)
{
  MBC1* mbc1 = (MBC1*)malloc(sizeof(MBC1));
  assert(mbc1);

  mbc1->externalRAM = NULL;
  mbc1->externalRAMSize = externalRAMSizeBytes;
  mbc1->ramEnabled = false;
  mbc1->romBank = 1; // Initialise to 1 because writes of 0 are translated to a 1
  mbc1->bankSelect = 0;
  mbc1->modeSelect = 0;
  mbc1->batteryFile = NULL;

  if (ram) {
    mbc1->externalRAM = (uint8_t*)malloc(externalRAMSizeBytes * sizeof(uint8_t));
    assert(mbc1->externalRAM);
  }

  if (!ram && externalRAMSizeBytes > 0) {
    warning("\b[MBC1]: Cartridge declared %u bytes of external RAM but cartridge type did not indicate RAM.\n", externalRAMSizeBytes);
  } else if (ram && externalRAMSizeBytes == 0) {
    warning("\b[MBC1]: Cartridge type indicated RAM but cartridge declared %u bytes of external RAM.\n", externalRAMSizeBytes);
  }

  if (battery) {
    mbc1->batteryFile = batteryFileOpen(romFilename, mbc1->externalRAM, mbc1->externalRAMSize);
  }

  memoryController->readByteImpl = &mbc1ReadByte;
  memoryController->writeByteImpl = &mbc1WriteByte;
  memoryController->mbc = mbc1;
}

void mbc1FinaliseMemoryController(MemoryController* memoryController)
{
  MBC1* mbc1 = (MBC1*)memoryController->mbc;

  if (mbc1->batteryFile != NULL) {
    fclose(mbc1->batteryFile);
  }

  if (mbc1->externalRAM != NULL) {
    free(mbc1->externalRAM);
  }

  free(memoryController->mbc);
}
