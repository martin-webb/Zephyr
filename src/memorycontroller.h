#ifndef MEMORYCONTROLLER_H_
#define MEMORYCONTROLLER_H_

#include "cgbmode.h"
#include "interrupts.h"
#include "joypad.h"
#include "lcd.h"
#include "speed.h"
#include "timercontroller.h"

#include <stdbool.h>
#include <stdint.h>

#define IO_REG_ADDRESS_SVBK 0xFF70

typedef struct MemoryController MemoryController;

struct MemoryController {
  uint8_t* vram;
  uint8_t* wram;
  uint8_t* oam;
  uint8_t* hram;
  uint8_t* cartridge;

  uint8_t dma; // FF46 - DMA - DMA Transfer and Start Address (W)
  bool dmaIsActive;
  uint16_t dmaNextAddress;

  uint8_t svbk; // FF70 - SVBK - WRAM Bank - CGB Mode Only (R/W)

  uint8_t (*readByteImpl)(MemoryController* memoryController, uint16_t address);
  void (*writeByteImpl)(MemoryController* memoryController, uint16_t address, uint8_t value);
  void (*cartridgeUpdateImpl)(MemoryController* memoryController, uint32_t cyclesExecuted, SpeedMode speedMode);

  void* mbc;

  CGBMode cgbMode;
  JoypadController* joypadController;
  LCDController* lcdController;
  TimerController* timerController;
  InterruptController* interruptController;
};

#endif // MEMORYCONTROLLER_H_