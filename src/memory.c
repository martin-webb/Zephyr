#include "memory.h"

#include "cartridge.h"
#include "cartridge-types/mbc1.h"
#include "cartridge-types/mbc3.h"
#include "cartridge-types/mbc5.h"
#include "cartridge-types/romonly.h"
#include "hdmatransfer.h"
#include "logging.h"
#include "speedcontroller.h"
#include "timer.h"

#include <stdlib.h>

#define HBLANK_DMA_TRANSFER_LENGTH 16

const HDMATransfer HDMA_TRANSFER_DEFAULT =
{
  .type = GENERAL,
  .isActive = false,
  .length = 0,
  .nextSourceAddr = 0,
  .nextDestinationAddr = 0
};

MemoryController InitMemoryController(
  uint8_t cartridgeType,
  uint8_t* vram,
  uint8_t* wram,
  uint8_t* oam,
  uint8_t* hram,
  uint8_t* cartridge,
  CGBMode cgbMode,
  JoypadController* joypadController,
  LCDController* lcdController,
  TimerController* timerController,
  InterruptController* interruptController,
  SpeedController* speedController,
  uint32_t externalRAMSizeBytes,
  const char* romFilename
)
{
  MemoryController memoryController = {
    vram,
    wram,
    oam,
    hram,
    cartridge,
    0,
    false,
    0,
    0x0000,
    0,
    0,
    0,
    0,
    0,
    HDMA_TRANSFER_DEFAULT,
    1, // SVBK should be initialised to 1 because writes of 0 are always translated to 1
    NULL,
    NULL,
    NULL,
    NULL,
    cgbMode,
    joypadController,
    lcdController,
    timerController,
    interruptController,
    speedController
  };

  switch (cartridgeType) {
    case CARTRIDGE_TYPE_ROM_ONLY:
      romOnlyInitialiseMemoryController(&memoryController);
      break;
    case CARTRIDGE_TYPE_MBC1:
      mbc1InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, false, false);
      break;
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM:
      mbc1InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, false);
      break;
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM_PLUS_BATTERY:
      mbc1InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, true);
      break;
    case CARTRIDGE_TYPE_MBC2:
    case CARTRIDGE_TYPE_MBC2_PLUS_BATTERY:
    case CARTRIDGE_TYPE_ROM_PLUS_RAM:
    case CARTRIDGE_TYPE_ROM_PLUS_RAM_PLUS_BATTERY:
    case CARTRIDGE_TYPE_MMM01:
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM:
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM_PLUS_BATTERY:
      critical("Error in %s - cartridge type UNSUPPORTED\n", __func__);
      exit(EXIT_FAILURE);
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_BATTERY:
      mbc3InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, false, true, true);
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_RAM_PLUS_BATTERY:
      mbc3InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, true, true);
      break;
    case CARTRIDGE_TYPE_MBC3:
      mbc3InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, false, false, false);
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM:
      mbc3InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, false, false);
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM_PLUS_BATTERY:
      mbc3InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, false, true);
      break;
    case CARTRIDGE_TYPE_MBC4:
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM_PLUS_BATTERY:
      critical("Error in %s - cartridge type UNSUPPORTED\n", __func__);
      exit(EXIT_FAILURE);
      break;
    case CARTRIDGE_TYPE_MBC5:
      mbc5InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, false, false);
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM:
      mbc5InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, false);
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM_PLUS_BATTERY:
      mbc5InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, true);
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE:
      mbc5InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, false, false);
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM:
      mbc5InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, false);
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM_PLUS_BATTERY:
      mbc5InitialiseMemoryController(&memoryController, externalRAMSizeBytes, romFilename, true, true);
      break;
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
  if (address >= 0x8000 && address <= 0x9FFF) // Read from VRAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode != 3) { // LCD Controller is not reading from VRAM and OAM so read access is okay
      if (memoryController->cgbMode == COLOUR) {
        uint16_t bankOffset = (memoryController->lcdController->vbk * 8 * 1024);
        return memoryController->vram[bankOffset + address - 0x8000];
      } else {
        return memoryController->vram[address - 0x8000];
      }
    } else {
      return 0xFF;
    }
  }
  else if (address >= 0xC000 && address <= 0xCFFF) // Read from WRAM (Bank 0)
  {
    return memoryController->wram[address - 0xC000];
  }
  else if (address >= 0xD000 && address <= 0xDFFF) // Read from WRAM (Banks 1-7)
  {
    if (memoryController->cgbMode == COLOUR) {
      uint16_t bankOffset = memoryController->svbk * 4 * 1024;
      return memoryController->wram[bankOffset + (address - 0xD000)];
    } else {
      return memoryController->wram[address - 0xC000];
    }
  }
  else if (address >= 0xE000 && address <= 0xFDFF) // Read from WRAM (echo)
  {
    return memoryController->wram[address - 0xE000];
  }
  else if (address >= 0xFE00 && address <= 0xFE9F) // Read from OAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode == 0 || lcdMode == 1) { // LCD Controller is in HBLANK or VBLANK so read access is okay
      return memoryController->oam[address - 0xFE00];
    } else {
      return 0xFF;
    }
  }
  else if (address >= 0xFEA0 && address <= 0xFEFF) // Not Usable
  {
    warning("Read from unusable address 0x%04X\n", address);
    return 0;
  }
  else if (address >= 0xFF00 && address <= 0xFF7F) // I/O Ports
  {
    if (address == IO_REG_ADDRESS_P1) { // 0xFF00
      return memoryController->joypadController->p1;
    } else if (address >= IO_REG_ADDRESS_DIV && address <= IO_REG_ADDRESS_TAC) { // 0xFF04 - 0xFF07
      switch (address) {
        case IO_REG_ADDRESS_DIV: // 0xFF04
          return memoryController->timerController->div;
        case IO_REG_ADDRESS_TIMA: // 0xFF05
          return memoryController->timerController->tima;
        case IO_REG_ADDRESS_TMA: // 0xFF06
          return memoryController->timerController->tma;
        case IO_REG_ADDRESS_TAC: // 0xFF07
          return memoryController->timerController->tac;
      }
    } else if (address == IO_REG_ADDRESS_IF) { // 0xFF0F
      return memoryController->interruptController->f;
    } else if (address == 0xFF1A) {
      return 0x80;
    } else if (address == IO_REG_ADDRESS_DMA) { // 0xFF46
      warning("Read from DMA I/O register (write only).\n");
      return memoryController->dma;
    } else if ((address >= IO_REG_ADDRESS_LCDC && address <= IO_REG_ADDRESS_LYC) ||  // 0xFF40 - 0xFF45
               (address >= IO_REG_ADDRESS_BGP  && address <= IO_REG_ADDRESS_WX)  ||  // 0xFF47 - 0xFF4B
               (address >= IO_REG_ADDRESS_BCPS && address <= IO_REG_ADDRESS_OCPD)) { // 0xFF68 - 0xFF6B
      switch (address) {
        case IO_REG_ADDRESS_LCDC: // 0xFF40
          return memoryController->lcdController->lcdc;
        case IO_REG_ADDRESS_STAT: // 0xFF41
          return memoryController->lcdController->stat;
        case IO_REG_ADDRESS_SCY: // 0xFF42
          return memoryController->lcdController->scy;
        case IO_REG_ADDRESS_SCX: // 0xFF43
          return memoryController->lcdController->scx;
        case IO_REG_ADDRESS_LY: // 0xFF44
          return memoryController->lcdController->ly;
        case IO_REG_ADDRESS_LYC: // 0xFF45
          return memoryController->lcdController->lyc;
        case IO_REG_ADDRESS_BGP: // 0xFF47
          return memoryController->lcdController->bgp;
        case IO_REG_ADDRESS_OBP0: // 0xFF48
          return memoryController->lcdController->obp0;
        case IO_REG_ADDRESS_OBP1: // 0xFF49
          return memoryController->lcdController->obp1;
        case IO_REG_ADDRESS_WY: // 0xFF4A
          return memoryController->lcdController->wy;
        case IO_REG_ADDRESS_WX: // 0xFF4B
          return memoryController->lcdController->wx;
        case IO_REG_ADDRESS_BCPS: // 0xFF68
          return memoryController->lcdController->bcps;
        case IO_REG_ADDRESS_BCPD: // 0xFF69
          return memoryController->lcdController->backgroundPaletteMemory[memoryController->lcdController->bcps & 0x3F];
        case IO_REG_ADDRESS_OCPS: // 0xFF6A
          return memoryController->lcdController->ocps;
        case IO_REG_ADDRESS_OCPD: // 0xFF6B
          return memoryController->lcdController->objectPaletteMemory[memoryController->lcdController->ocps & 0x3F];
      }
    } else if (address == IO_REG_ADDRESS_KEY1) { // 0xFF4D
      return memoryController->speedController->key1;
    } else if (address == IO_REG_ADDRESS_VBK) { // 0xFF4F
      return memoryController->lcdController->vbk;
    } else if (address >= IO_REG_ADDRESS_HDMA1 && address <= IO_REG_ADDRESS_HDMA5) { // 0xFF51 - 0xFF55
      switch (address) {
        case IO_REG_ADDRESS_HDMA1: // 0xFF51
          return memoryController->hdma1;
        case IO_REG_ADDRESS_HDMA2: // 0xFF52
          return memoryController->hdma2;
        case IO_REG_ADDRESS_HDMA3: // 0xFF53
          return memoryController->hdma3;
        case IO_REG_ADDRESS_HDMA4: // 0xFF54
          return memoryController->hdma4;
        case IO_REG_ADDRESS_HDMA5: // 0xFF55
          return memoryController->hdma5;
      }
    } else if (address == IO_REG_ADDRESS_SVBK) { // 0xFF70
      return memoryController->svbk;
    } else {
      warning("Read from unhandled I/O register address 0x%04X\n", address);
      return 0;
    }
  }
  else if (address >= 0xFF80 && address <= 0xFFFE) // High RAM
  {
    return memoryController->hram[address - 0xFF80];
  }
  else if (address == IO_REG_ADDRESS_IE) // Interrupt Enable Register 0xFFFF
  {
    return memoryController->interruptController->e;
  }

  warning("Read from unhandled address 0x%04X\n", address);
  return 0;
}

void commonWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address >= 0x8000 && address <= 0x9FFF) // Write to VRAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode != 3) { // LCD Controller is not reading from VRAM and OAM so write access is okay
      if (memoryController->cgbMode == COLOUR) {
        uint16_t bankOffset = (memoryController->lcdController->vbk * 8 * 1024);
        memoryController->vram[bankOffset + address - 0x8000] = value;
      } else {
        memoryController->vram[address - 0x8000] = value;
      }
    } else {
      warning("Invalid write of value 0x%02X to VRAM address 0x%04X while LCD is in Mode %u\n", value, address, lcdMode);
    }
  }
  else if (address >= 0xC000 && address <= 0xCFFF) // Write to WRAM (Bank 0)
  {
    memoryController->wram[address - 0xC000] = value;
  }
  else if (address >= 0xD000 && address <= 0xDFFF) // Write to WRAM (Banks 1-7)
  {
    if (memoryController->cgbMode == COLOUR) {
      uint16_t bankOffset = memoryController->svbk * 4 * 1024;
      memoryController->wram[bankOffset + (address - 0xD000)] = value;
    } else {
      memoryController->wram[address - 0xC000] = value;
    }
  }
  else if (address >= 0xE000 && address <= 0xFDFF) // Write to WRAM (echo)
  {
    memoryController->wram[address - 0xE000] = value;
  }
  else if (address >= 0xFE00 && address <= 0xFE9F) // Write to OAM
  {
    uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
    if (lcdMode == 0 || lcdMode == 1) { // LCD Controller is in HBLANK or VBLANK so write access is okay
      memoryController->oam[address - 0xFE00] = value;
    } else {
      warning("Invalid write of value 0x%02X to OAM address 0x%04X while LCD is in Mode %u\n", value, address, lcdMode);
    }
  }
  else if (address >= 0xFEA0 && address <= 0xFEFF) // Not Usable
  {
    warning("Write to unusable address 0x%04X\n", address);
  }
  else if (address >= 0xFF00 && address <= 0xFF7F) // I/O Ports
  {
    if (address == IO_REG_ADDRESS_P1) { // 0xFF00
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
    } else if (address >= IO_REG_ADDRESS_DIV && address <= IO_REG_ADDRESS_TAC) { // 0xFF04 - 0xFF07
      switch (address) {
        case IO_REG_ADDRESS_DIV: // 0xFF04
          memoryController->timerController->div = 0x00;
          break;
        case IO_REG_ADDRESS_TIMA: // 0xFF05
          memoryController->timerController->tima = value;
          break;
        case IO_REG_ADDRESS_TMA: // 0xFF06
          memoryController->timerController->tma = value;
          break;
        case IO_REG_ADDRESS_TAC: // 0xFF07
          memoryController->timerController->tac = value;
          break;
      }
    } else if (address == IO_REG_ADDRESS_IF) { // 0xFF0F
      memoryController->interruptController->f = value;
    } else if (address == IO_REG_ADDRESS_DMA) { // 0xFF46
      memoryController->dma = value;
      memoryController->dmaIsActive = true;
      memoryController->dmaUpdateCycles = 0;
      memoryController->dmaNextAddress = value * 0x100;
    } else if ((address >= IO_REG_ADDRESS_LCDC && address <= IO_REG_ADDRESS_LYC) ||  // 0xFF40 - 0xFF45
               (address >= IO_REG_ADDRESS_BGP  && address <= IO_REG_ADDRESS_WX)  ||  // 0xFF47 - 0xFF4B
               (address >= IO_REG_ADDRESS_BCPS && address <= IO_REG_ADDRESS_OCPD)) { // 0xFF68 - 0xFF6B
      switch (address) {
        case IO_REG_ADDRESS_LCDC: { // 0xFF40
          bool lcdEnabledBefore = memoryController->lcdController->lcdc & LCD_DISPLAY_ENABLE_BIT;
          bool lcdEnabledAfter = value & LCD_DISPLAY_ENABLE_BIT;

          memoryController->lcdController->lcdc = value;

          if (!lcdEnabledBefore && lcdEnabledAfter) {
            debug("LCD was ENABLED by write of value 0x%02X to LCDC\n", value);
          } else if (lcdEnabledBefore && !lcdEnabledAfter) {
            // Stopping LCD operation outside of the VBLANK period could damage the display hardware.
            // Nintendo is reported to reject any games that didn't follow this rule.
            uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
            if (lcdMode != 1) {
              warning("LCD was DISABLED outside of VBLANK period (clocks=%u)!\n", memoryController->lcdController->clockCycles);
              // exit(EXIT_FAILURE);
            }
            debug("LCD was DISABLED by write of value 0x%02X to LCDC\n", value);
            // memoryController->lcdController->clockCycles = 0;
          }
          break;
        }
        case IO_REG_ADDRESS_STAT: // 0xFF41
          memoryController->lcdController->stat = (value & 0xFC) | (memoryController->lcdController->stat & 0x03);
          break;
        case IO_REG_ADDRESS_SCY: // 0xFF42
          memoryController->lcdController->scy = value;
          break;
        case IO_REG_ADDRESS_SCX: // 0xFF43
          memoryController->lcdController->scx = value;
          break;
        case IO_REG_ADDRESS_LY: // 0xFF44
          memoryController->lcdController->ly = 0;
          debug("[MEMORY LOG]: WRITE TO LY!\n");
          break;
        case IO_REG_ADDRESS_LYC: // 0xFF45
          memoryController->lcdController->lyc = value;
          break;
        case IO_REG_ADDRESS_BGP: // 0xFF47
          memoryController->lcdController->bgp = value;
          break;
        case IO_REG_ADDRESS_OBP0: // 0xFF48
          memoryController->lcdController->obp0 = value;
          break;
        case IO_REG_ADDRESS_OBP1: // 0xFF49
          memoryController->lcdController->obp1 = value;
          break;
        case IO_REG_ADDRESS_WY: // 0xFF4A
          memoryController->lcdController->wy = value;
          break;
        case IO_REG_ADDRESS_WX: // 0xFF4B
          memoryController->lcdController->wx = value;
          break;
        case IO_REG_ADDRESS_BCPS: // 0xFF68
          memoryController->lcdController->bcps = value;
          break;
        case IO_REG_ADDRESS_BCPD: { // 0xFF69
          uint8_t bcps = memoryController->lcdController->bcps;
          memoryController->lcdController->backgroundPaletteMemory[bcps & 0x3F] = value;
          if (bcps & (1 << 7)) {
            memoryController->lcdController->bcps = (bcps & 0x80) | (((bcps & 0x3F) + 1) & 0x3F);
          }
          break;
        }
        case IO_REG_ADDRESS_OCPS: // 0xFF6A
          memoryController->lcdController->ocps = value;
          break;
        case IO_REG_ADDRESS_OCPD: { // 0xFF6B
          uint8_t ocps = memoryController->lcdController->ocps;
          memoryController->lcdController->objectPaletteMemory[ocps & 0x3F] = value;
          if (ocps & (1 << 7)) {
            memoryController->lcdController->ocps = (ocps & 0x80) | (((ocps & 0x3F) + 1) & 0x3F);
          }
          break;
        }
      }
    } else if (address == IO_REG_ADDRESS_KEY1) { // 0xFF4D
      memoryController->speedController->key1 = (memoryController->speedController->key1 | (value & 1));
    } else if (address == IO_REG_ADDRESS_VBK) { // 0xFF4F
      memoryController->lcdController->vbk = value & 1;
    } else if (address >= IO_REG_ADDRESS_HDMA1 && address <= IO_REG_ADDRESS_HDMA5) { // 0xFF51 - 0xFF55
      switch (address) {
        case IO_REG_ADDRESS_HDMA1: // 0xFF51
          if (!memoryController->hdmaTransfer.isActive) {
            memoryController->hdma1 = value;
          }
          break;
        case IO_REG_ADDRESS_HDMA2: // 0xFF52
          if (!memoryController->hdmaTransfer.isActive) {
            memoryController->hdma2 = value;
          }
          break;
        case IO_REG_ADDRESS_HDMA3: // 0xFF53
          if (!memoryController->hdmaTransfer.isActive) {
            memoryController->hdma3 = value;
          }
          break;
        case IO_REG_ADDRESS_HDMA4: // 0xFF54
          if (!memoryController->hdmaTransfer.isActive) {
            memoryController->hdma4 = value;
          }
          break;
        case IO_REG_ADDRESS_HDMA5: // 0xFF51
          memoryController->hdma5 = value;

          uint16_t length = ((value & 0x7F) + 1) * 16;
          uint16_t sourceStartAddr = (memoryController->hdma1 << 8) | (memoryController->hdma2 & 0xF0);
          uint16_t destinationStartAddr = 0x8000 + (((memoryController->hdma3 & 0x1F) << 8) | (memoryController->hdma4 & 0xF0));

          memoryController->hdmaTransfer.type = ((value & (1 << 7)) ? HBLANK : GENERAL);
          memoryController->hdmaTransfer.isActive = true;
          memoryController->hdmaTransfer.length = length;
          memoryController->hdmaTransfer.nextSourceAddr = sourceStartAddr;
          memoryController->hdmaTransfer.nextDestinationAddr = destinationStartAddr;
          break;
      }
    } else if (address == IO_REG_ADDRESS_SVBK) { // 0xFF70
      if (value == 0) {
        memoryController->svbk = 1;
      } else {
        memoryController->svbk = (value & 7);
      }
    } else {
      warning("Write of value 0x%02X to unhandled I/O register address 0x%04X\n", value, address);
    }
  }
  else if (address >= 0xFF80 && address <= 0xFFFE) // High RAM
  {
    memoryController->hram[address - 0xFF80] = value;
  }
  else if (address == IO_REG_ADDRESS_IE) // Interrupt Enable Register 0xFFFF
  {
    memoryController->interruptController->e = value;
  }
  else
  {
    warning("Write to unhandled address 0x%04X\n", address);
  }
}

void cartridgeUpdate(MemoryController* memoryController, uint8_t cyclesExecuted)
{
  if (memoryController->cartridgeUpdateImpl != NULL) {
    memoryController->cartridgeUpdateImpl(memoryController, cyclesExecuted);
  }
}

void dmaUpdate(MemoryController* memoryController, uint8_t cyclesExecuted)
{
  if (memoryController->dmaIsActive) {
    memoryController->dmaUpdateCycles += cyclesExecuted;

    while (memoryController->dmaUpdateCycles >= 4) {
      uint16_t sourceAddress = memoryController->dmaNextAddress;
      uint16_t destinationAddress = 0xFE00 + (sourceAddress & 0x00FF);

      // Memory can be transferred from ROM or RAM, so we need to use the MBC readByte() implementations instead of the "public" CPU methods to handle ROM and/or RAM banking.
      uint8_t value = memoryController->readByteImpl(memoryController, sourceAddress);
      memoryController->oam[destinationAddress - 0xFE00] = value;

      if ((destinationAddress + 1) < 0xFEA0) {
        memoryController->dmaNextAddress++;
      } else {
        memoryController->dmaIsActive = false;
        break;
      }
      memoryController->dmaUpdateCycles -= 4;
    }
  }
}

void hdmaUpdate(MemoryController* memoryController, uint8_t cyclesExecuted)
{
  HDMATransfer* transfer = &memoryController->hdmaTransfer;

  if (transfer->isActive) {
    if (transfer->type == GENERAL) {
      bool isDoubleSpeed = memoryController->speedController->key1 & (1 << 7);
      uint8_t numBytesToCopy = 2 * ((isDoubleSpeed) ? (cyclesExecuted / 2) : cyclesExecuted); // 2 bytes per usec

      for (int i = 0; i < numBytesToCopy && transfer->length > 0; i++, transfer->length--) {
        writeByte(memoryController, transfer->nextDestinationAddr++, readByte(memoryController, transfer->nextSourceAddr++));
      }
    } else if ((transfer->type == HBLANK) && ((memoryController->lcdController->stat & 3) == 0) && (memoryController->lcdController->ly <= 143)) {
      for (int i = 0; i < HBLANK_DMA_TRANSFER_LENGTH && transfer->length > 0; i++, transfer->length--) {
        writeByte(memoryController, transfer->nextDestinationAddr++, readByte(memoryController, transfer->nextSourceAddr++));
      }
    }

    if (transfer->length == 0) {
      transfer->isActive = false;
      memoryController->hdma5 = 0xFF;
    }
  }
}

bool generalPurposeDMAIsActive(MemoryController* memoryController)
{
  return (memoryController->hdmaTransfer.isActive) && (memoryController->hdmaTransfer.type == GENERAL);
}