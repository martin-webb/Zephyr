#ifndef MEMORYCONTROLLER_H_
#define MEMORYCONTROLLER_H_

#include "interrupts.h"
#include "joypad.h"
#include "lcd.h"
#include "timercontroller.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct MemoryController MemoryController;

struct MemoryController {
  uint8_t* memory;
  uint8_t* cartridge;

  bool dmaIsActive;
  uint16_t dmaNextAddress;

  uint8_t (*readByteImpl)(MemoryController* memoryController, uint16_t address);
  void (*writeByteImpl)(MemoryController* memoryController, uint16_t address, uint8_t value);

  void* mbc;

  JoypadController* joypadController;
  LCDController* lcdController;
  TimerController* timerController;
  InterruptController* interruptController;
};

#endif // MEMORYCONTROLLER_H_