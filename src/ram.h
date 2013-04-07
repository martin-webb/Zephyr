#ifndef RAM_H_
#define RAM_H_

#include <stdint.h>

void ramSave(uint8_t* ram, uint32_t size, const char* romFilename);
void ramLoad(uint8_t* ram, uint32_t size, const char* romFilename);

#endif // RAM_H_