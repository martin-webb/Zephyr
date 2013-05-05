#ifndef BATTERY_H_
#define BATTERY_H_

#include <stdint.h>

void batterySave(uint8_t* data, uint32_t size, const char* romFilename);
void batteryLoad(uint8_t* data, uint32_t size, const char* romFilename);

#endif // BATTERY_H_