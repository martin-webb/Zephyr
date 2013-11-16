#include "sprites.h"

int spritesCompareByXPosition(const void* a, const void* b)
{
  const Sprite* spriteA = (const Sprite*)a;
  const Sprite* spriteB = (const Sprite*)b;

  if (spriteA->xPosition != spriteB->xPosition) {
    return (spriteA->xPosition < spriteB->xPosition) ? -1 : 1;
  } else {
    return a < b; // Comparing by pointer values here is functionally equivalent to comparing by "table ordering" e.g FE00h - highest, FE04h - next highest, etc.
  }
}