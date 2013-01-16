#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "cpu.h"

int main(int argc, char* argv[])
{
  if (argc != 2) {
    printf("Usage: %s PATH_TO_ROM\n", argv[0]);
    return 1;
  }
  
  uint8_t* cartridgeData = LoadCartridge(argv[1]);
  if (cartridgeData == NULL) {
    printf("Failed to read cartridge from '%s'\n", argv[1]);
    exit(1);
  }
  
  uint8_t memory[1024 * 32]; // 32KB
  
  uint8_t cartridgeType = cartridgeData[CARTRIDGE_TYPE_ADDRESS];
  printf("Cartridge Type: 0x%02X - %s\n", cartridgeType, CartridgeTypeToString(cartridgeType));
  
  CPU cpu;
  MemoryController m = InitMemoryController(cartridgeType, memory, cartridgeData);
  
  printf("Title: ");
  for (int i = GAME_TITLE_START_ADDRESS; i < GAME_TITLE_END_ADDRESS; i++) {
    printf("%c", readByte(&m, i));
  }
  printf("\n");
  
  uint8_t colorGB = readByte(&m, COLOR_GB_FLAG_ADDRESS);
  printf("Color GB: 0x%02X - %s\n", colorGB, ColorGBIdentifierToString(colorGB));
  
  uint8_t gbOrSGB = readByte(&m, GB_OR_SGB_FLAG_ADDRESS);
  printf("GB/SGB: 0x%02X - %s\n", gbOrSGB, (gbOrSGB == 0x0) ? "GB" : (gbOrSGB == 0x3) ? "SGB" : "Unknown");
  
  uint8_t romSize = readByte(&m, ROM_SIZE_ADDRESS);
  printf("ROM size: 0x%02X - %s\n", romSize, ROMSizeToString(romSize));
  
  uint8_t ramSize = readByte(&m, RAM_SIZE_ADDRESS);
  printf("RAM size: 0x%02X - %s\n", ramSize, RAMSizeToString(ramSize));
  
  uint8_t destinationCode = readByte(&m, DESTINATION_CODE_ADDRESS);
  printf("Destination Code: 0x%02X - %s\n", destinationCode, DestinationCodeToString(destinationCode));
  
  InitForExecution(&cpu, &m, GB);
  PrintCPUState(&cpu);
  
  while (1) {
    cpuRunAtLeastNCycles(&cpu, &m, CPU_MIN_CYCLES_PER_SET);
  }
  
  free(cartridgeData);
  
  return 0;
}