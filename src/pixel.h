#ifndef PIXEL_H_
#define PIXEL_H_

#include <stdint.h>

typedef struct {
  uint8_t colour;
  float r;
  float g;
  float b;
} Pixel;

#endif // PIXEL_H_