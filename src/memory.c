#include <stdlib.h>

#include "cartridge.h"
#include "logging.h"
#include "memory.h"
#include "timer.h"

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

MemoryController InitMemoryController(
  uint8_t cartridgeType,
  uint8_t* memory,
  uint8_t* cartridge,
  LCDController* lcdController,
  TimerController* timerController,
  InterruptController* interruptController,
  uint32_t externalRAMSizeBytes
)
{
  switch (cartridgeType) {
    case CARTRIDGE_TYPE_ROM_ONLY:
      return InitROMOnlyMemoryController(memory, cartridge, lcdController, timerController, interruptController, externalRAMSizeBytes);
      break;
    case CARTRIDGE_TYPE_MBC1:
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM:
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM_PLUS_BATTERY:
      return InitMBC1MemoryController(memory, cartridge, lcdController, timerController, interruptController, externalRAMSizeBytes);
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
}

uint8_t CommonReadByte(MemoryController* memoryController, uint16_t address)
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

void CommonWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
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

/****************************************************************************/

uint8_t ROMOnlyReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address < CARTRIDGE_SIZE) {
    return memoryController->cartridge[address];
  } else {
    return CommonReadByte(memoryController, address);
  }
}

void ROMOnlyWriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address < CARTRIDGE_SIZE) {
    warning("Write of value 0x%02X to address 0x%04X in ROM space (0x%04X-0x%04X) on ROM Only cartridge\n", value, address, 0, CARTRIDGE_SIZE);
  } else {
    CommonWriteByte(memoryController, address, value);
  }
}

MemoryController InitROMOnlyMemoryController(
  uint8_t* memory,
  uint8_t* cartridge,
  LCDController* lcdController,
  TimerController* timerController,
  InterruptController* interruptController,
  uint32_t externalRAMSizeBytes
)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    NULL,
    0x00, // NOTE: Unused in ROM Only cartridges
    0x00, // NOTE: Unused in ROM Only cartridges
    0x01, // NOTE: Unused in ROM Only cartridges
    false, // NOTE: Unused in ROM Only cartridges
    false,
    0x0000,
    &ROMOnlyReadByte,
    &ROMOnlyWriteByte,
    lcdController,
    timerController,
    interruptController
  };
  return memoryController;
}

/****************************************************************************/

uint8_t MBC1ReadByte(MemoryController* memoryController, uint16_t address)
{
  if (address <= 0x3FFF) { // Read from ROM Bank 0
    return memoryController->cartridge[address];
  } else if (address >= 0x4000 && address <= 0x7FFF) { // Read from ROM Banks 01-7F
    uint8_t bankNumHigh = 0x00;
    if (memoryController->modeSelect == 0x00) {
      bankNumHigh = memoryController->bankSelect << 2;
    }
    uint8_t bankNum = bankNumHigh | memoryController->romBank;
    uint16_t offsetInBank = address - 0x4000;
    uint32_t romAddress = (bankNum * 1024 * 16) + offsetInBank;
    return memoryController->cartridge[romAddress];
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Read from external cartridge RAM
    if (memoryController->ramEnabled) {
      return memoryController->externalRAM[address - 0xA000];
    } else {
      warning("MBC1: Read from external RAM at address 0x%04X failed because RAM is DISABLED.\n", address);
      return 0; // TODO: What value should be returned here?
    }
  } else {
    return CommonReadByte(memoryController, address);
  }
}

void MBC1WriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  if (address <= 0x1FFF) { // External RAM Enable/Disable    
    if ((value & 0xF) == 0xA) {
      if (!memoryController->ramEnabled) {
        info("External RAM was ENABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      memoryController->ramEnabled = true;
    } else {
      if (memoryController->ramEnabled) {
        info("External RAM was DISABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      memoryController->ramEnabled = false;
    }
  } else if (address >= 0x2000 && address <= 0x3FFF) { // ROM Bank Number
    if (value == 0x00) {
      value = 0x01;
    }
    memoryController->romBank = value & 0x1F; // Select the lower 5 bits of the value
    // TODO: Add cycle cost here??
  } else if (address >= 0x4000 && address <= 0x5FFF) { // RAM Bank Number or Upper Bits of ROM Bank Number
    memoryController->bankSelect = value & 0x02;
  } else if (address >= 0x6000 && address <= 0x7FFF) { // ROM/RAM Mode Select
    memoryController->modeSelect = (address & 0x1);
    // TODO: Add cycle cost here??
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Write to external cartridge RAM
    if (memoryController->ramEnabled) {
      memoryController->externalRAM[address - 0xA000] = value;
    } else {
      warning("MBC1: Write of value 0x%02X to external RAM at address 0x%04X failed because RAM is DISABLED.\n", value, address);
    }
  } else {
    CommonWriteByte(memoryController, address, value);
  }
}

MemoryController InitMBC1MemoryController(
  uint8_t* memory,
  uint8_t* cartridge,
  LCDController* lcdController,
  TimerController* timerController,
  InterruptController* interruptController,
  uint32_t externalRAMSizeBytes
)
{
  MemoryController memoryController = {
    memory,
    cartridge,
    NULL,
    0x00, // TODO: Correct default?
    0x00, // TODO: Correct default?
    0x01,
    false,
    false,
    0x0000,
    &MBC1ReadByte,
    &MBC1WriteByte,
    lcdController,
    timerController,
    interruptController
  };
  
  // Allocate external RAM
  uint8_t* externalRAM = (uint8_t*)malloc(externalRAMSizeBytes * sizeof(uint8_t));
  if (!externalRAM) {
    critical("Failed to allocate %u bytes of external RAM for MBC1 cartridge.\n", externalRAMSizeBytes);
    exit(EXIT_FAILURE);
  }
  memoryController.externalRAM = externalRAM;
  
  return memoryController;
}