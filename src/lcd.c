#include "lcd.h"

#include "cpu.h"
#include "logging.h"
#include "speed.h"
#include "sprites.h"

#include <stdlib.h>

bool lcdIsEnabled(LCDController* lcdController)
{
  return lcdController->lcdc & LCD_DISPLAY_ENABLE_BIT;
}

void lcdStatSetMode(LCDController* lcdController, uint8_t mode)
{
  lcdController->stat = (lcdController->stat & 0xFC) | mode;
}

uint8_t lcdMonochromeColourForPixel(uint8_t pixelX, uint8_t lowByte, uint8_t highByte)
{
  switch (pixelX) {
    // High bit shifts for pixel monochrome colour number extraction:
    //   Pixels 0 - 5: Right by (6 - Pixel#)
    //   Pixel 6: No Shift
    //   Pixel 7: Left by 1
    // Low bit shifts for pixel monochrome colour number extraction:
    //   Pixels 0 - 6: Right by (7 - Pixel#)
    //   Pixel 7: No Shift
    case 7:
      return ((highByte << 1) & 2) | (lowByte & 1);
      break;
    case 6:
      return ((highByte) & 2) | ((lowByte >> (7 - pixelX)) & 1);
      break;
    default: // Pixels 0 - 5
      return ((highByte >> (6 - pixelX)) & 2) | ((lowByte >> (7 - pixelX)) & 1);
      break;
  }
}

void lcdDrawScanlineBackground(LCDController* lcdController)
{
  if (lcdController->lcdc & LCD_BG_DISPLAY_BIT)
  {
    uint16_t bgTileMapOffset = ((lcdController->lcdc & LCD_BG_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x9C00 : 0x9800);
    uint16_t bgAndWindowTileTableOffset = ((lcdController->lcdc & LCD_BG_AND_WINDOW_TILE_DATA_SELECT_BIT) ? 0x8000 : 0x8800);

    for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
      // Determine map tile for pixel based on the LCD controller LY, scanline X and SCX and SCY background offsets
      uint8_t backgroundX = (scanlineX + lcdController->scx) % 256;
      uint8_t backgroundY = (lcdController->ly + lcdController->scy) % 256;

      uint16_t backgroundMapTileIndex = ((backgroundY / 8) * 32) + (backgroundX / 8);
      uint16_t backgroundMapTileOffset = lcdController->vram[bgTileMapOffset - 0x8000 + backgroundMapTileIndex];

      uint16_t backgroundTileDataAddress = bgAndWindowTileTableOffset - 0x8000;
      if (bgAndWindowTileTableOffset == 0x8000) {
        backgroundTileDataAddress += backgroundMapTileOffset * 16;
      } else {
        backgroundTileDataAddress = 0x9000 - 0x8000 + (int8_t)backgroundMapTileOffset * 16;
      }

      uint16_t lineOffset = backgroundTileDataAddress + ((backgroundY % 8) * 2);

      uint8_t backgroundTileData[2] = {
        lcdController->vram[lineOffset],
        lcdController->vram[lineOffset + 1]
      };

      // Draw all pixels from the current tile, starting at the offset in the tile determined by the x location in the complete background
      for (uint8_t pixelX = backgroundX % 8; pixelX < 8 && scanlineX < LCD_WIDTH; pixelX++) {
        uint8_t colourNumber = lcdMonochromeColourForPixel(pixelX, backgroundTileData[0], backgroundTileData[1]);

        // Palette lookup
        // TODO: Check GB/GBC mode
        uint8_t shade = (lcdController->bgp >> (colourNumber * 2)) & 3;

        // Draw a pixel!
        // NOTE: We encode the colour number in the top nibble of the byte so that it can be used
        // to determine the OBJ-to-BG priority (and we can't determine the original colour number
        // based on the shade and the palette, because multiple colours could map to the same shade)
        lcdController->frameBuffer[lcdController->ly * LCD_WIDTH + scanlineX] = (colourNumber << 4) | shade;

        // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
        if (pixelX != 7) {
          scanlineX++;
        }
      }

    }
  }
  else // The background is disabled so with a monochrome Game Boy every pixel is white
  {
    for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
      lcdController->frameBuffer[lcdController->ly * LCD_WIDTH + scanlineX] = 0;
    }
  }
}

void lcdDrawScanlineWindow(LCDController* lcdController)
{
  uint16_t windowTileMapOffset = ((lcdController->lcdc & LCD_WINDOW_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x9C00 : 0x9800);
  uint16_t bgAndWindowTileTableOffset = ((lcdController->lcdc & LCD_BG_AND_WINDOW_TILE_DATA_SELECT_BIT) ? 0x8000 : 0x8800);

  // Check if the window offset results in a row of the window on the current scanline - if not then we're done
  if (lcdController->ly < lcdController->wy) {
    return;
  }

  for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
    // Determine map tile for pixel based on the LCD controller LY, scanline X and WX and WY window offsets
    uint8_t windowY = lcdController->ly - lcdController->wy;

    // Check if the window offset results in a column of the window in the current column of the scanline
    if (scanlineX < (lcdController->wx - 7)) {
      continue;
    }

    uint8_t windowX = scanlineX - (lcdController->wx - 7);

    uint16_t windowMapTileIndex = ((windowY / 8) * 32) + (windowX / 8);
    uint16_t windowMapTileOffset = lcdController->vram[windowTileMapOffset - 0x8000 + windowMapTileIndex];

    uint16_t windowTileDataAddress = bgAndWindowTileTableOffset - 0x8000;
    if (bgAndWindowTileTableOffset == 0x8000) {
      windowTileDataAddress += windowMapTileOffset * 16;
    } else {
      windowTileDataAddress = 0x9000 - 0x8000 + (int8_t)windowMapTileOffset * 16;
    }

    uint16_t lineOffset = windowTileDataAddress + ((windowY % 8) * 2);

    uint8_t windowTileData[2] = {
      lcdController->vram[lineOffset],
      lcdController->vram[lineOffset + 1]
    };

    // Draw all pixels from the current tile, starting at the offset in the tile determined by the x location in the complete background
    for (uint8_t pixelX = windowX % 8; pixelX < 8 && scanlineX < LCD_WIDTH; pixelX++) {
      uint8_t colourNumber = lcdMonochromeColourForPixel(pixelX, windowTileData[0], windowTileData[1]);

      // Palette lookup
      // TODO: Check GB/GBC mode
      uint8_t shade = (lcdController->bgp >> (colourNumber * 2)) & 3;

      // Draw a pixel!
      // NOTE: We encode the colour number in the top nibble of the byte so that it can be used
      // to determine the OBJ-to-BG priority (and we can't determine the original colour number
      // based on the shade and the palette, because multiple colours could map to the same shade)
      lcdController->frameBuffer[lcdController->ly * LCD_WIDTH + scanlineX] = (colourNumber << 4) | shade;

      // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
      if (pixelX != 7) {
        scanlineX++;
      }
    }

  }
}

uint8_t lcdCopySpritesVisibleInScanline(LCDController* lcdController, Sprite* sprites, uint8_t spriteHeight)
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
      if (spriteY < (lcdController->ly + 1) || spriteY > (lcdController->ly + 16)) {
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

void lcdDrawScanlineObjects(LCDController* lcdController, SpeedMode speedMode)
{
  Sprite sprites[MAX_SPRITES];

  const uint8_t spriteHeight = ((lcdController->lcdc & LCD_OBJ_SIZE_BIT) ? 16 : 8);

  // Fetch sprites that we know to be in the visible scanline
  const uint8_t spriteCount = lcdCopySpritesVisibleInScanline(lcdController, sprites, spriteHeight);

  // For non CGB mode, order sprites that are in the scanline by X position and then order in the sprite table
  if (speedMode == NORMAL) {
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
    if (lineNumInSprite > spriteHeight - 1) {
      continue;
    }

    // Y flip
    if (sprite.attributes & SPRITE_ATTR_BITS_Y_FLIP) {
      lineNumInSprite = (spriteHeight - 1) - lineNumInSprite;
    }

    uint8_t lineBytes[2] = {
      lcdController->vram[sprite.tileNumber * 16 + (lineNumInSprite * 2)],
      lcdController->vram[sprite.tileNumber * 16 + (lineNumInSprite * 2) + 1]
    };

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

      // TODO: Check GB/GBC mode

      uint8_t bgOrWinColourNumber = lcdController->frameBuffer[pixelPos] >> 4;
      uint8_t spriteColourNumber = lcdMonochromeColourForPixel(realX, lineBytes[0], lineBytes[1]);

      if ((bgOrWinColourNumber == 0 || !(sprite.attributes & SPRITE_ATTR_BITS_OBJ_TO_BG_PRIORITY)) && spriteColourNumber != 0) {
        uint8_t palette = ((sprite.attributes & SPRITE_ATTR_BITS_MONOCHROME_PALETTE_NUMBER) ? lcdController->obp1 : lcdController->obp0);
        uint8_t shade = (palette >> (spriteColourNumber * 2)) & 3;

        lcdController->frameBuffer[pixelPos] = shade;
      }
    }
  }
}

void lcdDrawScanline(LCDController* lcdController, SpeedMode speedMode)
{
  lcdDrawScanlineBackground(lcdController);
  if (lcdController->lcdc & LCD_WINDOW_DISPLAY_ENABLE_BIT) {
    lcdDrawScanlineWindow(lcdController);
  }
  if (lcdController->lcdc & LCD_OBJ_DISPLAY_ENABLE_BIT) {
    lcdDrawScanlineObjects(lcdController, speedMode);
  }
}

void lcdUpdate(LCDController* lcdController, InterruptController* interruptController, SpeedMode speedMode, uint8_t cyclesExecuted)
{
  if (!lcdIsEnabled(lcdController)) {
    return;
  }

  // Update the internal clock cycle counter
  lcdController->clockCycles = (lcdController->clockCycles + cyclesExecuted) % FULL_FRAME_CLOCK_CYCLES;

  uint8_t mode = lcdController->stat & STAT_MODE_FLAG_BITS; // NOTE: This is an 'out-of-date' value of mode from the previous update
  uint16_t horizontalScanClocks = lcdController->clockCycles % SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES;

  // Update LY and LYC (even in VBLANK), triggering a STAT interrupt for LY=LYC if enabled
  // NOTE: Apparently, for lines 0-143, LY is updated in the transition to Mode 2, however functionally
  // there is no difference if we do this here, and additionally this still works while in Mode 1
  // (VBLANK) and no extra checks are required in the Mode 1 code to keep LY up-to-date.
  uint8_t ly = lcdController->clockCycles / SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES;

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
      // NOTE: // The ly != lcdController->ly check ensures that the LCDC Status interrupt is generated only once per LY=LYC event
      interruptFlag(interruptController, LCDC_STATUS_INTERRUPT_BIT);
    }
  } else {
    lcdController->stat &= ~STAT_COINCIDENCE_FLAG_BIT;
  }
  lcdController->ly = ly;

  if (lcdController->clockCycles < HORIZONTAL_SCANNING_CLOCK_CYCLES) // Horizontal scanning mode (modes 2, 3 and 0)
  {
    if (horizontalScanClocks < 80) // Mode 2
    {
      if (mode == 0 || mode == 1) // Handle mode change from HBLANK or VBLANK
      {
        lcdStatSetMode(lcdController, 2);
        if (lcdController->stat & STAT_MODE_2_OAM_INTERRUPT_ENABLE_BIT) {
          interruptFlag(interruptController, LCDC_STATUS_INTERRUPT_BIT);
        }
        lcdController->mode3Cycles = MODE_3_CYCLES_MAX;
      }
      else if (mode == 2) {} // No mode change
      else
      {
        critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 2, horizontalScanClocks, lcdController->clockCycles);
        exit(EXIT_FAILURE);
      }
    }
    else if (horizontalScanClocks >= 80 && horizontalScanClocks < (80 + lcdController->mode3Cycles)) // Mode 3
    {
      if (mode == 2) // Handle mode change from mode 2
      {
        lcdStatSetMode(lcdController, 3);
        lcdDrawScanline(lcdController, speedMode);
      }
      else if (mode == 3) {} // No mode change
      else
      {
        critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 3, horizontalScanClocks, lcdController->clockCycles);
        exit(EXIT_FAILURE);
      }
    }
    else if (horizontalScanClocks >= (80 + lcdController->mode3Cycles) && horizontalScanClocks < SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES) // Mode 0
    {
      if (mode == 3) // Handle mode change from mode 3
      {
        lcdStatSetMode(lcdController, 0);
        if (lcdController->stat & STAT_MODE_0_HBLANK_INTERRUPT_ENABLE_BIT) {
          interruptFlag(interruptController, LCDC_STATUS_INTERRUPT_BIT);
        }
      }
      else if (mode == 0) {}
      else // No mode change
      {
        critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 0, horizontalScanClocks, lcdController->clockCycles);
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      critical("%s: Horizontal scan cycle count exceeded expected maximum (expected max %u, actual value %u)\n",
        __func__,
        SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES - 1,
        horizontalScanClocks
      );
      exit(EXIT_FAILURE);
    }
  }
  else // Vertical scanning mode (mode 1)
  {
    // Disable HBLANK stuff from previous mode? Or just set flags?
    if (mode == 0) // Handle mode change from HBLANK
    {
      lcdStatSetMode(lcdController, 1);
      // Trigger a VBLANK interrupt
      interruptFlag(interruptController, VBLANK_INTERRUPT_BIT);

      // Optionally trigger an LCDC Status interrupt
      if (lcdController->stat & STAT_MODE_1_VBLANK_INTERRUPT_ENABLE_BIT) {
        interruptFlag(interruptController, LCDC_STATUS_INTERRUPT_BIT);
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

    }
    else if (mode == 1) {} // No mode change
    else
    {
      critical("%s: Invalid LCDC mode transition from %u to %u (hclocks=%u vclocks=%u)\n", __func__, mode, 1, horizontalScanClocks, lcdController->clockCycles);
      exit(EXIT_FAILURE);
    }
  }
}