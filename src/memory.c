#include "memory.h"

#include "cartridge.h"
#include "cartridge-types/mbc1.h"
#include "cartridge-types/romonly.h"
#include "logging.h"
#include "timer.h"

#include <stdlib.h>

MemoryController InitMemoryController(
  uint8_t cartridgeType,
  uint8_t* memory,
  uint8_t* cartridge,
  JoypadController* joypadController,
  LCDController* lcdController,
  TimerController* timerController,
  InterruptController* interruptController,
  uint32_t externalRAMSizeBytes,
  const char* romFilename
)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    false,
    0x0000,
    NULL,
    NULL,
    NULL,
    joypadController,
    lcdController,
    timerController,
    interruptController
  };

  switch (cartridgeType) {
    case CARTRIDGE_TYPE_ROM_ONLY:
      romOnlyInitialiseMemoryController(&memoryController);
      break;
    case CARTRIDGE_TYPE_MBC1:
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM_PLUS_BATTERY:
      mbc1InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename);
      break;
    case CARTRIDGE_TYPE_MBC2:
    case CARTRIDGE_TYPE_MBC2_PLUS_BATTERY:
    case CARTRIDGE_TYPE_ROM_PLUS_RAM:
    case CARTRIDGE_TYPE_ROM_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MMM01:
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM:
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC3:
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC4:
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC5:
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE:
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_POCKET_CAMERA:
    case CARTRIDGE_TYPE_BANDAI_TAMA5:
    case CARTRIDGE_TYPE_HuC3:
    case CARTRIDGE_TYPE_HuC1_PLUS_RAM_PLUS_BATTERY:
      critical("Error in %s - cartridge type UNSUPPORTED\n", __func__);
      exit(EXIT_FAILURE);
      break;
    default:
      critical("Error in %s - cartridge type UNKNOWN\n", __func__);
      exit(EXIT_FAILURE);
      break;
  }

  return memoryController;
}

uint8_t readByte(MemoryController* memoryController, uint16_t address)
{
  if (memoryController->dmaIsActive && (address < 0xFF80 || address > 0xFFFE)) {
    critical("Read from non-HRAM address 0x%04X while DMA is active.\n", address);
    exit(EXIT_FAILURE);
  }
  return memoryController->readByteImpl(memoryController, address);
}

uint16_t readWord(MemoryController* memoryController, uint16_t address)
{
  uint8_t lsByte = memoryController->readByteImpl(memoryController, address);
  uint8_t msByte = memoryController->readByteImpl(memoryController, address + 1);
  return (msByte << 8) | lsByte;
}

void writeByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (memoryController->dmaIsActive && (address < 0xFF80 || address > 0xFFFE)) {
    critical("Write of value 0x%02X to non-HRAM address 0x%04X while DMA is active.\n", value, address);
    exit(EXIT_FAILURE);
  }
  memoryController->writeByteImpl(memoryController, address, value);
}

void writeWord(MemoryController* memoryController, uint16_t address, uint16_t value)
{
  memoryController->writeByteImpl(memoryController, address, value & 0x00FF);
  memoryController->writeByteImpl(memoryController, address + 1, (value & 0xFF00) >> 8);
}

uint8_t commonReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address >= 0x8000 && address < 0xA000) // Read from VRAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode != 3) { // LCD Controller is not reading from VRAM and OAM so read access is okay
      return memoryController->memory[address - CARTRIDGE_SIZE];
    } else {
      return 0xFF;
    }
  }
  else if (address >= 0xFE00 && address < 0xFEA0) // Read from OAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode == 0 || lcdMode == 1) { // LCD Controller is in HBLANK or VBLANK so read access is okay
      return memoryController->memory[address - CARTRIDGE_SIZE];
    } else {
      return 0xFF;
    }
  }
  else if (address == IO_REG_ADDRESS_P1)
  {
    return memoryController->joypadController->p1;
  }
  else if (address == IO_REG_ADDRESS_LCDC)
  {
    return memoryController->lcdController->lcdc;
  }
  else if (address == IO_REG_ADDRESS_STAT)
  {
    return memoryController->lcdController->stat;
  }
  else if (address == IO_REG_ADDRESS_SCY)
  {
    return memoryController->lcdController->scy;
  }
  else if (address == IO_REG_ADDRESS_SCX)
  {
    return memoryController->lcdController->scx;
  }
  else if (address == IO_REG_ADDRESS_LY)
  {
    return memoryController->lcdController->ly;
  }
  else if (address == IO_REG_ADDRESS_LYC)
  {
    return memoryController->lcdController->lyc;
  }
  else if (address == IO_REG_ADDRESS_DMA)
  {
    warning("Read from DMA I/O register.\n");
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
  else if (address == IO_REG_ADDRESS_BGP)
  {
    return memoryController->lcdController->bgp;
  }
  else if (address == IO_REG_ADDRESS_OBP0)
  {
    return memoryController->lcdController->obp0;
  }
  else if (address == IO_REG_ADDRESS_OBP1)
  {
    return memoryController->lcdController->obp1;
  }
  else if (address == IO_REG_ADDRESS_WY)
  {
    return memoryController->lcdController->wy;
  }
  else if (address == IO_REG_ADDRESS_WX)
  {
    return memoryController->lcdController->wx;
  }
  else if (address == IO_REG_ADDRESS_DIV)
  {
    return memoryController->timerController->div;
  }
  else if (address == IO_REG_ADDRESS_TIMA)
  {
    return memoryController->timerController->tima;
  }
  else if (address == IO_REG_ADDRESS_TMA)
  {
    return memoryController->timerController->tma;
  }
  else if (address == IO_REG_ADDRESS_TAC)
  {
    return memoryController->timerController->tac;
  }
  else if (address == IO_REG_ADDRESS_IF)
  {
    return memoryController->interruptController->f;
  }
  else if (address == IO_REG_ADDRESS_IE)
  {
    return memoryController->interruptController->e;
  }
  else
  {
    return memoryController->memory[address - CARTRIDGE_SIZE];
  }
}

void commonWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address >= 0x8000 && address < 0xA000) // Write to VRAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode != 3) { // LCD Controller is not reading from VRAM and OAM so write access is okay
      memoryController->memory[address - CARTRIDGE_SIZE] = value;
    } else {
      warning("Invalid write to VRAM while LCD is in Mode 3\n");
    }
  }
  else if (address >= 0xFE00 && address < 0xFEA0) // Write to OAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode == 0 || lcdMode == 1) { // LCD Controller is in HBLANK or VBLANK so write access is okay
      memoryController->memory[address - CARTRIDGE_SIZE] = value;
    } else {
      warning("Invalid write to OAM while LCD is not in Mode 0 or Mode 1\n");
    }
  }
  else if (address == IO_REG_ADDRESS_LCDC)
  {
    if (((memoryController->lcdController->lcdc & LCD_DISPLAY_ENABLE_BIT) == 0) && (value & LCD_DISPLAY_ENABLE_BIT)) {
      debug("LCD was ENABLED by write of value 0x%02X to LCDC\n", value);
    } else if ((memoryController->lcdController->lcdc & LCD_DISPLAY_ENABLE_BIT) && ((value & LCD_DISPLAY_ENABLE_BIT) == 0)) {
      debug("LCD was DISABLED by write of value 0x%02X to LCDC\n", value);
    }

    memoryController->lcdController->lcdc = value;
    // Handle enable and disable of LCD
    if (value & LCD_DISPLAY_ENABLE_BIT) {
      // TODO: What to do here?
      // memoryController->lcdController->stat = 0x02;
      // memoryController->lcdController->clockCycles = 0; // This must be reset on enable otherwise invalid mode transitions can occur falsely
    } else {
      // Stopping LCD operation outside of the VBLANK period can damage the display hardware
      // Nintendo is reported to reject any games that didn't follow this rule.
      // TODO: Make this a warning? Or a runtime option (e.g. "strict-lcd-disable-rule")?
      uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
      if (lcdMode != 0x01) {
        critical("Attempt to disable LCD outside of VBLANK period!\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  else if (address == IO_REG_ADDRESS_P1)
  {
    memoryController->joypadController->p1 = (value & 0x30) | 0x0F; // TODO: Do we need to preserve or reset (set to 1) the low 4 bits here?
    if ((value & (1 << 5)) == 0) {
      if (memoryController->joypadController->_start) memoryController->joypadController->p1 &= ~(1 << 3);
      if (memoryController->joypadController->_select) memoryController->joypadController->p1 &= ~(1 << 2);
      if (memoryController->joypadController->_b) memoryController->joypadController->p1 &= ~(1 << 1);
      if (memoryController->joypadController->_a) memoryController->joypadController->p1 &= ~(1 << 0);
    } else if ((value & (1 << 4)) == 0) {
      if (memoryController->joypadController->_down) memoryController->joypadController->p1 &= ~(1 << 3);
      if (memoryController->joypadController->_up) memoryController->joypadController->p1 &= ~(1 << 2);
      if (memoryController->joypadController->_left) memoryController->joypadController->p1 &= ~(1 << 1);
      if (memoryController->joypadController->_right) memoryController->joypadController->p1 &= ~(1 << 0);
    }
  }
  else if (address == IO_REG_ADDRESS_STAT)
  {
    memoryController->lcdController->stat = (value & 0xFC) | (memoryController->lcdController->stat & 0x03);
  }
  else if (address == IO_REG_ADDRESS_SCY)
  {
    memoryController->lcdController->scy = value;
  }
  else if (address == IO_REG_ADDRESS_SCX)
  {
    memoryController->lcdController->scx = value;
  }
  else if (address == IO_REG_ADDRESS_LY)
  {
    memoryController->lcdController->ly = 0;
    debug("[MEMORY LOG]: WRITE TO LY!\n");
  }
  else if (address == IO_REG_ADDRESS_LYC)
  {
    memoryController->lcdController->lyc = value;
  }
  else if (address == IO_REG_ADDRESS_DMA)
  {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;
    memoryController->dmaIsActive = true;
    memoryController->dmaNextAddress = value * 0x100;
  }
  else if (address == IO_REG_ADDRESS_BGP)
  {
    memoryController->lcdController->bgp = value;
  }
  else if (address == IO_REG_ADDRESS_OBP0)
  {
    memoryController->lcdController->obp0 = value;
  }
  else if (address == IO_REG_ADDRESS_OBP1)
  {
    memoryController->lcdController->obp1 = value;
  }
  else if (address == IO_REG_ADDRESS_WY)
  {
    memoryController->lcdController->wy = value;
  }
  else if (address == IO_REG_ADDRESS_WX)
  {
    memoryController->lcdController->wx = value;
  }
  else if (address == IO_REG_ADDRESS_DIV)
  {
    memoryController->timerController->div = 0x00;
  }
  else if (address == IO_REG_ADDRESS_TIMA)
  {
    memoryController->timerController->tima = value;
  }
  else if (address == IO_REG_ADDRESS_TMA)
  {
    memoryController->timerController->tma = value;
  }
  else if (address == IO_REG_ADDRESS_TAC)
  {
    memoryController->timerController->tac = value;
  }
  else if (address == IO_REG_ADDRESS_IF)
  {
    memoryController->interruptController->f = value;
  }
  else if (address == IO_REG_ADDRESS_IE)
  {
    memoryController->interruptController->e = value;
  }
  else
  {
    memoryController->memory[address - CARTRIDGE_SIZE] = value;

    // Additional write to support the echo of 7.5KB of internal RAM
    if (address >= 0xC000 && address < 0xDE00) {
      memoryController->memory[address - CARTRIDGE_SIZE + 0x2000] = value;
    } else if (address >= 0xE000 && address < 0xFE00) {
      memoryController->memory[address - CARTRIDGE_SIZE - 0x2000] = value;
    }
  }

  // TODO: Add to formal logging strategy that can be removed for release builds (for example)
  if (address >= 0x8000 && address < 0xA000) {
    // debug("\b[MEMORY] Write to VRAM address=0x%04X value=0x%02X\n", address, value);
  } else if (address >= 0xFE00 && address < 0xFEA0) {
    // debug("\b[MEMORY] Write to OAM address=0x%04X value=0x%02X\n", address, value);
  } else if (address >= 0xFF00 && address < 0xFF4C) {
    // debug("\b[MEMORY] Write to I/O reg - address=0x%04X value=0x%02X\n", address, value);
  }
}

void dmaUpdate(MemoryController* memoryController, uint8_t cyclesExecuted)
{
  if (memoryController->dmaIsActive) {
    if (cyclesExecuted % 4 != 0) {
      critical("%s called with cycle count not divisible by four.", __func__);
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < cyclesExecuted / 4; i++) {
      uint16_t sourceAddress = memoryController->dmaNextAddress;
      uint16_t destinationAddress = 0xFE00 + (sourceAddress & 0x00FF);

      // Memory can be transferred from ROM or RAM, so we need to use the MBC readByte() implementations (NOT the "public" CPU methods) to handle ROM banking.
      uint8_t value;
      if (sourceAddress < CARTRIDGE_SIZE) {
        value = memoryController->readByteImpl(memoryController, sourceAddress);
      } else {
        value = memoryController->memory[sourceAddress - CARTRIDGE_SIZE];
      }
      memoryController->memory[destinationAddress - CARTRIDGE_SIZE] = value;

      memoryController->dmaNextAddress++;

      if ((destinationAddress + 1) == 0xFEA0) {
        memoryController->dmaIsActive = false;
        break;
      }

    }
  }
}