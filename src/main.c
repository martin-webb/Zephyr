#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "cpu.h"
#include "gameboy.h"
#include "speed.h"
#include "timercontroller.h"

int main(int argc, char* argv[])
{
  if (argc != 2) {
    printf("Usage: %s PATH_TO_ROM\n", argv[0]);
    return 1;
  }
  
  // Load all cartridge data
  uint8_t* cartridgeData = cartridgeLoadData(argv[1]);
  if (cartridgeData == NULL) {
    fprintf(stderr, "Failed to read cartridge from '%s'\n", argv[1]);
    exit(EXIT_FAILURE);
  }
  
  uint8_t memory[1024 * 32] = {0};
  
  uint8_t cartridgeType = cartridgeGetType(cartridgeData);
  printf("Cartridge Type: 0x%02X - %s\n", cartridgeType, CartridgeTypeToString(cartridgeType));
  
  CPU cpu;
  TimerController timerController = {
    .dividerCounter = 0,
    .timerCounter = 0
  };
  MemoryController m = InitMemoryController(cartridgeType, memory, cartridgeData, &timerController);
  GameBoyType gameBoyType = GB;
  SpeedMode speedMode = NORMAL;
  GameBoyType gameType = gbGetGameType(cartridgeData);
  
  printf("Title: ");
  for (int i = GAME_TITLE_START_ADDRESS; i < GAME_TITLE_END_ADDRESS; i++) {
    printf("%c", cartridgeData[i]);
  }
  printf("\n");
  
  printf("Game Type: %s\n", (gameType == GB) ? "GB" : (gameType == CGB) ? "CGB" : "SGB");
  
  uint8_t romSize = cartridgeData[ROM_SIZE_ADDRESS];
  printf("ROM size: 0x%02X - %s\n", romSize, ROMSizeToString(romSize));
  
  uint8_t ramSize = cartridgeData[RAM_SIZE_ADDRESS];
  printf("RAM size: 0x%02X - %s\n", ramSize, RAMSizeToString(ramSize));
  
  uint8_t destinationCode = cartridgeData[DESTINATION_CODE_ADDRESS];
  printf("Destination Code: 0x%02X - %s\n", destinationCode, DestinationCodeToString(destinationCode));
  
  cpuReset(&cpu, &m, GB);
  cpuPrintState(&cpu);
  
  while (1) {
    gbRunAtLeastNCycles(&cpu, &m, &timerController, gameBoyType, speedMode, CPU_MIN_CYCLES_PER_SET);
  }
  
  free(cartridgeData);
  
  return 0;
}