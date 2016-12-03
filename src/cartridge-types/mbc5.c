#include "mbc5.h"

#include "../battery.h"
#include "../logging.h"
#include "../memory.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
  uint8_t* externalRAM;
  uint32_t externalRAMSize;
  bool ramEnabled;

  uint8_t romBankLo;
  uint8_t romBankHi;
  uint8_t ramBank;

  FILE* batteryFile;
} MBC5;

uint8_t mbc5ReadByte(MemoryController* memoryController, uint16_t address)
{
  MBC5* mbc5 = (MBC5*)memoryController->mbc;

  if (address <= 0x3FFF) { // Read from ROM Bank 0
    return memoryController->cartridge[address];
  } else if (address >= 0x4000 && address <= 0x7FFF) { // Read from ROM Banks 0-1FF
    uint16_t bankNumber = (mbc5->romBankHi << 8) | mbc5->romBankLo;
    uint32_t romAddress = (bankNumber * 16 * 1024) + (address - 0x4000);
    return memoryController->cartridge[romAddress];
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Read from external cartridge RAM
    if (mbc5->ramEnabled) {
      uint16_t ramAddress = (mbc5->ramBank * 8 * 1024) + (address - 0xA000);
      assert(ramAddress < mbc5->externalRAMSize);
      return mbc5->externalRAM[ramAddress];
    } else {
      warning("MBC5: Read from external RAM at address 0x%04X failed because RAM is DISABLED.\n", address);
      return 0; // TODO: What value should be returned here?
    }
  } else {
    return commonReadByte(memoryController, address);
  }
}

void mbc5WriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  MBC5* mbc5 = (MBC5*)memoryController->mbc;

  if (address <= 0x1FFF) { // External RAM enable/disable
    if ((value & 0xF) == 0xA) {
      if (!mbc5->ramEnabled) {
        info("External RAM was ENABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc5->ramEnabled = true;
    } else {
      if (mbc5->ramEnabled) {
        info("External RAM was DISABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc5->ramEnabled = false;
    }
  } else if (address >= 0x2000 && address <= 0x2FFF) { // ROM Bank Number - Lower 8 Bits
    mbc5->romBankLo = value;
  } else if (address >= 0x3000 && address <= 0x3FFF) { // ROM Bank Number - 9th Bit
    mbc5->romBankHi = value & 1;
  } else if (address >= 0x4000 && address <= 0x5FFF) { // RAM Bank Number
    if (value <= 0x0F) {
      mbc5->ramBank = value & 3;
    }
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Write to external cartridge RAM
    if (mbc5->ramEnabled) {
      uint16_t ramAddress = (mbc5->ramBank * 8 * 1024) + (address - 0xA000);
      assert(ramAddress < mbc5->externalRAMSize);
      mbc5->externalRAM[ramAddress] = value;
      if (mbc5->batteryFile != NULL) {
        batteryFileWriteByte(mbc5->batteryFile, ramAddress, value);
      }
    } else {
      warning("MBC5: Write of value 0x%02X to external RAM at address 0x%04X failed because RAM is DISABLED.\n", value, address);
    }
  } else {
    commonWriteByte(memoryController, address, value);
  }
}

void mbc5InitialiseMemoryController(
  MemoryController* memoryController,
  uint32_t externalRAMSizeBytes,
  const char* romFilename,
  bool ram,
  bool battery
)
{
  MBC5* mbc5 = (MBC5*)malloc(sizeof(MBC5));
  assert(mbc5);

  mbc5->externalRAM = NULL;
  mbc5->externalRAMSize = externalRAMSizeBytes;
  mbc5->ramEnabled = false;
  mbc5->romBankLo = 0;
  mbc5->romBankHi = 0;
  mbc5->ramBank = 0;
  mbc5->batteryFile = NULL;

  if (ram) {
    mbc5->externalRAM = (uint8_t*)malloc(externalRAMSizeBytes * sizeof(uint8_t));
    assert(mbc5->externalRAM);
  }

  if (!ram && externalRAMSizeBytes > 0) {
    warning("\b[MBC5]: Cartridge declared %u bytes of external RAM but cartridge type did not indicate RAM.\n", externalRAMSizeBytes);
  } else if (ram && externalRAMSizeBytes == 0) {
    warning("\b[MBC5]: Cartridge type indicated RAM but cartridge declared %u bytes of external RAM.\n", externalRAMSizeBytes);
  }

  if (battery) {
    mbc5->batteryFile = batteryFileOpen(romFilename, mbc5->externalRAM, mbc5->externalRAMSize);
  }

  memoryController->readByteImpl = &mbc5ReadByte;
  memoryController->writeByteImpl = &mbc5WriteByte;
  memoryController->mbc = mbc5;
}

void mbc5FinaliseMemoryController(MemoryController* memoryController)
{
  MBC5* mbc5 = (MBC5*)memoryController->mbc;

  if (mbc5->batteryFile != NULL) {
    fclose(mbc5->batteryFile);
  }

  if (mbc5->externalRAM != NULL) {
    free(mbc5->externalRAM);
  }

  free(memoryController->mbc);
}
