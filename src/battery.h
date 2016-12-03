#ifndef BATTERY_H_
#define BATTERY_H_

#include <stdint.h>
#include <stdio.h>


FILE* batteryFileOpen(const char* romFilename, uint8_t* data, uint32_t size);
void batteryFileWriteByte(FILE* saveFile, uint16_t address, uint8_t value);

#endif // BATTERY_H_
