#include "cartridge.h"

#include "logging.h"

#include <stdlib.h>

int cartridgeGetSize(FILE* cartridgeFile)
{
  long int size;
  fseek(cartridgeFile, 0, SEEK_END);
  size = ftell(cartridgeFile);
  fseek(cartridgeFile, 0, SEEK_SET);
  return size;
}

uint8_t* cartridgeLoadData(const char* pathToROM)
{
  uint8_t* cartridgeData = NULL;

  FILE* cartridgeFile = fopen(pathToROM, "rb");
  if (cartridgeFile == NULL) {
    printf("Failed to read GB cartridge from '%s'\n", pathToROM);
    return NULL;
  }
  long int cartridgeSize = cartridgeGetSize(cartridgeFile);
  printf("Cartridge Size: %li\n", cartridgeSize);

  cartridgeData = (uint8_t*)malloc(cartridgeSize * sizeof(uint8_t));

  fseek(cartridgeFile, 0, SEEK_SET);
  fread(cartridgeData, 1, cartridgeSize, cartridgeFile);
  fclose(cartridgeFile);

  return cartridgeData;
}

const char* cartridgeGetGameTitle(const uint8_t* cartridgeData)
{
  int titleLength = 0;
  for (int i = GAME_TITLE_START_ADDRESS; i <= GAME_TITLE_END_ADDRESS; i++) {
    if ((cartridgeData[i] == 0) || (i == GAME_TITLE_END_ADDRESS)) {
      titleLength = i - GAME_TITLE_START_ADDRESS;
      if (i == GAME_TITLE_END_ADDRESS) {
        titleLength++; // Plus 1 because we encountered no NULL bytes and used the full length of
      }
      break;
    }
  }

  char* gameTitle = malloc((titleLength + 1) * sizeof(char));
  if (gameTitle == NULL) {
    critical("%s: malloc() failed\n", __func__);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < titleLength; i++) {
    gameTitle[i] = cartridgeData[GAME_TITLE_START_ADDRESS + i];
  }
  gameTitle[titleLength] = '\0';

  return gameTitle;
}

uint8_t cartridgeGetType(const uint8_t* cartridgeData)
{
  return cartridgeData[CARTRIDGE_TYPE_ADDRESS];
}

uint32_t RAMSizeInBytes(uint8_t ramSize)
{
  switch (ramSize) {
    case 0x0:
      return 0;
      break;
    case 0x1:
      return 2 * 1024;
      break;
    case 0x2:
      return 8 * 1024;
      break;
    case 0x3:
      return 32 * 1024;
      break;
    case 0x4:
      return 128 * 1024;
      break;
    default:
      critical("Unsupported RAM size byte %u", ramSize);
      exit(EXIT_FAILURE);
      break;
  }
}

char* ROMSizeToString(uint8_t romSize)
{
  switch (romSize) {
    case 0x0:
      return "32KB (2 banks)";
      break;
    case 0x1:
      return "64KB (4 banks)";
      break;
    case 0x2:
      return "128KB (8 banks)";
      break;
    case 0x3:
      return "256KB (16 banks)";
      break;
    case 0x4:
      return "512KB (32 banks)";
      break;
    case 0x5:
      return "1MB (64 banks)";
      break;
    case 0x6:
      return "2MB (128 banks)";
      break;
    case 0x52:
      return "1.1MB (72 banks)";
      break;
    case 0x53:
      return "1.2MB (80 banks)";
      break;
    case 0x54:
      return "1.5MB (96 banks)";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}

char* RAMSizeToString(uint8_t ramSize)
{
  switch (ramSize) {
    case 0x0:
      return "None";
      break;
    case 0x1:
      return "2KB (1 bank)";
      break;
    case 0x2:
      return "8KB (1 bank)";
      break;
    case 0x3:
      return "32KB (4 banks)";
      break;
    case 0x4:
      return "128KB (16 banks)";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}

char* DestinationCodeToString(uint8_t destinationCode)
{
  switch (destinationCode) {
    case 0x0:
      return "Japanese";
      break;
    case 0x1:
      return "Non-Japanese";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}

char* CartridgeTypeToString(uint8_t cartridgeType)
{
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