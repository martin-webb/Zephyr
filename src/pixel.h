#ifndef PIXEL_H_
#define PIXEL_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t colour;
  bool bgPriority;
  float r;
  float g;
  float b;
} Pixel;

#endif // PIXEL_H_