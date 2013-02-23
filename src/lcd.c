#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "lcd.h"
#include "logging.h"

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
      for (uint8_t pixelX = backgroundX % 8; pixelX < 8; pixelX++) {
        uint8_t colourNumber = lcdMonochromeColourForPixel(pixelX, backgroundTileData[0], backgroundTileData[1]);
        
        // Palette lookup
        // TODO: Check GB/GBC mode
        uint8_t shade = (lcdController->bgp >> (colourNumber * 2)) & 3;

        // Draw a pixel!
        lcdController->frameBuffer[lcdController->ly * LCD_WIDTH + scanlineX] = shade;

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
  uint16_t windowTileMapOffset = ((lcdController->lcdc & LCD_BG_TILE_MAP_DISPLAY_SELECT_BIT) ? 0x9C00 : 0x9800);
  uint16_t bgAndWindowTileTableOffset = ((lcdController->lcdc & LCD_BG_AND_WINDOW_TILE_DATA_SELECT_BIT) ? 0x8000 : 0x8800);
  
  // Check if the window offset results in a row of the window on the current scanline - if not then we're done
  if (lcdController->ly < lcdController->wy) {
    return;
  }
  
  for (int scanlineX = 0; scanlineX < LCD_WIDTH; scanlineX++) {
    // Determine map tile for pixel based on the LCD controller LY, scanline X and WX and WY window offsets
    uint8_t windowY = lcdController->wy - lcdController->ly;
    
    // Check if the window offset results in a column of the window in the current column of the scanline
    if (scanlineX < (lcdController->wx - 7)) {
      continue;
    }
    
    uint8_t windowX = (lcdController->wx - 7) - scanlineX;
    
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
    for (uint8_t pixelX = windowX % 8; pixelX < 8; pixelX++) {
      uint8_t colourNumber = lcdMonochromeColourForPixel(pixelX, windowTileData[0], windowTileData[1]);

      // Palette lookup
      // TODO: Check GB/GBC mode
      uint8_t shade = (lcdController->bgp >> (colourNumber * 2)) & 3;

      // Draw a pixel!
      lcdController->frameBuffer[lcdController->ly * LCD_WIDTH + scanlineX] = shade;

      // Don't increment this for the last pixel of the current tile, the increment in the outer loop will do this for us
      if (pixelX != 7) {
        scanlineX++;
      }
    }

  }
}

void lcdDrawScanline(LCDController* lcdController)
{ 
  lcdDrawScanlineBackground(lcdController);
  if (lcdController->lcdc & LCD_WINDOW_DISPLAY_ENABLE_BIT) {
    lcdDrawScanlineWindow(lcdController);
  }
}

void lcdUpdate(LCDController* lcdController, InterruptController* interruptController, uint8_t cyclesExecuted)
{
  if (!lcdIsEnabled(lcdController)) {
    return;
  }
  
  // Update the internal clock cycle counter
  lcdController->clockCycles = (lcdController->clockCycles + cyclesExecuted) % FULL_FRAME_CLOCK_CYCLES;
  
  uint8_t mode = lcdController->stat & STAT_MODE_FLAG_BITS; // NOTE: This is an 'out-of-date' value of mode from the previous update
  uint16_t horizontalScanClocks = lcdController->clockCycles % SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES;
  
  // Update LY and LYC (even in VBLANK), triggering a STAT interrupt for LY=LYC if enabled
  uint8_t ly = lcdController->clockCycles / SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES;
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
      }
      else if (mode == 2) {} // No mode change
      else
      {
        critical("%s: Invalid LCDC mode transition from %u to %u\n", __func__, mode, 2);
        exit(EXIT_FAILURE);
      }
    }
    else if (horizontalScanClocks >= 80 && horizontalScanClocks < 252) // Mode 3
    {
      if (mode == 2) // Handle mode change from mode 2
      {
        lcdStatSetMode(lcdController, 3);
        lcdDrawScanline(lcdController);
        
      }
      else if (mode == 3) {} // No mode change
      else
      {
        critical("%s: Invalid LCDC mode transition from %u to %u\n", __func__, mode, 3);
        exit(EXIT_FAILURE);
      }
    }
    else if (horizontalScanClocks >= 252 && horizontalScanClocks < SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES) // Mode 0
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
        critical("%s: Invalid LCDC mode transition from %u to %u\n", __func__, mode, 0);
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
        debug("60 VBlanks (LCDC=%u STAT=%u Background=%u Window=%u Sprites=%u)\n",
          lcdController->lcdc,
          lcdController->stat,
          (lcdController->lcdc & LCD_BG_DISPLAY_BIT) != 0,
          (lcdController->lcdc & LCD_WINDOW_DISPLAY_ENABLE_BIT) != 0,
          (lcdController->lcdc & LCD_OBJ_DISPLAY_ENABLE_BIT) != 0
        );
      }
      
    }
    else if (mode == 1) {} // No mode change
    else
    {
      critical("%s: Invalid LCDC mode transition from %u to %u\n", __func__, mode, 1);
      exit(EXIT_FAILURE);
    }
  }
}