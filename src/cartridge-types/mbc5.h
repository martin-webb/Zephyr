#ifndef CARTRIDGE_TYPES_MBC5_H_
#define CARTRIDGE_TYPES_MBC5_H_

#include "../memorycontroller.h"

#include <stdint.h>


void mbc5InitialiseMemoryController(MemoryController* memoryController, uint32_t externalRAMSizeBytes, const char* romFilename, bool ram, bool battery);
void mbc5FinaliseMemoryController(MemoryController* memoryController);

#endif // CARTRIDGE_TYPES_MBC5_H_
