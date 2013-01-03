#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cartridge.h"
#include "memory.h"

#define CLOCK_CYCLE_FREQUENCY (1024 * 1024 * 4)
#define CLOCK_CYCLE_TIME_SECS (1.0 / CLOCK_CYCLE_FREQUENCY)

#define MACHINE_CYCLE_FREQUENCY (CLOCK_CYCLE_FREQUENCY / 4)
#define MACHINE_CYCLE_TIME_SECS (1.0 / MACHINE_CYCLE_FREQUENCY)

#define FLAG_REGISTER_C_BIT_SHIFT 4
#define FLAG_REGISTER_H_BIT_SHIFT 5
#define FLAG_REGISTER_N_BIT_SHIFT 6
#define FLAG_REGISTER_Z_BIT_SHIFT 7

#define FLAG_REGISTER_C_BIT (0x1 << FLAG_REGISTER_C_BIT_SHIFT)
#define FLAG_REGISTER_H_BIT (0x1 << FLAG_REGISTER_H_BIT_SHIFT)
#define FLAG_REGISTER_N_BIT (0x1 << FLAG_REGISTER_N_BIT_SHIFT)
#define FLAG_REGISTER_Z_BIT (0x1 << FLAG_REGISTER_Z_BIT_SHIFT)

#define VBLANK_INTERRUPT_START_ADDRESS 0x40
#define LCDC_STATUS_INTERRUPT_START_ADDRESS 0x48
#define TIMER_OVERFLOW_INTERRUPT_START_ADDRESS 0x50 
#define SERIAL_TRANSFER_COMPLETION_INTERRUPT_START_ADDRESS 0x58
#define HIGH_TO_LOW_P10_TO_P13_INTERRUPT_START_ADDRESS 0x60

/* Cartridge Internal Information ***************************************************************/

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

/* Opcode Generation Macros *********************************************************************/

#define MAKE_ADD_A_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = registers.a; \
  uint32_t new = old + registers.SOURCE_REGISTER; \
  registers.a = new; \
  registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  registers.f |= (((old & 0xF) + (new & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT; \
  registers.f |= (((old & 0xFF) + (new & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_ADC_A_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = registers.a; \
  uint32_t new = old + registers.SOURCE_REGISTER + ((registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT); \
  registers.a = new; \
  registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  registers.f |= (((old & 0xF) + (new & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT; \
  registers.f |= (((old & 0xFF) + (new & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_SUB_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = registers.a; \
  int32_t new = old - registers.SOURCE_REGISTER; \
  registers.a = new; \
  registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT; \
  registers.f |= ((new & 0xF) >= 0x0) << FLAG_REGISTER_H_BIT_SHIFT; \
  registers.f |= ((new & 0xFF) >= 0x00) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;
  
#define MAKE_SBC_A_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = registers.a; \
  int32_t new = old - (registers.SOURCE_REGISTER + ((registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT)); \
  registers.a = new; \
  registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT; \
  registers.f |= ((new & 0xF) >= 0x0) << FLAG_REGISTER_H_BIT_SHIFT; \
  registers.f |= ((new & 0xFF) >= 0x00) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

/************************************************************************************************/

typedef struct {
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t sp;
  uint16_t pc;
} CPURegisters;

typedef enum {
  GB,
  GBP,
  GBC,
  SGB
} GBType;

char* ColorGBIdentifierToString(uint8_t destinationCode) {
  switch (destinationCode) {
    case 0x80:
      return "Yes";
      break;
    default:
      return "No";
      break;
  }
}

char* ROMSizeToString(uint8_t romSize) {
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

char* RAMSizeToString(uint8_t ramSize) {
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

char* DestinationCodeToString(uint8_t destinationCode) {
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

void InitForExecution(CPURegisters* registers, MemoryController* m, GBType gbType) {
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
  registers->f = 0xB0;
  registers->sp = 0xFFFE;
  registers->pc = 0x100;
  
  writeByte(m, 0xFF05, 0x00);
  writeByte(m, 0xFF06, 0x00);
  writeByte(m, 0xFF07, 0x00);
  writeByte(m, 0xFF10, 0x80);
  writeByte(m, 0xFF11, 0xBF);
  writeByte(m, 0xFF12, 0xF3);
  writeByte(m, 0xFF14, 0xBF);
  writeByte(m, 0xFF16, 0x3F);
  writeByte(m, 0xFF17, 0x00);
  writeByte(m, 0xFF19, 0xBF);
  writeByte(m, 0xFF1A, 0x7F);
  writeByte(m, 0xFF1B, 0xFF);
  writeByte(m, 0xFF1C, 0x9F);
  writeByte(m, 0xFF1E, 0xBF);
  writeByte(m, 0xFF20, 0xFF);
  writeByte(m, 0xFF21, 0x00);
  writeByte(m, 0xFF22, 0x00);
  writeByte(m, 0xFF23, 0xBF);
  writeByte(m, 0xFF24, 0x77);
  writeByte(m, 0xFF25, 0xF3);
  switch (gbType) {
    case GB:
      writeByte(m, 0xFF26, 0xF1);
      break;
    case SGB:
      writeByte(m, 0xFF26, 0xF0);
      break;
    default:
      // TODO: Are there values for the GBP and GBC here?
      break;
  }
  writeByte(m, 0xFF40, 0x91);
  writeByte(m, 0xFF42, 0x00);
  writeByte(m, 0xFF43, 0x00);
  writeByte(m, 0xFF45, 0x00);
  writeByte(m, 0xFF47, 0xFC);
  writeByte(m, 0xFF48, 0xFF);
  writeByte(m, 0xFF49, 0xFF);
  writeByte(m, 0xFF4A, 0x00);
  writeByte(m, 0xFF4B, 0x00);
  writeByte(m, 0xFFFF, 0x00);
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
    (registers->f & FLAG_REGISTER_Z_BIT) >> 7,
    (registers->f & FLAG_REGISTER_N_BIT) >> 6,
    (registers->f & FLAG_REGISTER_H_BIT) >> 5,
    (registers->f & FLAG_REGISTER_C_BIT) >> 4
  );
}

int GetCartridgeSize(FILE* cartridgeFile) {
  long int size;
  fseek(cartridgeFile, 0, SEEK_END);
  size = ftell(cartridgeFile);
  fseek(cartridgeFile, 0, SEEK_SET);
  return size;
}

uint8_t* LoadCartridge(char* pathToROM) {
  uint8_t* cartridgeData = NULL;
  
  FILE* cartridgeFile = fopen(pathToROM, "rb");
  if (cartridgeFile == NULL) {
    printf("Failed to read GB cartridge from '%s'\n", pathToROM);
    return NULL;
  }
  long int cartridgeSize = GetCartridgeSize(cartridgeFile);
  printf("Cartridge Size: %li\n", cartridgeSize);
  
  cartridgeData = (uint8_t*)malloc(cartridgeSize * sizeof(uint8_t));
  
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
  
  uint8_t* cartridgeData = LoadCartridge(argv[1]);
  if (cartridgeData == NULL) {
    printf("Failed to read cartridge from '%s'", argv[1]);
    exit(1);
  }
  
  uint8_t memory[1024 * 32]; // 32KB
  
  uint8_t cartridgeType = cartridgeData[CARTRIDGE_TYPE_ADDRESS];
  printf("Cartridge Type: 0x%02X - %s\n", cartridgeType, CartridgeTypeToString(cartridgeType));
  
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
  
  CPURegisters registers;
  
  InitForExecution(&registers, &m, GB);
  PrintRegisters(&registers);
  
  while (1) {
    uint8_t opcode = readByte(&m, registers.pc++);
    // printf("PC: 0x%02X Opcode: 0x%02X\n", registers.pc - 1, opcode);
    
    // TODO: Check for overflow of opcode here?
    
    uint32_t cycles = 0;
    switch (opcode) {
      /* 8-Bit Loads ****************************************************************************/
      /* LD nn, n ------------------------------------------------------------------------------*/
      // TODO: Check if all of these are correct
      case 0x06: { // LD B, n
        registers.b = readByte(&m, registers.pc++);
        cycles += 8;
        break;
      }
      case 0x0E: { // LD C, n
        registers.c = readByte(&m, registers.pc++);
        cycles += 8;
        break;
      }
      case 0x16: { // LD D, n
        registers.d = readByte(&m, registers.pc++);
        cycles += 8;
        break;
      }
      case 0x1E: { // LD E, n
        registers.e = readByte(&m, registers.pc++);
        cycles += 8;
        break;
      }
      case 0x26: { // LD H, n
        registers.h = readByte(&m, registers.pc++);
        cycles += 8;
        break;
      }
      case 0x2E: { // LD L, n
        registers.l = readByte(&m, registers.pc++);
        cycles += 8;
        break;
      }
      
      /* LD r1, r2 -----------------------------------------------------------------------------*/
      case 0x7F: { // LD A, A
        registers.a = registers.a;
        cycles += 4;
        break;
      }
      case 0x78: { // LD A, B
        registers.a = registers.b;
        cycles += 4;
        break;
      }
      case 0x79: { // LD A, C
        registers.a = registers.c;
        cycles += 4;
        break;
      }
      case 0x7A: { // LD A, D
        registers.a = registers.d;
        cycles += 4;
        break;
      }
      case 0x7B: { // LD A, E
        registers.a = registers.e;
        cycles += 4;
        break;
      }
      case 0x7C: { // LD A, H
        registers.a = registers.h;
        cycles += 4;
        break;
      }
      case 0x7D: { // LD A, L
        registers.a = registers.l;
        cycles += 4;
        break;
      }
      case 0x7E: { // LD A, (HL)
        registers.a = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      
      case 0x40: { // LD B, B
        registers.b = registers.b;
        cycles += 4;
        break;
      }
      case 0x41: { // LD B, C
        registers.b = registers.c;
        cycles += 4;
        break;
      }
      case 0x42: { // LD B, D
        registers.b = registers.d;
        cycles += 4;
        break;
      }
      case 0x43: { // LD B, E
        registers.b = registers.e;
        cycles += 4;
        break;
      }
      case 0x44: { // LD B, H
        registers.b = registers.h;
        cycles += 4;
        break;
      }
      case 0x45: { // LD B, L
        registers.b = registers.l;
        cycles += 4;
        break;
      }
      case 0x46: { // LD B, (HL)
        registers.b = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      
      case 0x48: { // LD C, B
        registers.c = registers.b;
        cycles += 4;
        break;
      }
      case 0x49: { // LD C, C
        registers.c = registers.c;
        cycles += 4;
        break;
      }
      case 0x4A: { // LD C, D
        registers.c = registers.d;
        cycles += 4;
        break;
      }
      case 0x4B: { // LD C, E
        registers.c = registers.e;
        cycles += 4;
        break;
      }
      case 0x4C: { // LD C, H
        registers.c = registers.h;
        cycles += 4;
        break;
      }
      case 0x4D: { // LD C, L
        registers.c = registers.l;
        cycles += 4;
        break;
      }
      case 0x4E: { // LD C, (HL)
        registers.c = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      
      case 0x50: { // LD D, B
        registers.d = registers.b;
        cycles += 4;
        break;
      }
      case 0x51: { // LD D, C
        registers.d = registers.c;
        cycles += 4;
        break;
      }
      case 0x52: { // LD D, D
        registers.d = registers.d;
        cycles += 4;
        break;
      }
      case 0x53: { // LD D, E
        registers.d = registers.e;
        cycles += 4;
        break;
      }
      case 0x54: { // LD D, H
        registers.d = registers.h;
        cycles += 4;
        break;
      }
      case 0x55: { // LD D, L
        registers.d = registers.l;
        cycles += 4;
        break;
      }
      case 0x56: { // LD D, (HL)
        registers.d = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      
      case 0x58: { // LD E, B
        registers.e = registers.b;
        cycles += 4;
        break;
      }
      case 0x59: { // LD E, C
        registers.e = registers.c;
        cycles += 4;
        break;
      }
      case 0x5A: { // LD E, D
        registers.e = registers.d;
        cycles += 4;
        break;
      }
      case 0x5B: { // LD E, E
        registers.e = registers.e;
        cycles += 4;
        break;
      }
      case 0x5C: { // LD E, H
        registers.e = registers.h;
        cycles += 4;
        break;
      }
      case 0x5D: { // LD E, L
        registers.e = registers.l;
        cycles += 4;
        break;
      }
      case 0x5E: { // LD E, (HL)
        registers.e = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      
      case 0x60: { // LD H, B
        registers.h = registers.b;
        cycles += 4;
        break;
      }
      case 0x61: { // LD H, C
        registers.h = registers.c;
        cycles += 4;
        break;
      }
      case 0x62: { // LD H, D
        registers.h = registers.d;
        cycles += 4;
        break;
      }
      case 0x63: { // LD H, E
        registers.h = registers.e;
        cycles += 4;
        break;
      }
      case 0x64: { // LD H, H
        registers.h = registers.h;
        cycles += 4;
        break;
      }
      case 0x65: { // LD H, L
        registers.h = registers.l;
        cycles += 4;
        break;
      }
      case 0x66: { // LD H, (HL)
        registers.h = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      
      case 0x68: { // LD L, B
        registers.l = registers.b;
        cycles += 4;
        break;
      }
      case 0x69: { // LD L, C
        registers.l = registers.c;
        cycles += 4;
        break;
      }
      case 0x6A: { // LD L, D
        registers.l = registers.d;
        cycles += 4;
        break;
      }
      case 0x6B: { // LD L, E
        registers.l = registers.e;
        cycles += 4;
        break;
      }
      case 0x6C: { // LD L, H
        registers.l = registers.h;
        cycles += 4;
        break;
      }
      case 0x6D: { // LD L, L
        registers.l = registers.l;
        cycles += 4;
        break;
      }
      case 0x6E: { // LD L, (HL)
        registers.l = readByte(&m, (registers.h << 8) | registers.l);
        cycles += 8;
        break;
      }
      
      // TODO: Especially these
      case 0x70: { // LD (HL), B
        writeByte(&m, (registers.h << 8) | registers.l, registers.b);
        cycles += 8;
        break;
      }
      case 0x71: { // LD (HL), C
        writeByte(&m, (registers.h << 8) | registers.l, registers.c);
        cycles += 8;
        break;
      }
      case 0x72: { // LD (HL), D
        writeByte(&m, (registers.h << 8) | registers.l, registers.d);
        cycles += 8;
        break;
      }
      case 0x73: { // LD (HL), E
        writeByte(&m, (registers.h << 8) | registers.l, registers.e);
        cycles += 8;
        break;
      }
      case 0x74: { // LD (HL), H
        writeByte(&m, (registers.h << 8) | registers.l, registers.h);
        cycles += 8;
        break;
      }
      case 0x75: { // LD (HL), L
        writeByte(&m, (registers.h << 8) | registers.l, registers.l);
        cycles += 8;
        break;
      }
      case 0x36: { // LD (HL), n
        // TODO: Check this line?
        writeByte(&m, (registers.h << 8) | registers.l, readByte(&m, registers.pc++));
        cycles += 12;
        break;
      }
      
      /* LD A, n -------------------------------------------------------------------------------*/
      // NOTE: The GB CPU Manual contained duplicates of the following opcodes here: 7F, 78, 79, 7A, 7B, 7C, 7D, 0A, 1A, 7E, FA and 3E
      case 0x0A: { // LD A, (BC)
        registers.a = readByte(&m, (registers.b << 8) | registers.c);
        cycles += 8;
        break;
      }
      case 0x1A: { // LD A, (DE)
        registers.a = readByte(&m, (registers.d << 8) | registers.e);
        cycles += 8;
        break;
      }
      case 0xFA: { // LD A, (nn)
        registers.a = readByte(&m, readWord(&m, registers.pc));
        registers.pc += 2;
        cycles += 16;
        break;
      }
      case 0x3E: { // LD A, #
        registers.a = readByte(&m, registers.pc++); // TODO Check this
        cycles += 8;
        break;
      }
      
      /* LD n, A -------------------------------------------------------------------------------*/
      // NOTE: The GB CPU Manual contained duplicates of the following opcodes here: 7F
      case 0x47: { // LD B, A
        registers.b = registers.a;
        cycles += 4;
        break;
      }
      case 0x4F: { // LD C, A
        registers.c = registers.a;
        cycles += 4;
        break;
      }
      case 0x57: { // LD D, A
        registers.d = registers.a;
        cycles += 4;
        break;
      }
      case 0x5F: { // LD E, A
        registers.e = registers.a;
        cycles += 4;
        break;
      }
      case 0x67: { // LD H, A
        registers.h = registers.a;
        cycles += 4;
        break;
      }
      case 0x6F: { // LD L, A
        registers.l = registers.a;
        cycles += 4;
        break;
      }
      case 0x02: { // LD (BC), A
        writeByte(&m, (registers.b << 8) | registers.c, registers.a);
        cycles += 8;
        break;
      }
      case 0x12: { // LD (DE), A
        writeByte(&m, (registers.d << 8) | registers.e, registers.a);
        cycles += 8;
        break;
      }
      case 0x77: { // LD (HL), A
        writeByte(&m, (registers.h << 8) | registers.l, registers.a);
        cycles += 8;
        break;
      }
      case 0xEA: { // LD (HL), A
        writeByte(&m, readWord(&m, registers.pc), registers.a);
        registers.pc += 2;
        cycles += 16;
        break;
      }
      
      /* LD A, (C) -----------------------------------------------------------------------------*/
      case 0xF2: { // LD A, (C)
        registers.a = readByte(&m, 0xFF00 + registers.c);
        cycles += 8;
        break;
      }
      
      /* LD (C), A -----------------------------------------------------------------------------*/
      case 0xE2: { // LD (C), A
        writeByte(&m, 0xFF00 + registers.c, registers.a);
        cycles += 8;
        break;
      }
      
      /* LD A, (HLD) - Same as LDD A, (HL) -----------------------------------------------------*/
      /* LD A, (HL-) - Same as LDD A, (HL) -----------------------------------------------------*/
      /* LDD A, (HL) ---------------------------------------------------------------------------*/
      case 0x3A: { // LD A, (HLD), LD A, (HL-) and LDD A, (HL)
        registers.a = readByte(&m, (registers.h << 8) | registers.l);
        registers.l--;
        if (registers.l == 0xFF) { // If the resulting value is 255 then the previous value must have been 0 so also decrement H
          registers.h--;
        }
        cycles += 8;
        break;
      }
      
      /* LD (HLD), A - Same as LDD (HL), A -----------------------------------------------------*/
      /* LD (HL-), A - Same as LDD (HL), A -----------------------------------------------------*/
      /* LDD (HL), A ---------------------------------------------------------------------------*/
      case 0x32: { // LD (HLD), A, LD (HL-), A and LDD (HL), A
        writeByte(&m, (registers.h) << 8 | registers.l, registers.a);
        registers.l--;
        if (registers.l == 0xFF) { // If the resulting value is 255 then the previous value must have been 0 so also decrement H
          registers.h--;
        }
        cycles += 8;
        break;
      }
      
      /* LD A, (HLI) - Same as LDI A, (HL) -----------------------------------------------------*/
      /* LD A, (HL+) - Same as LDI A, (HL) -----------------------------------------------------*/
      /* LDI A, (HL) ---------------------------------------------------------------------------*/
      case 0x2A: { // LD A, (HLI), LD A, (HL+) and LDI A, (HL)
        registers.a = readByte(&m, (registers.h << 8) | registers.l);
        registers.l++;
        if (registers.l == 0x00) { // If the resulting value is 0 then the previous value must have been 255 so also increment H
          registers.h++;
        }
        cycles += 8;
        break;
      }
      
      /* LD (HLI), A - Same as LDI (HL), A -----------------------------------------------------*/
      /* LD (HL+), A - Same as LDI (HL), A -----------------------------------------------------*/
      /* LDI (HL), A ---------------------------------------------------------------------------*/
      case 0x22: { // LD (HLI), A, LD (HL+), A and LDI (HL), A
        writeByte(&m, (registers.h) << 8 | registers.l, registers.a);
        registers.l++;
        if (registers.l == 0x00) { // If the resulting value is 0 then the previous value must have been 255 so also increment H
          registers.h++;
        }
        cycles += 8;
        break;
      }
      
      /* LDH (n), A ----------------------------------------------------------------------------*/
      case 0xE0: { // LDH (n), A
        writeByte(&m, 0xFF00 + readByte(&m, registers.pc++), registers.a);
        cycles += 12;
        break;
      }
      
      /* LDH A, (n) ----------------------------------------------------------------------------*/
      case 0xF0: { // LDH A, (n)
        registers.a = readByte(&m, 0xFF00 + readByte(&m, registers.pc++));
        cycles += 12;
        break;
      }
      
      /* 16-Bit Loads ***************************************************************************/
      /* LD n, nn ------------------------------------------------------------------------------*/
      case 0x01: { // LD BC, nn
        registers.b = readByte(&m, registers.pc++);
        registers.c = readByte(&m, registers.pc++);
        cycles += 12;
        break;
      }
      case 0x11: { // LD DE, nn
        registers.d = readByte(&m, registers.pc++);
        registers.e = readByte(&m, registers.pc++);
        cycles += 12;
        break;
      }
      case 0x21: { // LD HL, nn
        registers.h = readByte(&m, registers.pc++);
        registers.l = readByte(&m, registers.pc++);
        cycles += 12;
        break;
      }
      case 0x31: { // LD SP, nn
        registers.sp = readWord(&m, registers.pc);
        registers.pc += 2;
        cycles += 12;
        break;
      }
      
      /* LD SP, HL -----------------------------------------------------------------------------*/
      case 0xF9: { // LD SP, HL
        registers.sp = (registers.h << 8) | registers.l;
        cycles += 8;
        break;
      }
      
      /* LD HL, SP + n - Same as LDHL SP, n ----------------------------------------------------*/
      /* LDHL SP, n ----------------------------------------------------------------------------*/
      case 0xF8: { // LD HL, SP + n and LDHL SP, n
        // TODO: Check this - particularly the setting of register F for the H and C bits
        int8_t signedValue = (int8_t)readByte(&m, registers.pc++);
        uint16_t oldAddress = (registers.h) | registers.l;
        uint16_t newAddress = registers.sp + signedValue;
        registers.h = (newAddress & 0xFF00) >> 8;
        registers.l = newAddress & 0x00FF;
        registers.f = 0x00;
        if (signedValue >= 0) {
          registers.f |= (((oldAddress & 0xFF) + signedValue) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
          registers.f |= (((oldAddress & 0xF) + (signedValue & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
        } else {
          registers.f |= ((newAddress & 0xFF) <= (oldAddress & 0xFF)) << FLAG_REGISTER_C_BIT_SHIFT;
          registers.f |= ((newAddress & 0xF) <= (oldAddress & 0xF)) << FLAG_REGISTER_H_BIT_SHIFT;
        }
        cycles += 12;
        break;
      }
      
      /* LD (nn), SP ---------------------------------------------------------------------------*/
      case 0x08: { // LD (nn), SP
        // TODO: Check this
        uint16_t address = readWord(&m, registers.pc);
        registers.pc += 2;
        writeWord(&m, address, registers.sp);
        cycles += 20;
        break;
      }
      
      /* PUSH nn -------------------------------------------------------------------------------*/
      case 0xF5: { // PUSH AF
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        writeByte(&m, --registers.sp, registers.a);
        writeByte(&m, --registers.sp, registers.f);
        cycles += 16;
        break;
      }
      case 0xC5: { // PUSH BC
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        writeByte(&m, --registers.sp, registers.b);
        writeByte(&m, --registers.sp, registers.c);
        cycles += 16;
        break;
      }
      case 0xD5: { // PUSH DE
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        writeByte(&m, --registers.sp, registers.d);
        writeByte(&m, --registers.sp, registers.e);
        cycles += 16;
        break;
      }
      case 0xE5: { // PUSH HL
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        writeByte(&m, --registers.sp, registers.h);
        writeByte(&m, --registers.sp, registers.l);
        cycles += 16;
        break;
      }
      
      /* POP, nn -------------------------------------------------------------------------------*/
      case 0xF1: { // POP AF
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        // TODO: Check clock cycles compared to PUSH nn
        registers.f = readByte(&m, registers.sp++);
        registers.a = readByte(&m, registers.sp++);
        cycles += 12;
        break;
      }
      case 0xC1: { // POP BC
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        // TODO: Check clock cycles compared to PUSH nn
        registers.c = readByte(&m, registers.sp++);
        registers.b = readByte(&m, registers.sp++);
        cycles += 12;
        break;
      }
      case 0xD1: { // POP DE
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        // TODO: Check clock cycles compared to PUSH nn
        registers.e = readByte(&m, registers.sp++);
        registers.d = readByte(&m, registers.sp++);
        cycles += 12;
        break;
      }
      case 0xE1: { // POP HL
        // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
        // TODO: Check clock cycles compared to PUSH nn
        registers.l = readByte(&m, registers.sp++);
        registers.h = readByte(&m, registers.sp++);
        cycles += 12;
        break;
      }
      
      /* 8-Bit ALU ******************************************************************************/
      /* ADD A, n ------------------------------------------------------------------------------*/
      case 0x87: { // ADD A, A
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(a)
      }
      case 0x80: { // ADD A, B
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(b)
      }
      case 0x81: { // ADD A, C
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(c)
      }
      case 0x82: { // ADD A, D
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(d)
      }
      case 0x83: { // ADD A, E
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(e)
      }
      case 0x84: { // ADD A, H
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(h)
      }
      case 0x85: { // ADD A, L
        // TODO: Check the setting of F register bits H and C
        MAKE_ADD_A_N_OPCODE_IMPL(l)
      }
      case 0x86: { // ADD A, (HL)
        // TODO: Check the setting of F register bits H and C
        uint8_t old = registers.a;
        uint32_t new = old + readByte(&m, (registers.h << 8) | registers.l);
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= (((old & 0xF) + (new & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= (((old & 0xFF) + (new & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
      case 0xC6: { // ADD A, #
        // TODO: Check the setting of F register bits H and C
        uint8_t old = registers.a;
        uint32_t new = old + readByte(&m, registers.pc++);
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= (((old & 0xF) + (new & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= (((old & 0xFF) + (new & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
      
      /* ADC A, n ------------------------------------------------------------------------------*/
      case 0x8F: { // ADC A, A
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(a)
      }
      case 0x88: { // ADC A, B
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(b)
      }
      case 0x89: { // ADC A, C
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(c)
      }
      case 0x8A: { // ADC A, D
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(d)
      }
      case 0x8B: { // ADC A, E
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(e)
      }
      case 0x8C: { // ADC A, H
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(h)
      }
      case 0x8D: { // ADC A, L
        // TODO: Check the setting of F register bits H and C
        MAKE_ADC_A_N_OPCODE_IMPL(l)
      }
      case 0x8E: { // ADC A, (HL)
        // TODO: Check the setting of F register bits H and C
        uint8_t old = registers.a;
        uint32_t new = old + readByte(&m, (registers.h << 8) | registers.l) + ((registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT);
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= (((old & 0xF) + (new & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= (((old & 0xFF) + (new & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
      case 0xCE: { // ADC A, #
        // TODO: Check the setting of F register bits H and C
        uint8_t old = registers.a;
        uint32_t new = old + readByte(&m, registers.pc++) + ((registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT);
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= (((old & 0xF) + (new & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= (((old & 0xFF) + (new & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
         
      /* SUB n ---------------------------------------------------------------------------------*/
      // TODO: Check all of these
      case 0x97: { // SUB A
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(a)
      }
      case 0x90: { // SUB B
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(b)
      }
      case 0x91: { // SUB C
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(c)
      }
      case 0x92: { // SUB D
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(d)
      }
      case 0x93: { // SUB E
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(e)
      }
      case 0x94: { // SUB H
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(h)
      }
      case 0x95: { // SUB L
        // TODO: Check the setting of F register bits H and C
        MAKE_SUB_N_OPCODE_IMPL(l)
      }
      case 0x96: { // SUB (HL)
        uint8_t old = registers.a;
        int32_t new = old - readByte(&m, (registers.h << 8) | registers.l);
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= ((new & 0xF) >= 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= ((new & 0xFF) >= 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
      case 0xD6: { // SUB #
        uint8_t old = registers.a;
        int32_t new = old - readByte(&m, registers.pc++);
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= ((new & 0xF) >= 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= ((new & 0xFF) >= 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
      
      /* SBC A, n ------------------------------------------------------------------------------*/
      // TODO: Check all of these
      case 0x9F: { // SBC A, A
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(a)
      }
      case 0x98: { // SBC A, B
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(b)
      }
      case 0x99: { // SBC A, C
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(c)
      }
      case 0x9A: { // SBC A, D
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(d)
      }
      case 0x9B: { // SBC A, E
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(e)
      }
      case 0x9C: { // SBC A, H
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(h)
      }
      case 0x9D: { // SBC A, L
        // TODO: Check the setting of F register bits H and C
        MAKE_SBC_A_N_OPCODE_IMPL(l)
      }
      case 0x9E: { // SBC A, (HL)
        // TODO: Check the setting of F register bits H and C
        uint8_t old = registers.a;
        int32_t new = old - (readByte(&m, (registers.h << 8) | registers.l) + ((registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT));
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= ((new & 0xF) >= 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= ((new & 0xFF) >= 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
      case 0xDE: { // SBC A, #
        // TODO: Check the setting of F register bits H and C
        // TODO: It this definitely the right opcode? Was listed as ?? in the GB CPU Manual PDF
        uint8_t old = registers.a;
        int32_t new = old - (readByte(&m, registers.pc++) + ((registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT));
        registers.a = new;
        registers.f |= (registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
        registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
        registers.f |= ((new & 0xF) >= 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
        registers.f |= ((new & 0xFF) >= 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
        cycles += 8;
        break;
      }
            
      /* AND n ---------------------------------------------------------------------------------*/
      /* OR n ----------------------------------------------------------------------------------*/
      /* XOR n ---------------------------------------------------------------------------------*/
      /* CP n ----------------------------------------------------------------------------------*/
      /* INC n ---------------------------------------------------------------------------------*/
      /* DEC n ---------------------------------------------------------------------------------*/
      
      /* 16-Bit Arithmetic **********************************************************************/
      /* ADD HL, n -----------------------------------------------------------------------------*/
      /* ADD SP, n -----------------------------------------------------------------------------*/
      /* INC nn --------------------------------------------------------------------------------*/
      /* DEC nn --------------------------------------------------------------------------------*/
      
      /* Miscellaneous **************************************************************************/
      /* SWAP n --------------------------------------------------------------------------------*/
      /* DAA -----------------------------------------------------------------------------------*/
      /* CPL -----------------------------------------------------------------------------------*/
      /* CCF -----------------------------------------------------------------------------------*/
      /* SCF -----------------------------------------------------------------------------------*/
      /* NOP -----------------------------------------------------------------------------------*/
      case 0x00: { // NOP
        cycles += 4;
        break;
      }
      /* HALT ----------------------------------------------------------------------------------*/
      /* STOP ----------------------------------------------------------------------------------*/
      /* DI ------------------------------------------------------------------------------------*/
      /* EI ------------------------------------------------------------------------------------*/
      
      /* Rotates and Shifts *********************************************************************/
      /* RLCA ----------------------------------------------------------------------------------*/
      /* RLA -----------------------------------------------------------------------------------*/
      /* RRCA ----------------------------------------------------------------------------------*/
      /* RRA -----------------------------------------------------------------------------------*/
      /* RLC n ---------------------------------------------------------------------------------*/
      /* RL n ----------------------------------------------------------------------------------*/
      /* RRC n ---------------------------------------------------------------------------------*/
      /* RR n ----------------------------------------------------------------------------------*/
      /* SLA n ---------------------------------------------------------------------------------*/
      /* SRA n ---------------------------------------------------------------------------------*/
      /* SRL n ---------------------------------------------------------------------------------*/
      
      /* Bit Opcodes ****************************************************************************/
      /* BIT b, r ------------------------------------------------------------------------------*/
      /* SET b, r ------------------------------------------------------------------------------*/
      /* RES b, r ------------------------------------------------------------------------------*/
      
      /* Jumps **********************************************************************************/
      /* JP nn ---------------------------------------------------------------------------------*/
      case 0xC3: { // JP nn
        registers.pc = readWord(&m, registers.pc);
        // registers.pc += 2; WHY WOULD WE DO THIS?????
        cycles += 12;
        break;
      }
      /* JP cc, nn -----------------------------------------------------------------------------*/
      /* JP (HL) -------------------------------------------------------------------------------*/
      /* JR n ----------------------------------------------------------------------------------*/
      /* JR cc, n-------------------------------------------------------------------------------*/
      
      /* Calls **********************************************************************************/
      /* CALL nn -------------------------------------------------------------------------------*/
      /* CALL cc, nn ---------------------------------------------------------------------------*/
      
      /* Restarts *******************************************************************************/
      /* RST n ---------------------------------------------------------------------------------*/
      
      /* Returns ********************************************************************************/
      /* RET -----------------------------------------------------------------------------------*/
      /* RET cc --------------------------------------------------------------------------------*/
      /* RETI ----------------------------------------------------------------------------------*/
      
      /******************************************************************************************/
      default:
        printf("FATAL ERROR: ENCOUNTERED UNKNOWN OPCODE: 0x%02X\n", opcode);
        exit(1);
        break;
    }
    struct timespec sleepRequested = {0, cycles * CLOCK_CYCLE_TIME_SECS * 1000000000};
    struct timespec sleepRemaining;
    nanosleep(&sleepRequested, &sleepRemaining);
  }
  
  free(cartridgeData);
  
  return 0;
}