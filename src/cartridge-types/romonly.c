#include "romonly.h"

#include "../cartridge.h"
#include "../logging.h"
#include "../memory.h"


uint8_t romOnlyReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address < CARTRIDGE_SIZE) {
    return memoryController->cartridge[address];
  } else {
    return commonReadByte(memoryController, address);
  }
}


void romOnlyWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address < CARTRIDGE_SIZE) {
    warning("Write of value 0x%02X to address 0x%04X in ROM space (0x%04X-0x%04X) on ROM Only cartridge\n", value, address, 0, CARTRIDGE_SIZE);
  } else {
    commonWriteByte(memoryController, address, value);
  }
}


void romOnlyInitialiseMemoryController(MemoryController* memoryController)
{
  memoryController->readByteImpl = &romOnlyReadByte;
  memoryController->writeByteImpl = &romOnlyWriteByte;
}
