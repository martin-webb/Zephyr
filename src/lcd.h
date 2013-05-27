#ifndef LCD_H_
#define LCD_H_

#include "interrupts.h"
#include "speed.h"

#include <stdint.h>
#include <time.h>

#define LCD_WIDTH 160
#define LCD_HEIGHT 144

#define IO_REG_ADDRESS_LCDC 0xFF40
#define IO_REG_ADDRESS_STAT 0xFF41
#define IO_REG_ADDRESS_SCY 0xFF42
#define IO_REG_ADDRESS_SCX 0xFF43
#define IO_REG_ADDRESS_LY 0xFF44
#define IO_REG_ADDRESS_LYC 0xFF45

#define IO_REG_ADDRESS_BGP 0xFF47
#define IO_REG_ADDRESS_OBP0 0xFF48
#define IO_REG_ADDRESS_OBP1 0xFF49

#define IO_REG_ADDRESS_WY 0xFF4A
#define IO_REG_ADDRESS_WX 0xFF4B

#define LCD_DISPLAY_ENABLE_BIT (1 << 7)
#define LCD_WINDOW_TILE_MAP_DISPLAY_SELECT_BIT (1 << 6)
#define LCD_WINDOW_DISPLAY_ENABLE_BIT (1 << 5)
#define LCD_BG_AND_WINDOW_TILE_DATA_SELECT_BIT (1 << 4)
#define LCD_BG_TILE_MAP_DISPLAY_SELECT_BIT (1 << 3)
#define LCD_OBJ_SIZE_BIT (1 << 2)
#define LCD_OBJ_DISPLAY_ENABLE_BIT (1 << 1)
#define LCD_BG_DISPLAY_BIT (1 << 0)

#define STAT_COINCIDENCE_INTERRUPT_ENABLE_BIT (1 << 6)
#define STAT_MODE_2_OAM_INTERRUPT_ENABLE_BIT (1 << 5)
#define STAT_MODE_1_VBLANK_INTERRUPT_ENABLE_BIT (1 << 4)
#define STAT_MODE_0_HBLANK_INTERRUPT_ENABLE_BIT (1 << 3)
#define STAT_COINCIDENCE_FLAG_BIT (1 << 2)
#define STAT_MODE_FLAG_BITS 0x3

#define VBLANK_INTERRUPT_BIT (1 << 0)
#define LCDC_STATUS_INTERRUPT_BIT (1 << 1)

#define SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES 456
#define FULL_FRAME_CLOCK_CYCLES (SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES * 154)
#define VBLANK_CLOCK_CYCLES (SINGLE_HORIZONTAL_SCAN_CLOCK_CYCLES * 10)
#define HORIZONTAL_SCANNING_CLOCK_CYCLES (FULL_FRAME_CLOCK_CYCLES - VBLANK_CLOCK_CYCLES)

#define MAX_SPRITES 40
#define MAX_SPRITES_PER_LINE 10

#define MODE_3_CYCLES_MAX 272
#define MODE_3_CYCLES_MIN 172
#define MODE_3_CYCLES_PER_SPRITE ((MODE_3_CYCLES_MAX - MODE_3_CYCLES_MIN) / MAX_SPRITES_PER_LINE)

typedef struct {
  uint8_t lcdc; // FF40 - LCD Control (R/W)
  uint8_t stat; // FF41 - LCDC Status (R/W)
  uint8_t scy;  // FF42 - Scroll Y (R/W)
  uint8_t scx;  // FF43 - Scroll X (R/W)
  uint8_t ly;   // FF44 - LCDC Y-Coordinate (R)
  uint8_t lyc;  // FF45 - LY Compare (R/W)
  uint8_t bgp;  // FF47 - BG Palette Data (R/W) - Non CGB Mode Only
  uint8_t obp0; // FF48 - Object Palette 0 Data (R/W) - Non CGB Mode Only
  uint8_t obp1; // FF49 - Object Palette 1 Data (R/W) - Non CGB Mode Only
  uint8_t wy;   // FF4A - Window Y Position (R/W)
  uint8_t wx;   // FF4B - Window X Position - 7 (R/W)

  uint8_t* vram;
  uint8_t* oam;
  uint8_t* frameBuffer;

  uint16_t mode3Cycles;
  uint32_t clockCycles;

  // Values for debug/profiling statistics
  uint32_t vblankCounter;
  time_t last60VBlanksTime;
} LCDController;

void lcdUpdate(LCDController* lcdController, InterruptController* interruptController, SpeedMode speedMode, uint8_t cyclesExecuted);

#endif // LCD_H_