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
  SoundController* soundController,
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
    soundController,
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


uint8_t vramReadByte(MemoryController* memoryController, uint16_t address)
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


uint8_t wramReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address >= 0xC000 && address <= 0xCFFF) { // Read from WRAM (Bank 0)
    return memoryController->wram[address - 0xC000];
  } else if (address >= 0xD000 && address <= 0xDFFF) { // Read from WRAM (Banks 1-7)
    if (memoryController->cgbMode == COLOUR) {
      uint16_t bankOffset = memoryController->svbk * 4 * 1024;
      return memoryController->wram[bankOffset + (address - 0xD000)];
    } else {
      return memoryController->wram[address - 0xC000];
    }
  } else if (address >= 0xE000 && address <= 0xFDFF) { // Read from WRAM (echo)
    return memoryController->wram[address - 0xE000];
  } else {
    return 0x00;
  }
}


uint8_t oamReadByte(MemoryController* memoryController, uint16_t address)
{
  uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
  if (lcdMode == 0 || lcdMode == 1) { // LCD Controller is in HBLANK or VBLANK so read access is okay
    return memoryController->oam[address - 0xFE00];
  } else {
    return 0xFF;
  }
}


uint8_t dmaReadByte(MemoryController* memoryController, uint16_t address)
{
  warning("Read from DMA I/O register (write only).\n");
  return memoryController->dma;
}


uint8_t hdmaReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address == IO_REG_ADDRESS_HDMA1) { // 0xFF51
    return memoryController->hdma1;
  } else if (address == IO_REG_ADDRESS_HDMA2) { // 0xFF52
    return memoryController->hdma2;
  } else if (address == IO_REG_ADDRESS_HDMA3) { // 0xFF53
    return memoryController->hdma3;
  } else if (address == IO_REG_ADDRESS_HDMA4) { // 0xFF54
    return memoryController->hdma4;
  } else if (address == IO_REG_ADDRESS_HDMA5) { // 0xFF55
    return memoryController->hdma5;
  } else {
    return 0x00;
  }
}


uint8_t svbkReadByte(MemoryController* memoryController, uint16_t address)
{
  return memoryController->svbk;
}


uint8_t hramReadByte(MemoryController* memoryController, uint16_t address)
{
  return memoryController->hram[address - 0xFF80];
}


uint8_t commonReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address >= 0x8000 && address <= 0x9FFF) { // Read from VRAM
    return vramReadByte(memoryController, address);
  } else if (address >= 0xC000 && address <= 0xFDFF) { // Read from WRAM
    return wramReadByte(memoryController, address);
  } else if (address >= 0xFE00 && address <= 0xFE9F) { // Read from OAM
    return oamReadByte(memoryController, address);
  } else if (address >= 0xFEA0 && address <= 0xFEFF) { // Not Usable
    warning("Read from unusable address 0x%04X\n", address);
    return 0;
  } else if (address >= 0xFF00 && address <= 0xFF7F) { // I/O Ports
    if (address == IO_REG_ADDRESS_P1) { // 0xFF00
      return joypadReadByte(memoryController->joypadController, address);
    } else if (address >= IO_REG_ADDRESS_DIV && address <= IO_REG_ADDRESS_TAC) { // 0xFF04 - 0xFF07
      return timerReadByte(memoryController->timerController, address);
    } else if (address == IO_REG_ADDRESS_IF) { // 0xFF0F
      return interruptReadByte(memoryController->interruptController, address);
    } else if (address == 0xFF1A) {
      return 0x80;
    } else if ((address >= IO_REG_ADDRESS_NR10 && address <= IO_REG_ADDRESS_NR52) || // 0xFF10 - 0xFF26
               (address >= 0xFF27 && address <= 0xFF2F) || // 0xFF27 - 0xFF2F
               (address >= IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN && address <= IO_REG_ADDRESS_WAVE_PATTERN_RAM_END)) { // 0xFF30 - 0xFF3F
      return soundControllerReadByte(memoryController->soundController, address);
    } else if (address == IO_REG_ADDRESS_DMA) { // 0xFF46
      return dmaReadByte(memoryController, address);
    } else if ((address >= IO_REG_ADDRESS_LCDC && address <= IO_REG_ADDRESS_LYC)  ||  // 0xFF40 - 0xFF45
               (address >= IO_REG_ADDRESS_BGP  && address <= IO_REG_ADDRESS_WX)   ||  // 0xFF47 - 0xFF4B
               (address >= IO_REG_ADDRESS_BCPS && address <= IO_REG_ADDRESS_OCPD) ||  // 0xFF68 - 0xFF6B
               (address == IO_REG_ADDRESS_VBK)) {                                     // 0xFF4F
      return lcdReadByte(memoryController->lcdController, address);
    } else if (address == IO_REG_ADDRESS_KEY1) { // 0xFF4D
      return speedReadByte(memoryController->speedController, address);
    } else if (address >= IO_REG_ADDRESS_HDMA1 && address <= IO_REG_ADDRESS_HDMA5) { // 0xFF51 - 0xFF55
      return hdmaReadByte(memoryController, address);
    } else if (address == IO_REG_ADDRESS_SVBK) { // 0xFF70
      return svbkReadByte(memoryController, address);
    } else {
      warning("Read from unhandled I/O register address 0x%04X\n", address);
      return 0;
    }
  } else if (address >= 0xFF80 && address <= 0xFFFE) { // High RAM
    return hramReadByte(memoryController, address);
  } else if (address == IO_REG_ADDRESS_IE) { // Interrupt Enable Register 0xFFFF
    return interruptReadByte(memoryController->interruptController, address);
  }

  warning("Read from unhandled address 0x%04X\n", address);
  return 0;
}


void vramWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
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


void wramWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address >= 0xC000 && address <= 0xCFFF) { // Write to WRAM (Bank 0)
    memoryController->wram[address - 0xC000] = value;
  } else if (address >= 0xD000 && address <= 0xDFFF) { // Write to WRAM (Banks 1-7)
    if (memoryController->cgbMode == COLOUR) {
      uint16_t bankOffset = memoryController->svbk * 4 * 1024;
      memoryController->wram[bankOffset + (address - 0xD000)] = value;
    } else {
      memoryController->wram[address - 0xC000] = value;
    }
  } else if (address >= 0xE000 && address <= 0xFDFF) { // Write to WRAM (echo)
    memoryController->wram[address - 0xE000] = value;
  }
}


void oamWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  uint8_t lcdMode = memoryController->lcdController->stat & STAT_MODE_FLAG_BITS;
  if (lcdMode == 0 || lcdMode == 1) { // LCD Controller is in HBLANK or VBLANK so write access is okay
    memoryController->oam[address - 0xFE00] = value;
  } else {
    warning("Invalid write of value 0x%02X to OAM address 0x%04X while LCD is in Mode %u\n", value, address, lcdMode);
  }
}


void dmaWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  memoryController->dma = value;
  memoryController->dmaIsActive = true;
  memoryController->dmaUpdateCycles = 0;
  memoryController->dmaNextAddress = value * 0x100;
}


void hdmaWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_HDMA1) { // 0xFF51
    if (!memoryController->hdmaTransfer.isActive) {
      memoryController->hdma1 = value;
    }
  } else if (address == IO_REG_ADDRESS_HDMA2) { // 0xFF52
    if (!memoryController->hdmaTransfer.isActive) {
      memoryController->hdma2 = value;
    }
  } else if (address == IO_REG_ADDRESS_HDMA3) { // 0xFF53
    if (!memoryController->hdmaTransfer.isActive) {
      memoryController->hdma3 = value;
    }
  } else if (address == IO_REG_ADDRESS_HDMA4) { // 0xFF54
    if (!memoryController->hdmaTransfer.isActive) {
      memoryController->hdma4 = value;
    }
  } else if (address == IO_REG_ADDRESS_HDMA5) { // 0xFF55
    // TODO: What happens if an HDMA transfer is active?
    memoryController->hdma5 = value;

    uint16_t length = ((value & 0x7F) + 1) * 16;
    uint16_t sourceStartAddr = (memoryController->hdma1 << 8) | (memoryController->hdma2 & 0xF0);
    uint16_t destinationStartAddr = 0x8000 + (((memoryController->hdma3 & 0x1F) << 8) | (memoryController->hdma4 & 0xF0));

    memoryController->hdmaTransfer.type = ((value & (1 << 7)) ? HBLANK : GENERAL);
    memoryController->hdmaTransfer.isActive = true;
    memoryController->hdmaTransfer.length = length;
    memoryController->hdmaTransfer.nextSourceAddr = sourceStartAddr;
    memoryController->hdmaTransfer.nextDestinationAddr = destinationStartAddr;
  }
}


void svbkWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (value == 0) {
    memoryController->svbk = 1;
  } else {
    memoryController->svbk = (value & 7);
  }
}


void hramWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  memoryController->hram[address - 0xFF80] = value;
}


void commonWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address >= 0x8000 && address <= 0x9FFF) { // Write to VRAM
    vramWriteByte(memoryController, address, value);
  } else if (address >= 0xC000 && address <= 0xFDFF) { // Write to WRAM
    wramWriteByte(memoryController, address, value);
  } else if (address >= 0xFE00 && address <= 0xFE9F) { // Write to OAM
    oamWriteByte(memoryController, address, value);
  } else if (address >= 0xFEA0 && address <= 0xFEFF) { // Not Usable
    warning("Write to unusable address 0x%04X\n", address);
  } else if (address >= 0xFF00 && address <= 0xFF7F) { // I/O Ports
    if (address == IO_REG_ADDRESS_P1) { // 0xFF00
      joypadWriteByte(memoryController->joypadController, address, value);
    } else if (address >= IO_REG_ADDRESS_DIV && address <= IO_REG_ADDRESS_TAC) { // 0xFF04 - 0xFF07
      timerWriteByte(memoryController->timerController, address, value);
    } else if (address == IO_REG_ADDRESS_IF) { // 0xFF0F
      interruptWriteByte(memoryController->interruptController, address, value);
    } else if ((address >= IO_REG_ADDRESS_NR10 && address <= IO_REG_ADDRESS_NR52) || // 0xFF10 - 0xFF26
               (address >= 0xFF27 && address <= 0xFF2F) || // 0xFF27 - 0xFF2F
               (address >= IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN && address <= IO_REG_ADDRESS_WAVE_PATTERN_RAM_END)) { // 0xFF30 - 0xFF3F
      soundControllerWriteByte(memoryController->soundController, address, value);
    } else if (address == IO_REG_ADDRESS_DMA) { // 0xFF46
      dmaWriteByte(memoryController, address, value);
    } else if ((address >= IO_REG_ADDRESS_LCDC && address <= IO_REG_ADDRESS_LYC)  ||  // 0xFF40 - 0xFF45
               (address >= IO_REG_ADDRESS_BGP  && address <= IO_REG_ADDRESS_WX)   ||  // 0xFF47 - 0xFF4B
               (address >= IO_REG_ADDRESS_BCPS && address <= IO_REG_ADDRESS_OCPD) ||  // 0xFF68 - 0xFF6B
               (address == IO_REG_ADDRESS_VBK)) {                                     // 0xFF4F
      lcdWriteByte(memoryController->lcdController, address, value);
    } else if (address == IO_REG_ADDRESS_KEY1) { // 0xFF4D
      speedWriteByte(memoryController->speedController, address, value);
    } else if (address >= IO_REG_ADDRESS_HDMA1 && address <= IO_REG_ADDRESS_HDMA5) { // 0xFF51 - 0xFF55
      hdmaWriteByte(memoryController, address, value);
    } else if (address == IO_REG_ADDRESS_SVBK) { // 0xFF70
      svbkWriteByte(memoryController, address, value);
    } else {
      warning("Write of value 0x%02X to unhandled I/O register address 0x%04X\n", value, address);
    }
  } else if (address >= 0xFF80 && address <= 0xFFFE) { // High RAM
    hramWriteByte(memoryController, address, value);
  } else if (address == IO_REG_ADDRESS_IE) { // Interrupt Enable Register 0xFFFF
    interruptWriteByte(memoryController->interruptController, address, value);
  } else {
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
