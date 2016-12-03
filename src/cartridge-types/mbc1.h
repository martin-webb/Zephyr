#ifndef CARTRIDGE_TYPES_MBC1_H_
#define CARTRIDGE_TYPES_MBC1_H_

#include "../memorycontroller.h"

#include <stdint.h>

void mbc1InitialiseMemoryController(MemoryController* memoryController, uint32_t externalRAMSizeBytes, const char* romFilename, bool ram, bool battery);
void mbc1FinaliseMemoryController(MemoryController* memoryController);

#endif // CARTRIDGE_TYPES_MBC1_H_
