#ifndef CARTRIDGE_TYPES_MBC3_H_
#define CARTRIDGE_TYPES_MBC3_H_

#include "../memorycontroller.h"

#include <stdint.h>

void mbc3InitialiseMemoryController(MemoryController* memoryController, uint32_t externalRAMSizeBytes, const char* romFilename, bool ram, bool timer, bool battery);
void mbc3FinaliseMemoryController(MemoryController* memoryController);

#endif // CARTRIDGE_TYPES_MBC3_H_
