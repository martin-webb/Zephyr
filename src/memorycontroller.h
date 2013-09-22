#ifndef MEMORYCONTROLLER_H_
#define MEMORYCONTROLLER_H_

#include "cgbmode.h"
#include "hdmatransfer.h"
#include "interrupts.h"
#include "joypad.h"
#include "lcd.h"
#include "speed.h"
#include "speedcontroller.h"
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

  uint8_t hdma1; // FF51 - HDMA1 - New DMA Source, High - CGB Mode Only
  uint8_t hdma2; // FF52 - HDMA1 - New DMA Source, Low - CGB Mode Only
  uint8_t hdma3; // FF53 - HDMA1 - New DMA Source, High - CGB Mode Only
  uint8_t hdma4; // FF54 - HDMA1 - New DMA Source, High - CGB Mode Only
  uint8_t hdma5; // FF55 - HDMA1 - New DMA Source, High - CGB Mode Only

  HDMATransfer hdmaTransfer;

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
  SpeedController* speedController;
};

#endif // MEMORYCONTROLLER_H_