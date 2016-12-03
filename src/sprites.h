#ifndef SPRITES_H_
#define SPRITES_H_

#include <stdint.h>


#define SPRITE_ATTR_BITS_OBJ_TO_BG_PRIORITY (1 << 7)
#define SPRITE_ATTR_BITS_Y_FLIP (1 << 6)
#define SPRITE_ATTR_BITS_X_FLIP (1 << 5)
#define SPRITE_ATTR_BITS_MONOCHROME_PALETTE_NUMBER (1 << 4)
#define SPRITE_ATTR_BITS_TILE_VRAM_BANK (1 << 3)
#define SPRITE_ATTR_BITS_COLOUR_PALETTE_NUMBER 3


typedef struct {
  uint8_t yPosition;
  uint8_t xPosition;
  uint8_t tileNumber;
  uint8_t attributes;
} Sprite;


int spritesCompareByXPosition(const void* a, const void* b);

#endif // SPRITES_H_
