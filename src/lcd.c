#include "lcd.h"

#include "cpu.h"
#include "logging.h"
#include "sprites.h"

#include <stdlib.h>


#define BACKGROUND_WIDTH 256
#define BACKGROUND_HEIGHT 256

#define LOW_2_BITS 3
#define LOW_3_BITS 7
#define LOW_5_BITS 31


typedef struct
{
  int x;
  int y;
} Point;


typedef struct
{
  int low;
  int high;
} TileLineData;


typedef struct
{
  int r;
  int g;
  int b;
} Colour;


typedef struct
{
  int paletteNum;
  int vramBankNum;
  bool horizontalFlip;
  bool verticalFlip;
  bool bgPriority;
} BGMapAttributes;


static const BGMapAttributes BG_MAP_ATTRIBUTES_DEFAULT =
{
  .paletteNum = 0,
  .vramBankNum = 0,
  .horizontalFlip = false,
  .verticalFlip = false,
  .bgPriority = false
};


static TileLineData getTileLineData(LCDController* lcdController, uint16_t address)
{
  TileLineData data = {
    .low = lcdController->vram[address],
    .high = lcdController->vram[address + 1]
  };
  return data;
}


void initLCDController(LCDController* lcdController, InterruptController* interruptController, uint8_t* vram, uint8_t* oam, Pixel* frameBuffer, GameBoyType gameBoyType, CGBMode cgbMode)
{
  lcdController->gameBoyType = gameBoyType;
  lcdController->cgbMode = cgbMode;
  lcdController->stat = 0;
  lcdController->vbk = 0;
  lcdController->vram = vram;
  lcdController->oam = oam;
  lcdController->frameBuffer = frameBuffer;
  lcdController->clockCycles = 0;
  lcdController->vblankCounter = 0;
  lcdController->interruptController = interruptController;
}


uint8_t lcdReadByte(LCDController* lcdController, uint16_t address)
{
  if (address == IO_REG_ADDRESS_LCDC) { // 0xFF40
    return lcdController->lcdc;
  } else if (address == IO_REG_ADDRESS_STAT) { // 0xFF41
    return lcdController->stat;
  } else if (address == IO_REG_ADDRESS_SCY) { // 0xFF42
    return lcdController->scy;
  } else if (address == IO_REG_ADDRESS_SCX) { // 0xFF43
    return lcdController->scx;
  } else if (address == IO_REG_ADDRESS_LY) { // 0xFF44
    return lcdController->ly;
  } else if (address == IO_REG_ADDRESS_LYC) { // 0xFF45
    return lcdController->lyc;
  } else if (address == IO_REG_ADDRESS_BGP) { // 0xFF47
    return lcdController->bgp;
  } else if (address == IO_REG_ADDRESS_OBP0) { // 0xFF48
    return lcdController->obp0;
  } else if (address == IO_REG_ADDRESS_OBP1) { // 0xFF49
    return lcdController->obp1;
  } else if (address == IO_REG_ADDRESS_WY) { // 0xFF4A
    return lcdController->wy;
  } else if (address == IO_REG_ADDRESS_WX) { // 0xFF4B
    return lcdController->wx;
  } else if (address == IO_REG_ADDRESS_VBK) { // 0xFF4F
    return lcdController->vbk;
  } else if (address == IO_REG_ADDRESS_BCPS) { // 0xFF68
    return lcdController->bcps;
  } else if (address == IO_REG_ADDRESS_BCPD) { // 0xFF69
    return lcdController->backgroundPaletteMemory[lcdController->bcps & 0x3F];
  } else if (address == IO_REG_ADDRESS_OCPS) { // 0xFF6A
    return lcdController->ocps;
  } else if (address == IO_REG_ADDRESS_OCPD) { // 0xFF6B
    return lcdController->objectPaletteMemory[lcdController->ocps & 0x3F];
  } else {
    return 0x00;
  }
}


void lcdWriteByte(LCDController* lcdController, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_LCDC) { // 0xFF40
    bool lcdEnabledBefore = lcdController->lcdc & LCD_DISPLAY_ENABLE_BIT;
    bool lcdEnabledAfter = value & LCD_DISPLAY_ENABLE_BIT;

    lcdController->lcdc = value;

    if (!lcdEnabledBefore && lcdEnabledAfter) {
      debug("LCD was ENABLED by write of value 0x%02X to LCDC\n", value);
    } else if (lcdEnabledBefore && !lcdEnabledAfter) {
      // Stopping LCD operation outside of the VBLANK period could damage the display hardware.
      // Nintendo is reported to reject any games that didn't follow this rule.
      uint8_t lcdMode = lcdController->stat & STAT_MODE_FLAG_BITS;
      if (lcdMode != 1) {
        warning("LCD was DISABLED outside of VBLANK period (clocks=%u)!\n", lcdController->clockCycles);
        // exit(EXIT_FAILURE);
      }
      debug("LCD was DISABLED by write of value 0x%02X to LCDC\n", value);
      // lcdController->clockCycles = 0;
    }
  } else if (address == IO_REG_ADDRESS_STAT) { // 0xFF41
    lcdController->stat = (value & 0xFC) | (lcdController->stat & 0x03);
  } else if (address == IO_REG_ADDRESS_SCY) { // 0xFF42
    lcdController->scy = value;
  } else if (address == IO_REG_ADDRESS_SCX) { // 0xFF43
    lcdController->scx = value;
  } else if (address == IO_REG_ADDRESS_LY) { // 0xFF44
    lcdController->ly = 0;
    debug("[MEMORY LOG]: WRITE TO LY!\n");
  } else if (address == IO_REG_ADDRESS_LYC) { // 0xFF45
    lcdController->lyc = value;
  } else if (address == IO_REG_ADDRESS_BGP) { // 0xFF47
    lcdController->bgp = value;
  } else if (address == IO_REG_ADDRESS_OBP0) { // 0xFF48
    lcdController->obp0 = value;
  } else if (address == IO_REG_ADDRESS_OBP1) { // 0xFF49
    lcdController->obp1 = value;
  } else if (address == IO_REG_ADDRESS_WY) { // 0xFF4A
    lcdController->wy = value;
  } else if (address == IO_REG_ADDRESS_WX) { // 0xFF4B
    lcdController->wx = value;
  } else if (address == IO_REG_ADDRESS_VBK) { // 0xFF4F
    lcdController->vbk = value & 1;
  } else if (address == IO_REG_ADDRESS_BCPS) { // 0xFF68
    lcdController->bcps = value;
  } else if (address == IO_REG_ADDRESS_BCPD) { // 0xFF69
    uint8_t bcps = lcdController->bcps;
    lcdController->backgroundPaletteMemory[bcps & 0x3F] = value;
    if (bcps & (1 << 7)) {
      lcdController->bcps = (bcps & 0x80) | (((bcps & 0x3F) + 1) & 0x3F);
    }
  } else if (address == IO_REG_ADDRESS_OCPS) { // 0xFF6A
    lcdController->ocps = value;
  } else if (address == IO_REG_ADDRESS_OCPD) { // 0xFF6B
    uint8_t ocps = lcdController->ocps;
    lcdController->objectPaletteMemory[ocps & 0x3F] = value;
    if (ocps & (1 << 7)) {
      lcdController->ocps = (ocps & 0x80) | (((ocps & 0x3F) + 1) & 0x3F);
    }
  }
}


static bool lcdIsEnabled(LCDController* lcdController)
{
  return lcdController->lcdc & LCD_DISPLAY_ENABLE_BIT;
}


static void lcdStatSetMode(LCDController* lcdController, uint8_t mode)
{
  lcdController->stat = (lcdController->stat & 0xFC) | mode;
}


static uint8_t pixelColour(TileLineData lineData, uint8_t pixelX)
{
  switch (pixelX) {
    // High bit shifts for pixel colour extraction:
    //   Pixels 0 - 5: Right by (6 - Pixel#)
    //   Pixel 6: No Shift
    //   Pixel 7: Left by 1
    // Low bit shifts for pixel colour extraction:
    //   Pixels 0 - 6: Right by (7 - Pixel#)
    //   Pixel 7: No Shift
    case 7:
      return ((lineData.high << 1) & 2) | (lineData.low & 1);
      break;
    case 6:
      return ((lineData.high) & 2) | ((lineData.low >> (7 - pixelX)) & 1);
      break;
    default: // Pixels 0 - 5
      return ((lineData.high >> (6 - pixelX)) & 2) | ((lineData.low >> (7 - pixelX)) & 1);
      break;
  }
}


static void setPixelRGBForMonochromeShade(LCDController* lcdController, Pixel* pixel, uint8_t shade)
{
  switch (shade) {
    case 0:
      pixel->r = pixel->g = pixel->b = 1.0;
      break;
    case 1:
      pixel->r = pixel->g = pixel->b = 0.66;
      break;
    case 2:
      pixel->r = pixel->g = pixel->b = 0.33;
      break;
    case 3:
      pixel->r = pixel->g = pixel->b = 0.0;
      break;
  }
}


static uint8_t colourToBackgroundMonochromeShade(LCDController* lcdController, uint8_t colour)
{
  return (lcdController->bgp >> (colour * 2)) & LOW_2_BITS;
}


static uint8_t colourToObjectMonochromeShade(LCDController* lcdController, uint8_t colour, uint8_t palette)
{
  return (palette >> (colour * 2)) & 3;
}


static uint8_t extract5BitRedValue(uint8_t colourByteLow)
{
  return colourByteLow & LOW_5_BITS;
}


static uint8_t extract5BitGreenValue(uint8_t colourByteLow, int8_t colourByteHigh)
{
  return ((colourByteHigh & 3) << LOW_2_BITS) | ((colourByteLow >> 5) & LOW_3_BITS);
}


static uint8_t extract5BitBlueValue(uint8_t colourByteHigh)
{
  return (colourByteHigh >> 2) & LOW_5_BITS;
}


static Colour getBackgroundColour(LCDController* lcdController, uint8_t paletteNum, uint8_t colourNum)
{
  Colour colour;

  uint8_t paletteIndex = (paletteNum * 8) + (colourNum * 2);
  uint8_t colourByteLow = lcdController->backgroundPaletteMemory[paletteIndex];
  uint8_t colourByteHigh = lcdController->backgroundPaletteMemory[paletteIndex + 1];

  colour.r = extract5BitRedValue(colourByteLow);
  colour.g = extract5BitGreenValue(colourByteLow, colourByteHigh);
  colour.b = extract5BitBlueValue(colourByteHigh);

  return colour;
}


static Colour getObjectColour(LCDController* lcdController, uint8_t paletteNum, uint8_t colourNum)
{
  Colour colour;

  uint8_t paletteIndex = (paletteNum * 8) + (colourNum * 2);
  uint8_t colourByteLow = lcdController->objectPaletteMemory[paletteIndex];
  uint8_t colourByteHigh = lcdController->objectPaletteMemory[paletteIndex + 1];

  colour.r = extract5BitRedValue(colourByteLow);
  colour.g = extract5BitGreenValue(colourByteLow, colourByteHigh);
  colour.b = extract5BitBlueValue(colourByteHigh);

  return colour;
}


static float fiveBitToFloat(uint8_t value)
{
  return value / 31.0;
}


static void setPixelRGBForCGBColour(LCDController* lcdController, Pixel* pixel, Colour colour)
{
  pixel->r = fiveBitToFloat(colour.r);
  pixel->g = fiveBitToFloat(colour.g);
  pixel->b = fiveBitToFloat(colour.b);
}


static Pixel* getFramebufferPixel(LCDController* lcdController, int16_t scanlineX)
{
  return &lcdController->frameBuffer[lcdController->ly * LCD_WIDTH + scanlineX];
}


static uint16_t getBackgroundTileMapAddress(LCDController* lcdController)
{
  return ((lcdController->lcdc & LCD_BG_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x9C00 : 0x9800);
}


static uint16_t getWindowTileMapAddress(LCDController* lcdController)
{
  return ((lcdController->lcdc & LCD_WINDOW_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x9C00 : 0x9800);
}


static uint16_t getTileDataAddress(LCDController* lcdController)
{
  return ((lcdController->lcdc & LCD_BG_AND_WINDOW_TILE_DATA_SELECT_BIT) ? 0x8000 : 0x8800);
}


static void setBGMapAttributes(LCDController* lcdController, BGMapAttributes* attributes, uint16_t backgroundMapTileIndex)
{
  uint8_t attributesByte = lcdController->vram[((lcdController->lcdc & LCD_BG_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x3C00 : 0x3800) + backgroundMapTileIndex];

  attributes->paletteNum = attributesByte & 7;
  attributes->vramBankNum = (attributesByte >> 3) & 1;
  attributes->horizontalFlip = (attributesByte & (1 << 5));
  attributes->verticalFlip = (attributesByte & (1 << 6));
  attributes->bgPriority = (attributesByte & (1 << 7));
}


static void setWindowMapAttributes(LCDController* lcdController, BGMapAttributes* attributes, uint16_t windowMapTileIndex)
{
  uint8_t attributesByte = lcdController->vram[((lcdController->lcdc & LCD_WINDOW_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x3C00 : 0x3800) + windowMapTileIndex];

  attributes->paletteNum = attributesByte & 7;
  attributes->vramBankNum = (attributesByte >> 3) & 1;
  attributes->horizontalFlip = (attributesByte & (1 << 5));
  attributes->verticalFlip = (attributesByte & (1 << 6));
  attributes->bgPriority = (attributesByte & (1 << 7));
}


static Point getPositionInBackground(LCDController* lcdController, int scanlineX)
{
  Point position = {
    .x = (scanlineX + lcdController->scx) % BACKGROUND_WIDTH,
    .y = (lcdController->ly + lcdController->scy) % BACKGROUND_HEIGHT
  };
  return position;
}


static uint16_t positionToMapTileIndex(Point point)
{
  return ((point.y / 8) * 32) + (point.x / 8);
}


static void lcdDrawScanlineBackground(LCDController* lcdController)
{
  if (lcdController->lcdc & LCD_BG_DISPLAY_BIT) {
    uint16_t tileMapAddress = getBackgroundTileMapAddress(lcdController);
    uint16_t tileDataAddress = getTileDataAddress(lcdController);

    for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
      Point positionInBackground = getPositionInBackground(lcdController, scanlineX);

      uint16_t mapTileIndex = positionToMapTileIndex(positionInBackground);

      uint16_t backgroundMapTileOffset = lcdController->vram[tileMapAddress - 0x8000 + mapTileIndex];

      uint16_t backgroundTileDataAddress = tileDataAddress - 0x8000;
      if (tileDataAddress == 0x8000) {
        backgroundTileDataAddress += backgroundMapTileOffset * 16;
      } else {
        backgroundTileDataAddress = 0x9000 - 0x8000 + (int8_t)backgroundMapTileOffset * 16;
      }

      BGMapAttributes attributes = BG_MAP_ATTRIBUTES_DEFAULT;
      if (lcdController->cgbMode == COLOUR) {
        setBGMapAttributes(lcdController, &attributes, mapTileIndex);
      }

      uint16_t lineOffset = backgroundTileDataAddress + ((attributes.verticalFlip) ? (14 - ((positionInBackground.y % 8) * 2)) : ((positionInBackground.y % 8) * 2));

      if (attributes.vramBankNum == 1) {
        lineOffset += (1024 * 8);
      }

      TileLineData lineData = getTileLineData(lcdController, lineOffset);

      // Draw all pixels from the current tile, starting at the offset in the tile determined by the x location in the complete background
      if (lcdController->cgbMode == MONOCHROME) {
        for (uint8_t pixelX = positionInBackground.x % 8; pixelX < 8 && scanlineX < LCD_WIDTH; pixelX++) {
          Pixel* pixel = getFramebufferPixel(lcdController, scanlineX);

          pixel->colour = pixelColour(lineData, pixelX);
          uint8_t shade = colourToBackgroundMonochromeShade(lcdController, pixel->colour);
          setPixelRGBForMonochromeShade(lcdController, pixel, shade);

          // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
          if (pixelX != 7) {
            scanlineX++;
          }
        }
      } else if (lcdController->cgbMode == COLOUR) {
        for (uint8_t pixelX = positionInBackground.x % 8; pixelX < 8 && scanlineX < LCD_WIDTH; pixelX++) {
          Pixel* pixel = getFramebufferPixel(lcdController, scanlineX);

          uint8_t lineX = (attributes.horizontalFlip ? 7 - pixelX : pixelX);

          pixel->colour = pixelColour(lineData, lineX);
          pixel->bgPriority = attributes.bgPriority;

          Colour colour = getBackgroundColour(lcdController, attributes.paletteNum, pixel->colour);
          setPixelRGBForCGBColour(lcdController, pixel, colour);

          // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
          if (pixelX != 7) {
            scanlineX++;
          }
        }
      }
    }
  } else {
    // The background is disabled so with a monochrome Game Boy every pixel is white
    for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
      Pixel* pixel = getFramebufferPixel(lcdController, scanlineX);
      setPixelRGBForMonochromeShade(lcdController, pixel, 0);
    }
  }
}


static void lcdDrawScanlineWindow(LCDController* lcdController)
{
  // Check if the window offset results in a row of the window on the current scanline - if not then we're done
  if (lcdController->ly < lcdController->wy) {
    return;
  }

  uint16_t tileMapAddress = getWindowTileMapAddress(lcdController);
  uint16_t tileDataAddress = getTileDataAddress(lcdController);

  for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
    // Determine map tile for pixel based on the LCD controller LY, scanline X and WX and WY window offsets
    uint8_t windowY = lcdController->ly - lcdController->wy;

    // Check if the window offset results in a column of the window in the current column of the scanline
    if (scanlineX < (lcdController->wx - 7)) {
      continue;
    }

    uint8_t windowX = scanlineX - (lcdController->wx - 7);

    uint16_t windowMapTileIndex = ((windowY / 8) * 32) + (windowX / 8);
    uint16_t windowMapTileOffset = lcdController->vram[tileMapAddress - 0x8000 + windowMapTileIndex];

    uint16_t windowTileDataAddress = tileDataAddress - 0x8000;
    if (tileDataAddress == 0x8000) {
      windowTileDataAddress += windowMapTileOffset * 16;
    } else {
      windowTileDataAddress = 0x9000 - 0x8000 + (int8_t)windowMapTileOffset * 16;
    }

    BGMapAttributes attributes = BG_MAP_ATTRIBUTES_DEFAULT;
    if (lcdController->cgbMode == COLOUR) {
      setWindowMapAttributes(lcdController, &attributes, windowMapTileIndex);
    }

    uint16_t lineOffset = windowTileDataAddress + ((windowY % 8) * 2);

    if (attributes.vramBankNum == 1) {
      lineOffset += (1024 * 8);
    }

    TileLineData lineData = getTileLineData(lcdController, lineOffset);

    // Draw all pixels from the current tile, starting at the offset in the tile determined by the x location in the complete background
    if (lcdController->cgbMode == MONOCHROME) {
      for (uint8_t pixelX = windowX % 8; pixelX < 8 && scanlineX < LCD_WIDTH; pixelX++) {
        Pixel* pixel = getFramebufferPixel(lcdController, scanlineX);
        pixel->colour = pixelColour(lineData, pixelX);
        uint8_t shade = colourToBackgroundMonochromeShade(lcdController, pixel->colour);

        setPixelRGBForMonochromeShade(lcdController, pixel, shade);

        // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
        if (pixelX != 7) {
          scanlineX++;
        }
      }
    } else if (lcdController->cgbMode == COLOUR) {
      for (uint8_t pixelX = windowX % 8; pixelX < 8 && scanlineX < LCD_WIDTH; pixelX++) {
        Pixel* pixel = getFramebufferPixel(lcdController, scanlineX);

        uint8_t lineX = (attributes.horizontalFlip ? 7 - pixelX : pixelX);

        pixel->colour = pixelColour(lineData, lineX);
        pixel->bgPriority = attributes.bgPriority;

        Colour colour = getBackgroundColour(lcdController, attributes.paletteNum, pixel->colour);
        setPixelRGBForCGBColour(lcdController, pixel, colour);

        // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
        if (pixelX != 7) {
          scanlineX++;
        }
      }
    }
  }
}


static uint8_t lcdCopySpritesVisibleInScanline(LCDController* lcdController, Sprite* sprites, uint8_t spriteHeight)
{
  uint8_t spriteCount = 0;
  for (uint8_t spriteIndex = 0; spriteIndex < MAX_SPRITES; spriteIndex++) {
    // Does the sprite's Y position exclude it from appearing in the scanline?
    // TODO: Check that "spriteY == 0 ||" and "|| spriteY >= 160" are implied here?
    uint8_t spriteY = lcdController->oam[spriteIndex * 4];
    if (spriteHeight == 8) {
      if (spriteY < (lcdController->ly + 8 + 1) || spriteY > (lcdController->ly + 16)) {
        continue;
      }
    } else {
      if (spriteY < (lcdController->ly + 1) || spriteY > (lcdController->ly + spriteHeight)) {
        continue;
      }
    }

    // Does the sprite's X position exclude it from appearing in the scanline?
    // NOTE: No checks possible for overlapping of sprites at this point because we cover all columns in the scanline
    uint8_t spriteX = lcdController->oam[spriteIndex * 4 + 1];
    if (spriteX == 0 || spriteX >= 168) {
      continue;
    }

    // If we get here the sprite should be drawn, but we can't determine the draw priorities until
    // we have scanned all the sprites in OAM, so store the sprite data for later.
    sprites[spriteCount].yPosition = spriteY;
    sprites[spriteCount].xPosition = spriteX;
    sprites[spriteCount].tileNumber = lcdController->oam[spriteIndex * 4 + 2];
    sprites[spriteCount].attributes = lcdController->oam[spriteIndex * 4 + 3];

    spriteCount++;
  }
  return spriteCount;
}


static bool bgAndWinMasterPriority(LCDController* lcdController)
{
  return (lcdController->lcdc & 1);
}


static bool pixelBGPriority(Pixel* pixel)
{
  return pixel->bgPriority;
}


static bool spriteBGPriority(Sprite* sprite)
{
  return (sprite->attributes & SPRITE_ATTR_BITS_OBJ_TO_BG_PRIORITY);
}


static bool spritePixelVisibleInMonochromeMode(uint8_t spriteColourNumber, uint8_t bgOrWinColour, Pixel* pixel, Sprite* sprite)
{
  return (spriteColourNumber != 0) &&
    ((bgOrWinColour == 0) || (!pixelBGPriority(pixel) && !spriteBGPriority(sprite)));
}


static bool spritePixelVisibleInColourMode(LCDController* lcdController, uint8_t spriteColourNumber, uint8_t bgOrWinColour, Pixel* pixel, Sprite* sprite)
{
  return (spriteColourNumber != 0) &&
    ((bgOrWinColour == 0) || (!bgAndWinMasterPriority(lcdController)) || (!pixelBGPriority(pixel) && !spriteBGPriority(sprite)));
}


static void lcdDrawScanlineObjects(LCDController* lcdController)
{
  Sprite sprites[MAX_SPRITES];

  const uint8_t spriteHeight = ((lcdController->lcdc & LCD_OBJ_SIZE_BIT) ? 16 : 8);

  // Fetch sprites that we know to be in the visible scanline
  const uint8_t spriteCount = lcdCopySpritesVisibleInScanline(lcdController, sprites, spriteHeight);

  // For non CGB mode, order sprites that are in the scanline first by X position and then by order in the sprite table
  if (lcdController->cgbMode == MONOCHROME) {
    qsort((void*)sprites, spriteCount, sizeof(Sprite), spritesCompareByXPosition);
  }

  // Limit the number of sprites drawn
  uint8_t spritesToRender = spriteCount;
  if (spritesToRender > MAX_SPRITES_PER_LINE) {
    spritesToRender = MAX_SPRITES_PER_LINE;
  }

  // Now that we know how many sprites we're drawing we can adjust the Mode 3 timing accordingly
  lcdController->mode3Cycles = MODE_3_CYCLES_MIN + (spritesToRender * MODE_3_CYCLES_PER_SPRITE);

  // Draw sprites from least to highest priority so higher priority sprites will be drawn over lower priority sprites
  // TODO: This could be improved by moving across the scanline from left to right and not drawing
  // overlapped parts of individual sprites and expecting them to be covered by drawing from right to left.
  for (int sp = spritesToRender - 1; sp >= 0; sp--) {
    Sprite sprite = sprites[sp];

    uint8_t lineNumInSprite = lcdController->ly - sprite.yPosition + 16;

    // All sprites are 16 pixels high but in 8x8 mode we don't draw the bottom half (however the positioning is always done in terms of 16 pixels height)
    // TODO: Is this check needed? lcdCopySpritesVisibleInScanline() should filter out sprites that shouldn't be shown.
    if (lineNumInSprite > spriteHeight - 1) {
      continue;
    }

    // Y flip
    // TODO: For 8x16 tiles, does the flip affect each 8x8 individually or the 8x16 tile as a whole?
    uint8_t lineNumAfterYFlip = lineNumInSprite;
    if (sprite.attributes & SPRITE_ATTR_BITS_Y_FLIP) {
      lineNumAfterYFlip = (spriteHeight - 1) - lineNumInSprite;
    }

    uint16_t lineOffset = ((spriteHeight == 8) ? sprite.tileNumber : (sprite.tileNumber & 0xFE)) * 16 + (lineNumAfterYFlip * 2);

    if (lcdController->cgbMode == COLOUR) {
      if (sprite.attributes & (1 << 3)) {
        lineOffset += (1024 * 8);
      }
    }

    TileLineData lineData = getTileLineData(lcdController, lineOffset);

    for (uint8_t lineX = 0; lineX < 8; lineX++) {
      // Stop sprite pixels from wrapping back around onto the end of the previous scanline
      if (((sprite.xPosition - 8) + lineX) < 0) {
        continue;
      }

      // Stop sprite pixels from wrapping around onto the start of the next scanline
      if (((sprite.xPosition - 8) + lineX) >= LCD_WIDTH) {
        break;
      }

      uint8_t realX = lineX;

      // X flip
      if (sprite.attributes & SPRITE_ATTR_BITS_X_FLIP) {
        realX = 7 - realX;
      }

      uint16_t pixelPos = lcdController->ly * LCD_WIDTH + (sprite.xPosition - 8) + lineX;

      Pixel* pixel = &lcdController->frameBuffer[pixelPos];

      uint8_t bgOrWinColour = pixel->colour;
      uint8_t spriteColourNumber = pixelColour(lineData, realX);

      if (lcdController->cgbMode == MONOCHROME) {
        if (spritePixelVisibleInMonochromeMode(spriteColourNumber, bgOrWinColour, pixel, &sprite)) {
          uint8_t palette = ((sprite.attributes & SPRITE_ATTR_BITS_MONOCHROME_PALETTE_NUMBER) ? lcdController->obp1 : lcdController->obp0);
          uint8_t shade = colourToObjectMonochromeShade(lcdController, spriteColourNumber, palette);

          setPixelRGBForMonochromeShade(lcdController, pixel, shade);
        }
      } else if (lcdController->cgbMode == COLOUR) {
        if (spritePixelVisibleInColourMode(lcdController, spriteColourNumber, bgOrWinColour, pixel, &sprite)) {
          uint8_t paletteNum = sprite.attributes & 7;

          Colour colour = getObjectColour(lcdController, paletteNum, spriteColourNumber);
          setPixelRGBForCGBColour(lcdController, pixel, colour);
        }
      }
    }
  }
}


static void lcdDrawScanline(LCDController* lcdController)
{
  lcdDrawScanlineBackground(lcdController);
  if (lcdController->lcdc & LCD_WINDOW_DISPLAY_ENABLE_BIT) {
    lcdDrawScanlineWindow(lcdController);
  }
  if (lcdController->lcdc & LCD_OBJ_DISPLAY_ENABLE_BIT) {
    lcdDrawScanlineObjects(lcdController);
  }
}


static bool hclocksIndicateMode2(LCDController* lcdController, uint16_t horizontalScanClocks)
{
  return (horizontalScanClocks < 80);
}


static bool hclocksIndicateMode3(LCDController* lcdController, uint16_t horizontalScanClocks)
{
  return (horizontalScanClocks >= 80 && horizontalScanClocks < (80 + lcdController->mode3Cycles));
}


static bool hclocksIndicateMode0(LCDController* lcdController, uint16_t horizontalScanClocks)
{
  return (horizontalScanClocks >= (80 + lcdController->mode3Cycles) && horizontalScanClocks < SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES);
}


static void updateInternalClockCycles(LCDController* lcdController, uint8_t cyclesExecuted)
{
  lcdController->clockCycles = (lcdController->clockCycles + cyclesExecuted) % FULL_FRAME_CLOCK_CYCLES;
}


static uint16_t getHorizontalScanClocks(LCDController* lcdController)
{
  return (lcdController->clockCycles % SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES);
}


void updateLY(LCDController* lcdController)
{
  // Update LY and LYC (even in VBLANK), triggering a STAT interrupt for LY=LYC if enabled
  // NOTE: Apparently, for lines 0-143, LY is updated in the transition to Mode 2, however functionally
  // there is no difference if we do this here, and additionally this still works while in Mode 1
  // (VBLANK) and no extra checks are required in the Mode 1 code to keep LY up-to-date.
  uint8_t ly = lcdController->clockCycles / SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES;

  uint16_t horizontalScanClocks = getHorizontalScanClocks(lcdController);

  // Line 153 is interesting, in that LY apparently stays at 153 for ~4 clock cycles then rolls over early to 0.
  // This is noticeable in the introductory sequence to The Legend of Zelda: Link's Awakening where,
  // in the second section of the sequence (on Koholint Island) the top line of the screen will exhibit
  // graphics glitches if the LYC Coincidence interrupt for LY=0 LYC=0 is triggered for the actual
  // line 0, instead of the actual line 153.
  // TODO: I read that this was a hardware bug present only in the GB hardware. Is this verified?
  // NOTE: The "&& horizontalScanClocks >= 4" check below seems unnecessary for functionally correct behaviour
  if (ly == 153 && horizontalScanClocks >= 4) {
    ly = 0;
  }

  if (ly == lcdController->lyc) {
    lcdController->stat |= STAT_COINCIDENCE_FLAG_BIT;
    if (lcdController->stat & STAT_COINCIDENCE_INTERRUPT_ENABLE_BIT && ly != lcdController->ly) {
      // The ly != lcdController->ly check ensures that the LCDC Status interrupt is generated only once per LY=LYC event
      interruptFlag(lcdController->interruptController, LCDC_STATUS_INTERRUPT_BIT);
    }
  } else {
    lcdController->stat &= ~STAT_COINCIDENCE_FLAG_BIT;
  }
  lcdController->ly = ly;
}


void updateMode(LCDController* lcdController)
{
  uint8_t mode = lcdController->stat & STAT_MODE_FLAG_BITS; // NOTE: This is an 'out-of-date' value of mode from the previous update
  uint16_t horizontalScanClocks = getHorizontalScanClocks(lcdController);

  if (lcdController->clockCycles < HORIZONTAL_SCANNING_CLOCK_CYCLES) { // Horizontal scanning mode (modes 2, 3 and 0)
    if (hclocksIndicateMode2(lcdController, horizontalScanClocks)) { // Mode 2
      if (mode == 0 || mode == 1) { // Handle mode change from HBLANK or VBLANK
        lcdStatSetMode(lcdController, 2);
        if (lcdController->stat & STAT_MODE_2_OAM_INTERRUPT_ENABLE_BIT) {
          interruptFlag(lcdController->interruptController, LCDC_STATUS_INTERRUPT_BIT);
        }
        lcdController->mode3Cycles = MODE_3_CYCLES_MAX;
      } else if (mode == 2) { // No mode change
      } else {
        critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 2, horizontalScanClocks, lcdController->clockCycles);
        exit(EXIT_FAILURE);
      }
    } else if (hclocksIndicateMode3(lcdController, horizontalScanClocks)) { // Mode 3
      if (mode == 2) { // Handle mode change from mode 2
        lcdStatSetMode(lcdController, 3);
        lcdDrawScanline(lcdController);
      } else if (mode == 3) { // No mode change
      } else {
        critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 3, horizontalScanClocks, lcdController->clockCycles);
        exit(EXIT_FAILURE);
      }
    } else if (hclocksIndicateMode0(lcdController, horizontalScanClocks)) { // Mode 0
      if (mode == 3) { // Handle mode change from mode 3
        lcdStatSetMode(lcdController, 0);
        if (lcdController->stat & STAT_MODE_0_HBLANK_INTERRUPT_ENABLE_BIT) {
          interruptFlag(lcdController->interruptController, LCDC_STATUS_INTERRUPT_BIT);
        }
      } else if (mode == 0) { // No mode change
      } else {
        critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 0, horizontalScanClocks, lcdController->clockCycles);
        exit(EXIT_FAILURE);
      }
    } else {
      critical("%s: Horizontal scan cycle count exceeded expected maximum (expected max %u, actual value %u)\n",
        __func__,
        SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES - 1,
        horizontalScanClocks
      );
      exit(EXIT_FAILURE);
    }
  } else { // Vertical scanning mode (mode 1)
    // Disable HBLANK stuff from previous mode? Or just set flags?
    if (mode == 0) { // Handle mode change from HBLANK
      lcdStatSetMode(lcdController, 1);

      // Trigger a VBLANK interrupt
      interruptFlag(lcdController->interruptController, VBLANK_INTERRUPT_BIT);

      // Optionally trigger an LCDC Status interrupt
      if (lcdController->stat & STAT_MODE_1_VBLANK_INTERRUPT_ENABLE_BIT) {
        interruptFlag(lcdController->interruptController, LCDC_STATUS_INTERRUPT_BIT);
      }

      // Debug
      if (++lcdController->vblankCounter % 60 == 0) {
        time_t now = time(NULL);
        double secondsSinceLast60VBlanks = difftime(now, lcdController->last60VBlanksTime);
        double meanTimeBetweenFramesMS = (secondsSinceLast60VBlanks / 60.0) * 1000;
        debug("60 VBlanks ~%.2fms/frame (LCDC=0x%02X STAT=0x%02X Background=%u Window=%u Sprites=%u SCX=0x%02X SCY=0x%02X WX=0x%02X WY=0x%02X LYC=0x%02X)\n",
          meanTimeBetweenFramesMS,
          lcdController->lcdc,
          lcdController->stat,
          (lcdController->lcdc & LCD_BG_DISPLAY_BIT) != 0,
          (lcdController->lcdc & LCD_WINDOW_DISPLAY_ENABLE_BIT) != 0,
          (lcdController->lcdc & LCD_OBJ_DISPLAY_ENABLE_BIT) != 0,
          lcdController->scx,
          lcdController->scy,
          lcdController->wx,
          lcdController->wy,
          lcdController->lyc
        );
        lcdController->last60VBlanksTime = now;
      }

    } else if (mode == 1) { // No mode change
    } else {
      critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 1, horizontalScanClocks, lcdController->clockCycles);
      exit(EXIT_FAILURE);
    }
  }
}


void lcdUpdate(LCDController* lcdController, uint8_t cyclesExecuted)
{
  if (!lcdIsEnabled(lcdController)) {
    return;
  }

  updateInternalClockCycles(lcdController, cyclesExecuted);
  updateLY(lcdController);
  updateMode(lcdController);
}


void lcdSpeedChange(LCDController* lcdController)
{
  lcdController->stat = (lcdController->stat & 0xFC) | 1;
  lcdController->clockCycles = 0;
}
