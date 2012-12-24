#include "cartridge.h"

char* CartridgeTypeToString(unsigned char cartridgeType) {
  switch (cartridgeType) {
    case CARTRIDGE_TYPE_ROM_ONLY:
      return "ROM ONLY";
      break;
    case CARTRIDGE_TYPE_MBC1:
      return "MBC1";
      break;
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM:
      return "MBC1+RAM";
      break;
    case CARTRIDGE_TYPE_MBC1_PLUS_RAM_PLUS_BATTERY:
      return "MBC1+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC2:
      return "MBC2";
      break;
    case CARTRIDGE_TYPE_MBC2_PLUS_BATTERY:
      return "MBC2+BATTERY";
      break;
    case CARTRIDGE_TYPE_ROM_PLUS_RAM:
      return "ROM+RAM";
      break;
    case CARTRIDGE_TYPE_ROM_PLUS_RAM_PLUS_BATTERY:
      return "ROM+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MMM01:
      return "MMM01";
      break;
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM:
      return "MMM01+RAM";
      break;
    case CARTRIDGE_TYPE_MMM01_PLUS_RAM_PLUS_BATTERY:
      return "MMM01+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_BATTERY:
      return "MBC3+TIMER+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_RAM_PLUS_BATTERY:
      return "MBC3+TIMER+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC3:
      return "MBC3";
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM:
      return "MBC3+RAM";
      break;
    case CARTRIDGE_TYPE_MBC3_PLUS_RAM_PLUS_BATTERY:
      return "MBC3+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC4:
      return "MBC4";
      break;
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM:
      return "MBC4+RAM";
      break;
    case CARTRIDGE_TYPE_MBC4_PLUS_RAM_PLUS_BATTERY:
      return "MBC4+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC5:
      return "MBC5";
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM:
      return "MBC5+RAM";
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RAM_PLUS_BATTERY:
      return "MBC5+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE:
      return "MBC5+RUMBLE";
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM:
      return "MBC5+RUMBLE+RAM";
      break;
    case CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM_PLUS_BATTERY:
      return "MBC5+RUMBLE+RAM+BATTERY";
      break;
    case CARTRIDGE_TYPE_POCKET_CAMERA:
      return "POCKET CAMERA";
      break;
    case CARTRIDGE_TYPE_BANDAI_TAMA5:
      return "BANDAI TAMA5";
      break;
    case CARTRIDGE_TYPE_HuC3:
      return "HuC3";
      break;
    case CARTRIDGE_TYPE_HuC1_PLUS_RAM_PLUS_BATTERY:
      return "HuC1+RAM+BATTERY";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}