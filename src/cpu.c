#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "memory.h"

#define CLOCK_CYCLE_FREQUENCY (1024 * 1024 * 4)
#define CLOCK_CYCLE_TIME_SECS (1.0 / CLOCK_CYCLE_FREQUENCY)

#define MACHINE_CYCLE_FREQUENCY (CLOCK_CYCLE_FREQUENCY / 4)
#define MACHINE_CYCLE_TIME_SECS (1.0 / MACHINE_CYCLE_FREQUENCY)

#define FLAG_REGISTER_C_BIT 16
#define FLAG_REGISTER_H_BIT 32
#define FLAG_REGISTER_N_BIT 64
#define FLAG_REGISTER_Z_BIT 128

#define VBLANK_INTERRUPT_START_ADDRESS 0x40
#define LCDC_STATUS_INTERRUPT_START_ADDRESS 0x48
#define TIMER_OVERFLOW_INTERRUPT_START_ADDRESS 0x50 
#define SERIAL_TRANSFER_COMPLETION_INTERRUPT_START_ADDRESS 0x58
#define HIGH_TO_LOW_P10_TO_P13_INTERRUPT_START_ADDRESS 0x60

/********************** CARTRIDGE INTERNAL INFORMATION **********************/

#define NINTENDO_GRAPHIC_START_ADDRESS 0x104
#define NINTENDO_GRAPHIC_END_ADDRESS 0x133

#define GAME_TITLE_START_ADDRESS 0x134
#define GAME_TITLE_END_ADDRESS 0x142

#define COLOR_GB_FLAG_ADDRESS 0x143
#define GB_OR_SGB_FLAG_ADDRESS 0x146
#define CARTRIDGE_TYPE_ADDRESS 0x147

#define ROM_SIZE_ADDRESS 0x148
#define RAM_SIZE_ADDRESS 0x149

#define DESTINATION_CODE_ADDRESS 0x14A
#define LICENSEE_CODE_ADDRESS 0x14B
#define MASK_ROM_VERSION_NUM_ADDRESS 0x14C
#define COMPLEMENT_CHECK_ADDRESS 0x14D

#define CHECKSUM_START_ADDRESS 0x14E
#define CHECKSUM_END_ADDRESS 0x14F

/****************************************************************************/

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

typedef struct {
  unsigned char a;
  unsigned char b;
  unsigned char c;
  unsigned char d;
  unsigned char e;
  unsigned char f;
  unsigned char h;
  unsigned char l;
  
  unsigned short sp;
  unsigned short pc;
  
  unsigned char flag;
} CPURegisters;

typedef enum {
  GB,
  GBP,
  GBC,
  SGB
} GBType;

char* ColorGBIdentifierToString(unsigned char destinationCode) {
  switch (destinationCode) {
    case 0x80:
      return "Yes";
      break;
    default:
      return "No";
      break;
  }
}

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

char* ROMSizeToString(unsigned char romSize) {
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

char* RAMSizeToString(unsigned char ramSize) {
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

char* DestinationCodeToString(unsigned char destinationCode) {
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

void InitForExecution(CPURegisters* registers, MemoryController* memoryController, GBType gbType) {
  switch (gbType) {
    case GB:
    case SGB:
      registers->a = 0x00;
      registers->f = 0x01;
      break;
    case GBP:
      registers->a = 0x00;
      registers->f = 0xFF;
      break;
    case GBC:
      registers->a = 0x00;
      registers->f = 0x11;
      break;
    default:
      printf("InitForExecution(): Unknown GBType %i encountered. Exiting...", gbType);
      exit(1);
      break;
  }
  registers->flag = 0xB0;
  registers->sp = 0xFFFE;
  registers->pc = 0x100;
  
  memoryController->writeByte(0xFF05, 0x00, memoryController);
  /*
  memory[0xFF06] = 0x00;
  memory[0xFF07] = 0x00;
  memory[0xFF10] = 0x80;
  memory[0xFF11] = 0xBF;
  memory[0xFF12] = 0xF3;
  memory[0xFF14] = 0xBF;
  memory[0xFF16] = 0x3F;
  memory[0xFF17] = 0x00;
  memory[0xFF19] = 0xBF;
  memory[0xFF1A] = 0x7F;
  memory[0xFF1B] = 0xFF;
  memory[0xFF1C] = 0x9F;
  memory[0xFF1E] = 0xBF;
  memory[0xFF20] = 0xFF;
  memory[0xFF21] = 0x00;
  memory[0xFF22] = 0x00;
  memory[0xFF23] = 0xBF;
  memory[0xFF24] = 0x77;
  memory[0xFF25] = 0xF3;
  switch (gbType) {
    case GB:
      memory[0xFF26] = 0xF1;
      break;
    case SGB:
      memory[0xFF26] = 0xF0;
      break;
    default:
      // TODO: Are there values for the GBP and GBC here?
      break;
  }
  memory[0xFF40] = 0x91;
  memory[0xFF42] = 0x00;
  memory[0xFF43] = 0x00;
  memory[0xFF45] = 0x00;
  memory[0xFF47] = 0xFC;
  memory[0xFF48] = 0xFF;
  memory[0xFF49] = 0xFF;
  memory[0xFF4A] = 0x00;
  memory[0xFF4B] = 0x00;
  memory[0xFFFF] = 0x00;
  */
}

void PrintRegisters(CPURegisters* registers) {
  printf("A: 0x%02X B: 0x%02X C: 0x%02X D: 0x%02X E: 0x%02X F: 0x%02X H: 0x%02X L: 0x%02X\n",
    registers->a,
    registers->b,
    registers->c,
    registers->d,
    registers->e,
    registers->f,
    registers->h,
    registers->l
  );
  printf("SP: 0x%02X PC: 0x%02X\n",
    registers->sp,
    registers->pc
  );
  printf("FLAG: Z: %i N: %i H: %i C: %i\n",
    (registers->flag & FLAG_REGISTER_Z_BIT) >> 7,
    (registers->flag & FLAG_REGISTER_N_BIT) >> 6,
    (registers->flag & FLAG_REGISTER_H_BIT) >> 5,
    (registers->flag & FLAG_REGISTER_C_BIT) >> 4
  );
}

int GetCartridgeSize(FILE* cartridgeFile) {
  long int size;
  fseek(cartridgeFile, 0, SEEK_END);
  size = ftell(cartridgeFile);
  fseek(cartridgeFile, 0, SEEK_SET);
  return size;
}

unsigned char* LoadCartridge(char* pathToROM) {
  unsigned char* cartridgeData = NULL;
  
  FILE* cartridgeFile = fopen(pathToROM, "rb");
  if (cartridgeFile == NULL) {
    printf("Failed to read GB cartridge from '%s'\n", pathToROM);
    return NULL;
  }
  long int cartridgeSize = GetCartridgeSize(cartridgeFile);
  printf("Cartridge Size: %li\n", cartridgeSize);
  
  cartridgeData = (unsigned char*)malloc(cartridgeSize * sizeof(unsigned char));
  
  fseek(cartridgeFile, 0, SEEK_SET);
  fread(cartridgeData, 1, cartridgeSize, cartridgeFile);
  fclose(cartridgeFile);
  
  return cartridgeData;
}


int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s PATH_TO_ROM\n", argv[0]);
    return 1;
  }
  
  unsigned char* cartridgeData = LoadCartridge(argv[1]);
  if (cartridgeData == NULL) {
    printf("Failed to read cartridge from '%s'", argv[1]);
    exit(1);
  }
  
  unsigned char memory[1024 * 32]; // 32KB
  
  MemoryController memoryController = InitROMOnlyMemoryController(memory, cartridgeData);
  
  
  printf("Title: ");
  for (int i = GAME_TITLE_START_ADDRESS; i < GAME_TITLE_END_ADDRESS; i++) {
    printf("%c", memoryController.readByte(i, &memoryController));
  }
  printf("\n");
  
  unsigned char cartridgeType = memoryController.readByte(CARTRIDGE_TYPE_ADDRESS, &memoryController);
  printf("Cartridge Type: 0x%02X - %s\n", cartridgeType, CartridgeTypeToString(cartridgeType));
  
  unsigned char colorGB = memory[COLOR_GB_FLAG_ADDRESS];
  printf("Color GB: 0x%02X - %s\n", colorGB, ColorGBIdentifierToString(colorGB));
  
  unsigned char gbOrSGB = memory[GB_OR_SGB_FLAG_ADDRESS];
  printf("GB/SGB: 0x%02X - %s\n", gbOrSGB, (gbOrSGB == 0x0) ? "GB" : (gbOrSGB == 0x3) ? "SGB" : "Unknown");
  
  unsigned char romSize = memory[ROM_SIZE_ADDRESS];
  printf("ROM size: 0x%02X - %s\n", romSize, ROMSizeToString(romSize));
  
  unsigned char ramSize = memory[RAM_SIZE_ADDRESS];
  printf("RAM size: 0x%02X - %s\n", ramSize, RAMSizeToString(ramSize));
  
  unsigned char destinationCode = memory[DESTINATION_CODE_ADDRESS];
  printf("Destination Code: 0x%02X - %s\n", destinationCode, DestinationCodeToString(destinationCode));
  
  CPURegisters registers;
  
  InitForExecution(&registers, &memoryController, GB);
  PrintRegisters(&registers);
  
  while (1) {
    break;
    unsigned char opcode = memory[registers.pc++];
    printf("PC: 0x%02X Opcode: 0x%02X\n", registers.pc - 1, opcode);
    
    switch (opcode) {
      case 0x00: {// NOP
        // TODO: Cycles + 4
        // TODO: PC++?
        break;
      }
      case 0xC3: { // JP nn
        unsigned char lsb = memory[registers.pc++];
        unsigned char msb = memory[registers.pc++];
        
        registers.pc = (msb << 8) | lsb;
        // TODO: Cycles + 12
        // TODO: PC++?
        break;
      }
      default:
        printf("FATAL ERROR: ENCOUNTERED UNKNOWN OPCODE: 0x%02X\n", opcode);
        exit(1);
        break;
    }
  }
  
  free(cartridgeData);
  
  return 0;
}