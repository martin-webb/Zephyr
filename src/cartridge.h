#ifndef CARTRIDGE_H_
#define CARTRIDGE_H_

#include <stdint.h>
#include <stdio.h>

#define CARTRIDGE_SIZE 0x8000

#define NINTENDO_GRAPHIC_START_ADDRESS 0x104
#define NINTENDO_GRAPHIC_END_ADDRESS 0x133

#define GAME_TITLE_START_ADDRESS 0x134
#define GAME_TITLE_END_ADDRESS 0x142

#define CGB_FLAG_ADDRESS 0x143
#define SGB_FLAG_ADDRESS 0x146
#define CARTRIDGE_TYPE_ADDRESS 0x147

#define ROM_SIZE_ADDRESS 0x148
#define RAM_SIZE_ADDRESS 0x149

#define DESTINATION_CODE_ADDRESS 0x14A
#define LICENSEE_CODE_ADDRESS 0x14B
#define MASK_ROM_VERSION_NUM_ADDRESS 0x14C
#define COMPLEMENT_CHECK_ADDRESS 0x14D

#define CHECKSUM_START_ADDRESS 0x14E
#define CHECKSUM_END_ADDRESS 0x14F

#define CARTRIDGE_TYPE_ROM_ONLY 0x0
#define CARTRIDGE_TYPE_MBC1 0x1
#define CARTRIDGE_TYPE_MBC1_PLUS_RAM 0x2
#define CARTRIDGE_TYPE_MBC1_PLUS_RAM_PLUS_BATTERY 0x3
#define CARTRIDGE_TYPE_MBC2 0x5
#define CARTRIDGE_TYPE_MBC2_PLUS_BATTERY 0x6
#define CARTRIDGE_TYPE_ROM_PLUS_RAM 0x8
#define CARTRIDGE_TYPE_ROM_PLUS_RAM_PLUS_BATTERY 0x9
#define CARTRIDGE_TYPE_MMM01 0xB
#define CARTRIDGE_TYPE_MMM01_PLUS_RAM 0xC
#define CARTRIDGE_TYPE_MMM01_PLUS_RAM_PLUS_BATTERY 0xD
#define CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_BATTERY 0xF
#define CARTRIDGE_TYPE_MBC3_PLUS_TIMER_PLUS_RAM_PLUS_BATTERY 0x10
#define CARTRIDGE_TYPE_MBC3 0x11
#define CARTRIDGE_TYPE_MBC3_PLUS_RAM 0x12
#define CARTRIDGE_TYPE_MBC3_PLUS_RAM_PLUS_BATTERY 0x13
#define CARTRIDGE_TYPE_MBC4 0x15
#define CARTRIDGE_TYPE_MBC4_PLUS_RAM 0x16
#define CARTRIDGE_TYPE_MBC4_PLUS_RAM_PLUS_BATTERY 0x17
#define CARTRIDGE_TYPE_MBC5 0x19
#define CARTRIDGE_TYPE_MBC5_PLUS_RAM 0x1A
#define CARTRIDGE_TYPE_MBC5_PLUS_RAM_PLUS_BATTERY 0x1B
#define CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE 0x1C
#define CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM 0x1D
#define CARTRIDGE_TYPE_MBC5_PLUS_RUMBLE_PLUS_RAM_PLUS_BATTERY 0x1E
#define CARTRIDGE_TYPE_POCKET_CAMERA 0xFC
#define CARTRIDGE_TYPE_BANDAI_TAMA5 0xFD
#define CARTRIDGE_TYPE_HuC3 0xFE
#define CARTRIDGE_TYPE_HuC1_PLUS_RAM_PLUS_BATTERY 0xFF

int cartridgeGetSize(FILE* cartridgeFile);
uint8_t* cartridgeLoadData(char* pathToROM);
uint8_t cartridgeGetType(uint8_t* cartridgeData);

uint32_t RAMSizeInBytes(uint8_t ramSize);

char* ROMSizeToString(uint8_t romSize);
char* RAMSizeToString(uint8_t ramSize);
char* DestinationCodeToString(uint8_t destinationCode);
char* CartridgeTypeToString(uint8_t cartridgeType);

#endif // CARTRIDGE_H_