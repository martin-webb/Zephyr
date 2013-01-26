#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "timer.h"

#include "mnemonics.h"

/* Opcode Generation Macros *********************************************************************/

#define MAKE_ADD_A_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = cpu->registers.a; \
  uint8_t value = cpu->registers.SOURCE_REGISTER; \
  uint32_t new = old + value; \
  cpu->registers.a = new; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xF) + (value & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xFF) + (value & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_ADC_A_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = cpu->registers.a; \
  uint8_t value = cpu->registers.SOURCE_REGISTER; \
  uint8_t carry = ((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT); \
  uint32_t new = old + value + carry; \
  cpu->registers.a = new; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xF) + (value & 0xF) + carry) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xFF) + (value & 0xFF) + carry) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_SUB_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = cpu->registers.a; \
  int32_t new = old - cpu->registers.SOURCE_REGISTER; \
  cpu->registers.a = new; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= ((new & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;
  
#define MAKE_SBC_A_N_OPCODE_IMPL(SOURCE_REGISTER) \
  uint8_t old = cpu->registers.a; \
  int32_t new = old - (cpu->registers.SOURCE_REGISTER + ((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT)); \
  cpu->registers.a = new; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= ((new & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_AND_N_OPCODE_IMPL(SOURCE_REGISTER) \
  cpu->registers.a &= cpu->registers.SOURCE_REGISTER; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;
  
#define MAKE_OR_N_OPCODE_IMPL(SOURCE_REGISTER) \
  cpu->registers.a |= cpu->registers.SOURCE_REGISTER; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_XOR_N_OPCODE_IMPL(SOURCE_REGISTER) \
  cpu->registers.a ^= cpu->registers.SOURCE_REGISTER; \
  cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_CP_N_OPCODE_IMPL(SOURCE_REGISTER) \
  int16_t result = cpu->registers.a - cpu->registers.SOURCE_REGISTER; \
  cpu->registers.f |= (result == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= ((result & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= ((result & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_INC_N_OPCODE_IMPL(REGISTER) \
  uint8_t old = cpu->registers.REGISTER; \
  uint16_t new = old + 1; \
  cpu->registers.REGISTER = new; \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xF) + (1 & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_DEC_N_OPCODE_IMPL(REGISTER) \
  uint8_t old = cpu->registers.REGISTER; \
  int16_t new = old - 1; \
  cpu->registers.REGISTER = new; \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 4; \
  break;

#define MAKE_ADD_HL_N_OPCODE_IMPL(REGISTER_HIGH, REGISTER_LOW) \
  uint16_t old = (cpu->registers.h << 8) | cpu->registers.l; \
  uint16_t value = (cpu->registers.REGISTER_HIGH << 8) | cpu->registers.REGISTER_LOW; \
  uint16_t result = old + value; \
  cpu->registers.h = (result & 0xFF00) >> 8; \
  cpu->registers.l = (result & 0x00FF); \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xFFF) + (value & 0xFFF)) > 0xFFF) << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= (((old & 0xFFFF) + (value & 0xFFFF)) > 0xFFFF) << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_INC_NN_OPCODE_IMPL(REGISTER_HIGH, REGISTER_LOW) \
  cpu->registers.REGISTER_LOW++; \
  if (cpu->registers.REGISTER_LOW == 0) { \
    cpu->registers.REGISTER_HIGH++; \
  } \
  cycles += 8; \
  break;

#define MAKE_DEC_NN_OPCODE_IMPL(REGISTER_HIGH, REGISTER_LOW) \
  cpu->registers.REGISTER_LOW--; \
  if (cpu->registers.REGISTER_LOW == 0xFF) { \
    cpu->registers.REGISTER_HIGH--; \
  } \
  cycles += 8; \
  break;

#define MAKE_SWAP_N_OPCODE_IMPL(REGISTER) \
  cpu->registers.REGISTER = ((cpu->registers.REGISTER & 0xF0) >> 4) | ((cpu->registers.REGISTER & 0x0F) << 4); \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_RLC_N_OPCODE_IMPL(REGISTER) \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); \
  cpu->registers.REGISTER = (cpu->registers.REGISTER << 1) | ((cpu->registers.REGISTER & BIT_7) >> BIT_7_SHIFT); \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_RL_N_OPCODE_IMPL(REGISTER) \
  uint8_t oldCarryBit = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT; \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); \
  cpu->registers.REGISTER = (cpu->registers.REGISTER << 1) | oldCarryBit; \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_RRC_N_OPCODE_IMPL(REGISTER) \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; \
  cpu->registers.REGISTER = ((cpu->registers.REGISTER & BIT_0) << BIT_7_SHIFT) | (cpu->registers.REGISTER >> 1); \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_RR_N_OPCODE_IMPL(REGISTER) \
  uint8_t oldCarryBit = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT; \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; \
  cpu->registers.REGISTER = (oldCarryBit << BIT_7_SHIFT) | (cpu->registers.REGISTER >> 1); \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_SLA_N_OPCODE_IMPL(REGISTER) \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); \
  cpu->registers.REGISTER <<= 1; \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_SRA_N_OPCODE_IMPL(REGISTER) \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; \
  cpu->registers.REGISTER = (cpu->registers.REGISTER & BIT_7) | (cpu->registers.REGISTER >> 1); \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;
  
#define MAKE_SRL_N_OPCODE_IMPL(REGISTER) \
  cpu->registers.f |= (cpu->registers.REGISTER & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; \
  cpu->registers.REGISTER = cpu->registers.REGISTER >> 1; \
  cpu->registers.f |= (cpu->registers.REGISTER == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

#define MAKE_BIT_B_R_OPCODE_IMPL(B, REGISTER) \
  cpu->registers.f |= (((cpu->registers.REGISTER & (0x1 << B)) >> B) == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 8; \
  break;

// TODO: Check this, with only one extra memory access (the byte read) I would expect this to be 12 clock cycles, not 16
#define MAKE_BIT_B_MEM_AT_HL_OPCODE_IMPL(B) \
  uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l); \
  cpu->registers.f |= (((value & (0x1 << B)) >> B) == 0) << FLAG_REGISTER_Z_BIT_SHIFT; \
  cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT; \
  cpu->registers.f |= 1 << FLAG_REGISTER_H_BIT_SHIFT; \
  cycles += 16; \
  break;

#define MAKE_BIT_B_R_OPCODE_GROUP(B, BEGINNING_OPCODE) \
  case BEGINNING_OPCODE - 0: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, a) \
  } \
  case BEGINNING_OPCODE - 7: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, b) \
  } \
  case BEGINNING_OPCODE - 6: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, c) \
  } \
  case BEGINNING_OPCODE - 5: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, d) \
  } \
  case BEGINNING_OPCODE - 4: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, e) \
  } \
  case BEGINNING_OPCODE - 3: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, h) \
  } \
  case BEGINNING_OPCODE - 2: { \
    MAKE_BIT_B_R_OPCODE_IMPL(B, l) \
  } \
  case BEGINNING_OPCODE - 1: { \
    MAKE_BIT_B_MEM_AT_HL_OPCODE_IMPL(B) \
  }

#define MAKE_SET_B_R_OPCODE_IMPL(B, REGISTER) \
  cpu->registers.REGISTER |= (0x1 << B); \
  cycles += 8; \
  break;
  
#define MAKE_SET_B_MEM_AT_HL_OPCODE_IMPL(B) \
  uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l); \
  value |= (0x1 << B); \
  writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value); \
  cycles += 16; \
  break;

#define MAKE_SET_B_R_OPCODE_GROUP(B, BEGINNING_OPCODE) \
  case BEGINNING_OPCODE - 0: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, a) \
  } \
  case BEGINNING_OPCODE - 7: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, b) \
  } \
  case BEGINNING_OPCODE - 6: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, c) \
  } \
  case BEGINNING_OPCODE - 5: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, d) \
  } \
  case BEGINNING_OPCODE - 4: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, e) \
  } \
  case BEGINNING_OPCODE - 3: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, h) \
  } \
  case BEGINNING_OPCODE - 2: { \
    MAKE_SET_B_R_OPCODE_IMPL(B, l) \
  } \
  case BEGINNING_OPCODE - 1: { \
    MAKE_SET_B_MEM_AT_HL_OPCODE_IMPL(B) \
  }

#define MAKE_RES_B_R_OPCODE_IMPL(B, REGISTER) \
  cpu->registers.REGISTER &= ~(0x0 << B); \
  cycles += 8; \
  break;

#define MAKE_RES_B_MEM_AT_HL_OPCODE_IMPL(B) \
  uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l); \
  value &= ~(0x0 << B); \
  writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value); \
  cycles += 16; \
  break;

#define MAKE_RES_B_R_OPCODE_GROUP(B, BEGINNING_OPCODE) \
  case BEGINNING_OPCODE - 0: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, a) \
  } \
  case BEGINNING_OPCODE - 7: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, b) \
  } \
  case BEGINNING_OPCODE - 6: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, c) \
  } \
  case BEGINNING_OPCODE - 5: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, d) \
  } \
  case BEGINNING_OPCODE - 4: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, e) \
  } \
  case BEGINNING_OPCODE - 3: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, h) \
  } \
  case BEGINNING_OPCODE - 2: { \
    MAKE_RES_B_R_OPCODE_IMPL(B, l) \
  } \
  case BEGINNING_OPCODE - 1: { \
    MAKE_RES_B_MEM_AT_HL_OPCODE_IMPL(B) \
  }

#define MAKE_CALL_CC_NN_OPCODE_IMPL(FLAG_REGISTER_BIT_MASK, FLAG_REGISTER_BIT_SHIFT, CONDITION_VALUE) \
  uint16_t address = readWord(m, cpu->registers.pc); \
  cpu->registers.pc += 2; \
  if (((cpu->registers.f & FLAG_REGISTER_BIT_MASK) >> FLAG_REGISTER_BIT_SHIFT) == CONDITION_VALUE) { \
    writeByte(m, --cpu->registers.sp, ((cpu->registers.pc & 0xFF00) >> 8)); \
    writeByte(m, --cpu->registers.sp, (cpu->registers.pc & 0x00FF)); \
    cpu->registers.pc = address; \
  } \
  cycles += 12; \
  break;

// TODO: Check the implementations here - the GB CPU Manual says to 'push present address onto
// the stack' but if the 'present address' is the address of the opcode currently executing
// and this address is returned to after a jump, the same opcode will be executed, causing an infinite loop.
// The wording here is notably different to CALL instructions, where the 'address of the next instruction'
// is pushed onto the stack.
// Zilog's Z80 manual always uses the phrase 'current contents of the program counter' for
// CALL and RST instructions, implying both the same behaviour and a program counter that is already incremented.
// TODO: Also check the 32 clock cycles for this
#define MAKE_RST_N_OPCODE_IMPL(N) \
  writeByte(m, --cpu->registers.sp, ((cpu->registers.pc & 0xFF00) >> 8)); \
  writeByte(m, --cpu->registers.sp, (cpu->registers.pc & 0x00FF)); \
  cpu->registers.pc = N; \
  cycles += 32; \
  break;

#define MAKE_RET_CC_OPCODE_IMPL(FLAG_REGISTER_BIT_MASK, FLAG_REGISTER_BIT_SHIFT, CONDITION_VALUE) \
  if (((cpu->registers.f & FLAG_REGISTER_BIT_MASK) >> FLAG_REGISTER_BIT_SHIFT) == CONDITION_VALUE) { \
    uint8_t addressLow = readByte(m, cpu->registers.sp++); \
    uint8_t addressHigh = readByte(m, cpu->registers.sp++); \
    cpu->registers.pc = (addressHigh << 8) | addressLow; \
  } \
  cycles += 8; \
  break;

/* End Opcode Generation Macros *****************************************************************/

void cpuReset(CPU* cpu, MemoryController* m, GameBoyType gameBoyType)
{
  switch (gameBoyType) {
    case GB:
    case SGB:
      cpu->registers.a = 0x00;
      cpu->registers.f = 0x01;
      break;
    case GBP:
      cpu->registers.a = 0x00;
      cpu->registers.f = 0xFF;
      break;
    case CGB:
      cpu->registers.a = 0x00;
      cpu->registers.f = 0x11;
      break;
    default:
      fprintf(stderr, "%s: Unknown GameBoyType %i encountered. Exiting...\n", __func__, gameBoyType);
      exit(EXIT_FAILURE);
      break;
  }
  cpu->registers.f = 0xB0;
  cpu->registers.sp = 0xFFFE;
  cpu->registers.pc = 0x100;

  writeByte(m, IO_REG_ADDRESS_TIMA, 0x00);
  writeByte(m, IO_REG_ADDRESS_TMA, 0x00);
  writeByte(m, IO_REG_ADDRESS_TAC, 0x00);
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
  switch (gameBoyType) {
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
  writeByte(m, IO_REG_ADDRESS_IE, 0x00);
}

void cpuPrintState(CPU* cpu)
{
  printf("A: 0x%02X B: 0x%02X C: 0x%02X D: 0x%02X E: 0x%02X F: 0x%02X H: 0x%02X L: 0x%02X\n",
    cpu->registers.a,
    cpu->registers.b,
    cpu->registers.c,
    cpu->registers.d,
    cpu->registers.e,
    cpu->registers.f,
    cpu->registers.h,
    cpu->registers.l
  );
  printf("SP: 0x%04X PC: 0x%04X\n",
    cpu->registers.sp,
    cpu->registers.pc
  );
  printf("FLAG: Z: %i N: %i H: %i C: %i\n",
    (cpu->registers.f & FLAG_REGISTER_Z_BIT) >> 7,
    (cpu->registers.f & FLAG_REGISTER_N_BIT) >> 6,
    (cpu->registers.f & FLAG_REGISTER_H_BIT) >> 5,
    (cpu->registers.f & FLAG_REGISTER_C_BIT) >> 4
  );
}

uint8_t cpuRunSingleOp(CPU* cpu, MemoryController* m)
{
  uint8_t opcode = readByte(m, cpu->registers.pc++);
  printf("[0x%04X][0x%02X] %s\n", cpu->registers.pc - 1, opcode, OPCODE_MNEMONICS[opcode]);

  // TODO: Check for overflow of opcode here?

  uint8_t cycles = 0;
  switch (opcode) {
    /* 8-Bit Loads ****************************************************************************/
    /* LD nn, n ------------------------------------------------------------------------------*/
    case 0x06: { // LD B, n
      cpu->registers.b = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }
    case 0x0E: { // LD C, n
      cpu->registers.c = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }
    case 0x16: { // LD D, n
      cpu->registers.d = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }
    case 0x1E: { // LD E, n
      cpu->registers.e = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }
    case 0x26: { // LD H, n
      cpu->registers.h = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }
    case 0x2E: { // LD L, n
      cpu->registers.l = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }

    /* LD r1, r2 -----------------------------------------------------------------------------*/
    case 0x7F: { // LD A, A
      cpu->registers.a = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x78: { // LD A, B
      cpu->registers.a = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x79: { // LD A, C
      cpu->registers.a = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x7A: { // LD A, D
      cpu->registers.a = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x7B: { // LD A, E
      cpu->registers.a = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x7C: { // LD A, H
      cpu->registers.a = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x7D: { // LD A, L
      cpu->registers.a = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x7E: { // LD A, (HL)
      cpu->registers.a = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }


    case 0x40: { // LD B, B
      cpu->registers.b = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x41: { // LD B, C
      cpu->registers.b = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x42: { // LD B, D
      cpu->registers.b = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x43: { // LD B, E
      cpu->registers.b = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x44: { // LD B, H
      cpu->registers.b = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x45: { // LD B, L
      cpu->registers.b = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x46: { // LD B, (HL)
      cpu->registers.b = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }


    case 0x48: { // LD C, B
      cpu->registers.c = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x49: { // LD C, C
      cpu->registers.c = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x4A: { // LD C, D
      cpu->registers.c = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x4B: { // LD C, E
      cpu->registers.c = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x4C: { // LD C, H
      cpu->registers.c = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x4D: { // LD C, L
      cpu->registers.c = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x4E: { // LD C, (HL)
      cpu->registers.c = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }


    case 0x50: { // LD D, B
      cpu->registers.d = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x51: { // LD D, C
      cpu->registers.d = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x52: { // LD D, D
      cpu->registers.d = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x53: { // LD D, E
      cpu->registers.d = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x54: { // LD D, H
      cpu->registers.d = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x55: { // LD D, L
      cpu->registers.d = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x56: { // LD D, (HL)
      cpu->registers.d = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }


    case 0x58: { // LD E, B
      cpu->registers.e = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x59: { // LD E, C
      cpu->registers.e = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x5A: { // LD E, D
      cpu->registers.e = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x5B: { // LD E, E
      cpu->registers.e = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x5C: { // LD E, H
      cpu->registers.e = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x5D: { // LD E, L
      cpu->registers.e = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x5E: { // LD E, (HL)
      cpu->registers.e = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }


    case 0x60: { // LD H, B
      cpu->registers.h = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x61: { // LD H, C
      cpu->registers.h = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x62: { // LD H, D
      cpu->registers.h = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x63: { // LD H, E
      cpu->registers.h = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x64: { // LD H, H
      cpu->registers.h = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x65: { // LD H, L
      cpu->registers.h = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x66: { // LD H, (HL)
      cpu->registers.h = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }


    case 0x68: { // LD L, B
      cpu->registers.l = cpu->registers.b;
      cycles += 4;
      break;
    }
    case 0x69: { // LD L, C
      cpu->registers.l = cpu->registers.c;
      cycles += 4;
      break;
    }
    case 0x6A: { // LD L, D
      cpu->registers.l = cpu->registers.d;
      cycles += 4;
      break;
    }
    case 0x6B: { // LD L, E
      cpu->registers.l = cpu->registers.e;
      cycles += 4;
      break;
    }
    case 0x6C: { // LD L, H
      cpu->registers.l = cpu->registers.h;
      cycles += 4;
      break;
    }
    case 0x6D: { // LD L, L
      cpu->registers.l = cpu->registers.l;
      cycles += 4;
      break;
    }
    case 0x6E: { // LD L, (HL)
      cpu->registers.l = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cycles += 8;
      break;
    }
    
    case 0x70: { // LD (HL), B
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.b);
      cycles += 8;
      break;
    }
    case 0x71: { // LD (HL), C
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.c);
      cycles += 8;
      break;
    }
    case 0x72: { // LD (HL), D
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.d);
      cycles += 8;
      break;
    }
    case 0x73: { // LD (HL), E
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.e);
      cycles += 8;
      break;
    }
    case 0x74: { // LD (HL), H
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.h);
      cycles += 8;
      break;
    }
    case 0x75: { // LD (HL), L
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.l);
      cycles += 8;
      break;
    }
    case 0x36: { // LD (HL), n
      // TODO: Check this line?
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, readByte(m, cpu->registers.pc++));
      cycles += 12;
      break;
    }

    /* LD A, n -------------------------------------------------------------------------------*/
    // NOTE: The GB CPU Manual contained duplicates of the following opcodes here: 7F, 78, 79, 7A, 7B, 7C, 7D and 3E
    case 0x0A: { // LD A, (BC)
      cpu->registers.a = readByte(m, (cpu->registers.b << 8) | cpu->registers.c);
      cycles += 8;
      break;
    }
    case 0x1A: { // LD A, (DE)
      cpu->registers.a = readByte(m, (cpu->registers.d << 8) | cpu->registers.e);
      cycles += 8;
      break;
    }
    case 0xFA: { // LD A, (nn)
      cpu->registers.a = readByte(m, readWord(m, cpu->registers.pc));
      cpu->registers.pc += 2;
      cycles += 16;
      break;
    }
    case 0x3E: { // LD A, #
      cpu->registers.a = readByte(m, cpu->registers.pc++);
      cycles += 8;
      break;
    }

    /* LD n, A -------------------------------------------------------------------------------*/
    // NOTE: The GB CPU Manual contained duplicates of the following opcodes here: 7F
    case 0x47: { // LD B, A
      cpu->registers.b = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x4F: { // LD C, A
      cpu->registers.c = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x57: { // LD D, A
      cpu->registers.d = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x5F: { // LD E, A
      cpu->registers.e = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x67: { // LD H, A
      cpu->registers.h = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x6F: { // LD L, A
      cpu->registers.l = cpu->registers.a;
      cycles += 4;
      break;
    }
    case 0x02: { // LD (BC), A
      writeByte(m, (cpu->registers.b << 8) | cpu->registers.c, cpu->registers.a);
      cycles += 8;
      break;
    }
    case 0x12: { // LD (DE), A
      writeByte(m, (cpu->registers.d << 8) | cpu->registers.e, cpu->registers.a);
      cycles += 8;
      break;
    }
    case 0x77: { // LD (HL), A
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.a);
      cycles += 8;
      break;
    }
    case 0xEA: { // LD (NN), A
      writeByte(m, readWord(m, cpu->registers.pc), cpu->registers.a);
      cpu->registers.pc += 2;
      cycles += 16;
      break;
    }

    /* LD A, (C) -----------------------------------------------------------------------------*/
    case 0xF2: { // LD A, (C)
      cpu->registers.a = readByte(m, 0xFF00 + cpu->registers.c);
      cycles += 8;
      break;
    }

    /* LD (C), A -----------------------------------------------------------------------------*/
    case 0xE2: { // LD (C), A
      writeByte(m, 0xFF00 + cpu->registers.c, cpu->registers.a);
      cycles += 8;
      break;
    }

    /* LD A, (HLD) - Same as LDD A, (HL) -----------------------------------------------------*/
    /* LD A, (HL-) - Same as LDD A, (HL) -----------------------------------------------------*/
    /* LDD A, (HL) ---------------------------------------------------------------------------*/
    case 0x3A: { // LD A, (HLD), LD A, (HL-) and LDD A, (HL)
      cpu->registers.a = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.l--;
      if (cpu->registers.l == 0xFF) { // If the resulting value is 255 then the previous value must have been 0 so also decrement H
        cpu->registers.h--;
      }
      cycles += 8;
      break;
    }

    /* LD (HLD), A - Same as LDD (HL), A -----------------------------------------------------*/
    /* LD (HL-), A - Same as LDD (HL), A -----------------------------------------------------*/
    /* LDD (HL), A ---------------------------------------------------------------------------*/
    case 0x32: { // LD (HLD), A, LD (HL-), A and LDD (HL), A
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, cpu->registers.a);
      cpu->registers.l--;
      if (cpu->registers.l == 0xFF) { // If the resulting value is 255 then the previous value must have been 0 so also decrement H
        cpu->registers.h--;
      }
      cycles += 8;
      break;
    }

    /* LD A, (HLI) - Same as LDI A, (HL) -----------------------------------------------------*/
    /* LD A, (HL+) - Same as LDI A, (HL) -----------------------------------------------------*/
    /* LDI A, (HL) ---------------------------------------------------------------------------*/
    case 0x2A: { // LD A, (HLI), LD A, (HL+) and LDI A, (HL)
      cpu->registers.a = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.l++;
      if (cpu->registers.l == 0x00) { // If the resulting value is 0 then the previous value must have been 255 so also increment H
        cpu->registers.h++;
      }
      cycles += 8;
      break;
    }

    /* LD (HLI), A - Same as LDI (HL), A -----------------------------------------------------*/
    /* LD (HL+), A - Same as LDI (HL), A -----------------------------------------------------*/
    /* LDI (HL), A ---------------------------------------------------------------------------*/
    case 0x22: { // LD (HLI), A, LD (HL+), A and LDI (HL), A
      writeByte(m, (cpu->registers.h) << 8 | cpu->registers.l, cpu->registers.a);
      cpu->registers.l++;
      if (cpu->registers.l == 0x00) { // If the resulting value is 0 then the previous value must have been 255 so also increment H
        cpu->registers.h++;
      }
      cycles += 8;
      break;
    }

    /* LDH (n), A ----------------------------------------------------------------------------*/
    case 0xE0: { // LDH (n), A
      writeByte(m, 0xFF00 + readByte(m, cpu->registers.pc++), cpu->registers.a);
      cycles += 12;
      break;
    }

    /* LDH A, (n) ----------------------------------------------------------------------------*/
    case 0xF0: { // LDH A, (n)
      cpu->registers.a = readByte(m, 0xFF00 + readByte(m, cpu->registers.pc++));
      cycles += 12;
      break;
    }

    /* 16-Bit Loads ***************************************************************************/
    /* LD n, nn ------------------------------------------------------------------------------*/
    case 0x01: { // LD BC, nn
      cpu->registers.c = readByte(m, cpu->registers.pc++);
      cpu->registers.b = readByte(m, cpu->registers.pc++);
      cycles += 12;
      break;
    }
    case 0x11: { // LD DE, nn
      cpu->registers.e = readByte(m, cpu->registers.pc++);
      cpu->registers.d = readByte(m, cpu->registers.pc++);
      cycles += 12;
      break;
    }
    case 0x21: { // LD HL, nn
      cpu->registers.l = readByte(m, cpu->registers.pc++);
      cpu->registers.h = readByte(m, cpu->registers.pc++);
      cycles += 12;
      break;
    }
    case 0x31: { // LD SP, nn
      cpu->registers.sp = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      cycles += 12;
      break;
    }

    /* LD SP, HL -----------------------------------------------------------------------------*/
    case 0xF9: { // LD SP, HL
      cpu->registers.sp = (cpu->registers.h << 8) | cpu->registers.l;
      cycles += 8;
      break;
    }

    /* LD HL, SP + n - Same as LDHL SP, n ----------------------------------------------------*/
    /* LDHL SP, n ----------------------------------------------------------------------------*/
    case 0xF8: { // LD HL, SP + n and LDHL SP, n
      // TODO: Check this - particularly the setting of register F for the H and C bits
      int8_t signedValue = (int8_t)readByte(m, cpu->registers.pc++);
      uint16_t oldAddress = (cpu->registers.h) | cpu->registers.l;
      uint16_t newAddress = cpu->registers.sp + signedValue;
      cpu->registers.h = (newAddress & 0xFF00) >> 8;
      cpu->registers.l = newAddress & 0x00FF;
      cpu->registers.f |= 0 << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      if (signedValue >= 0) {
        cpu->registers.f |= (((oldAddress & 0xFF) + signedValue) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
        cpu->registers.f |= (((oldAddress & 0xF) + (signedValue & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
      } else {
        cpu->registers.f |= ((newAddress & 0xFF) <= (oldAddress & 0xFF)) << FLAG_REGISTER_C_BIT_SHIFT;
        cpu->registers.f |= ((newAddress & 0xF) <= (oldAddress & 0xF)) << FLAG_REGISTER_H_BIT_SHIFT;
      }
      cycles += 12;
      break;
    }

    /* LD (nn), SP ---------------------------------------------------------------------------*/
    case 0x08: { // LD (nn), SP
      uint16_t address = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      writeWord(m, address, cpu->registers.sp);
      cycles += 20;
      break;
    }

    /* PUSH nn -------------------------------------------------------------------------------*/
    case 0xF5: { // PUSH AF
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      writeByte(m, --cpu->registers.sp, cpu->registers.a);
      writeByte(m, --cpu->registers.sp, cpu->registers.f);
      cycles += 16;
      break;
    }
    case 0xC5: { // PUSH BC
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      writeByte(m, --cpu->registers.sp, cpu->registers.b);
      writeByte(m, --cpu->registers.sp, cpu->registers.c);
      cycles += 16;
      break;
    }
    case 0xD5: { // PUSH DE
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      writeByte(m, --cpu->registers.sp, cpu->registers.d);
      writeByte(m, --cpu->registers.sp, cpu->registers.e);
      cycles += 16;
      break;
    }
    case 0xE5: { // PUSH HL
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      writeByte(m, --cpu->registers.sp, cpu->registers.h);
      writeByte(m, --cpu->registers.sp, cpu->registers.l);
      cycles += 16;
      break;
    }

    /* POP, nn -------------------------------------------------------------------------------*/
    case 0xF1: { // POP AF
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      // TODO: Check clock cycles compared to PUSH nn
      cpu->registers.f = readByte(m, cpu->registers.sp++);
      cpu->registers.a = readByte(m, cpu->registers.sp++);
      cycles += 12;
      break;
    }
    case 0xC1: { // POP BC
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      // TODO: Check clock cycles compared to PUSH nn
      cpu->registers.c = readByte(m, cpu->registers.sp++);
      cpu->registers.b = readByte(m, cpu->registers.sp++);
      cycles += 12;
      break;
    }
    case 0xD1: { // POP DE
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      // TODO: Check clock cycles compared to PUSH nn
      cpu->registers.e = readByte(m, cpu->registers.sp++);
      cpu->registers.d = readByte(m, cpu->registers.sp++);
      cycles += 12;
      break;
    }
    case 0xE1: { // POP HL
      // TODO: Check the order of registers - here we are keeping the higher-order byte of the register pair at the higher address in memory
      // TODO: Check clock cycles compared to PUSH nn
      cpu->registers.l = readByte(m, cpu->registers.sp++);
      cpu->registers.h = readByte(m, cpu->registers.sp++);
      cycles += 12;
      break;
    }

    /* 8-Bit ALU ******************************************************************************/
    /* ADD A, n ------------------------------------------------------------------------------*/
    case 0x87: { // ADD A, A
      MAKE_ADD_A_N_OPCODE_IMPL(a)
    }
    case 0x80: { // ADD A, B
      MAKE_ADD_A_N_OPCODE_IMPL(b)
    }
    case 0x81: { // ADD A, C
      MAKE_ADD_A_N_OPCODE_IMPL(c)
    }
    case 0x82: { // ADD A, D
      MAKE_ADD_A_N_OPCODE_IMPL(d)
    }
    case 0x83: { // ADD A, E
      MAKE_ADD_A_N_OPCODE_IMPL(e)
    }
    case 0x84: { // ADD A, H
      MAKE_ADD_A_N_OPCODE_IMPL(h)
    }
    case 0x85: { // ADD A, L
      MAKE_ADD_A_N_OPCODE_IMPL(l)
    }
    case 0x86: { // ADD A, (HL)
      uint8_t old = cpu->registers.a;
      uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      uint32_t new = old + value;
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xF) + (value & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xFF) + (value & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xC6: { // ADD A, #
      uint8_t old = cpu->registers.a;
      uint8_t value = readByte(m, cpu->registers.pc++);
      uint32_t new = old + value;
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xF) + (value & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xFF) + (value & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* ADC A, n ------------------------------------------------------------------------------*/
    case 0x8F: { // ADC A, A
      MAKE_ADC_A_N_OPCODE_IMPL(a)
    }
    case 0x88: { // ADC A, B
      MAKE_ADC_A_N_OPCODE_IMPL(b)
    }
    case 0x89: { // ADC A, C
      MAKE_ADC_A_N_OPCODE_IMPL(c)
    }
    case 0x8A: { // ADC A, D
      MAKE_ADC_A_N_OPCODE_IMPL(d)
    }
    case 0x8B: { // ADC A, E
      MAKE_ADC_A_N_OPCODE_IMPL(e)
    }
    case 0x8C: { // ADC A, H
      MAKE_ADC_A_N_OPCODE_IMPL(h)
    }
    case 0x8D: { // ADC A, L
      MAKE_ADC_A_N_OPCODE_IMPL(l)
    }
    case 0x8E: { // ADC A, (HL)
      uint8_t old = cpu->registers.a;
      uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      uint8_t carry = ((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT);
      uint32_t new = old + value + carry;
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xF) + (value & 0xF) + carry) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xFF) + (value & 0xFF) + carry) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xCE: { // ADC A, #
      uint8_t old = cpu->registers.a;
      uint8_t value = readByte(m, cpu->registers.pc++);
      uint8_t carry = ((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT);
      uint32_t new = old + value + carry;
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xF) + (value & 0xF) + carry) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xFF) + (value & 0xFF) + carry) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* SUB n ---------------------------------------------------------------------------------*/
    // TODO: Check all of these
    case 0x97: { // SUB A
      MAKE_SUB_N_OPCODE_IMPL(a)
    }
    case 0x90: { // SUB B
      MAKE_SUB_N_OPCODE_IMPL(b)
    }
    case 0x91: { // SUB C
      MAKE_SUB_N_OPCODE_IMPL(c)
    }
    case 0x92: { // SUB D
      MAKE_SUB_N_OPCODE_IMPL(d)
    }
    case 0x93: { // SUB E
      MAKE_SUB_N_OPCODE_IMPL(e)
    }
    case 0x94: { // SUB H
      MAKE_SUB_N_OPCODE_IMPL(h)
    }
    case 0x95: { // SUB L
      MAKE_SUB_N_OPCODE_IMPL(l)
    }
    case 0x96: { // SUB (HL)
      uint8_t old = cpu->registers.a;
      int32_t new = old - readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xD6: { // SUB #
      uint8_t old = cpu->registers.a;
      int32_t new = old - readByte(m, cpu->registers.pc++);
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* SBC A, n ------------------------------------------------------------------------------*/
    // TODO: Check all of these
    case 0x9F: { // SBC A, A
      MAKE_SBC_A_N_OPCODE_IMPL(a)
    }
    case 0x98: { // SBC A, B
      MAKE_SBC_A_N_OPCODE_IMPL(b)
    }
    case 0x99: { // SBC A, C
      MAKE_SBC_A_N_OPCODE_IMPL(c)
    }
    case 0x9A: { // SBC A, D
      MAKE_SBC_A_N_OPCODE_IMPL(d)
    }
    case 0x9B: { // SBC A, E
      MAKE_SBC_A_N_OPCODE_IMPL(e)
    }
    case 0x9C: { // SBC A, H
      MAKE_SBC_A_N_OPCODE_IMPL(h)
    }
    case 0x9D: { // SBC A, L
      MAKE_SBC_A_N_OPCODE_IMPL(l)
    }
    case 0x9E: { // SBC A, (HL)
      uint8_t old = cpu->registers.a;
      int32_t new = old - (readByte(m, (cpu->registers.h << 8) | cpu->registers.l) + ((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT));
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xDE: { // SBC A, #
      uint8_t old = cpu->registers.a;
      int32_t new = old - (readByte(m, cpu->registers.pc++) + ((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT));
      cpu->registers.a = new;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xFF) < 0x0) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* AND n ---------------------------------------------------------------------------------*/
    case 0xA7: { // AND A
      MAKE_AND_N_OPCODE_IMPL(a)
    }
    case 0xA0: { // AND B
      MAKE_AND_N_OPCODE_IMPL(b)
    }
    case 0xA1: { // AND C
      MAKE_AND_N_OPCODE_IMPL(c)
    }
    case 0xA2: { // AND D
      MAKE_AND_N_OPCODE_IMPL(d)
    }
    case 0xA3: { // AND E
      MAKE_AND_N_OPCODE_IMPL(e)
    }
    case 0xA4: { // AND H
      MAKE_AND_N_OPCODE_IMPL(h)
    }
    case 0xA5: { // AND L
      MAKE_AND_N_OPCODE_IMPL(l)
    }
    case 0xA6: { // AND (HL)
      cpu->registers.a &= readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xE6: { // AND #
      cpu->registers.a &= readByte(m, cpu->registers.pc++);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* OR n ----------------------------------------------------------------------------------*/
    case 0xB7: { // OR A
      MAKE_OR_N_OPCODE_IMPL(a)
    }
    case 0xB0: { // OR B
      MAKE_OR_N_OPCODE_IMPL(b)
    }
    case 0xB1: { // OR C
      MAKE_OR_N_OPCODE_IMPL(c)
    }
    case 0xB2: { // OR D
      MAKE_OR_N_OPCODE_IMPL(d)
    }
    case 0xB3: { // OR E
      MAKE_OR_N_OPCODE_IMPL(e)
    }
    case 0xB4: { // OR H
      MAKE_OR_N_OPCODE_IMPL(h)
    }
    case 0xB5: { // OR L
      MAKE_OR_N_OPCODE_IMPL(l)
    }
    case 0xB6: { // OR (HL)
      cpu->registers.a |= readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xF6: { // OR #
      cpu->registers.a |= readByte(m, cpu->registers.pc++);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* XOR n ---------------------------------------------------------------------------------*/
    case 0xAF: { // XOR A
      MAKE_XOR_N_OPCODE_IMPL(a)
    }
    case 0xA8: { // XOR B
      MAKE_XOR_N_OPCODE_IMPL(b)
    }
    case 0xA9: { // XOR C
      MAKE_XOR_N_OPCODE_IMPL(c)
    }
    case 0xAA: { // XOR D
      MAKE_XOR_N_OPCODE_IMPL(d)
    }
    case 0xAB: { // XOR E
      MAKE_XOR_N_OPCODE_IMPL(e)
    }
    case 0xAC: { // XOR H
      MAKE_XOR_N_OPCODE_IMPL(h)
    }
    case 0xAD: { // XOR L
      MAKE_XOR_N_OPCODE_IMPL(l)
    }
    case 0xAE: { // XOR (HL)
      cpu->registers.a ^= readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xEE: { // XOR *
      cpu->registers.a ^= readByte(m, cpu->registers.pc++);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* CP n ----------------------------------------------------------------------------------*/
    case 0xBF: { // CP A
      MAKE_CP_N_OPCODE_IMPL(a)
    }
    case 0xB8: { // CP B
      MAKE_CP_N_OPCODE_IMPL(b)
    }
    case 0xB9: { // CP C
      MAKE_CP_N_OPCODE_IMPL(c)
    }
    case 0xBA: { // CP D
      MAKE_CP_N_OPCODE_IMPL(d)
    }
    case 0xBB: { // CP E
      MAKE_CP_N_OPCODE_IMPL(e)
    }
    case 0xBC: { // CP H
      MAKE_CP_N_OPCODE_IMPL(h)
    }
    case 0xBD: { // CP L
      MAKE_CP_N_OPCODE_IMPL(l)
    }
    case 0xBE: { // CP (HL)
      int16_t result = cpu->registers.a - readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      cpu->registers.f |= (result == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((result & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= ((result & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }
    case 0xFE: { // CP #
      int16_t result = cpu->registers.a - readByte(m, cpu->registers.pc++);
      cpu->registers.f |= (result == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((result & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= ((result & 0xFF) < 0x00) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* INC n ---------------------------------------------------------------------------------*/
    case 0x3C: { // INC A
      MAKE_INC_N_OPCODE_IMPL(a)
    }
    case 0x04: { // INC B
      MAKE_INC_N_OPCODE_IMPL(b)
    }
    case 0x0C: { // INC C
      MAKE_INC_N_OPCODE_IMPL(c)
    }
    case 0x14: { // INC D
      MAKE_INC_N_OPCODE_IMPL(d)
    }
    case 0x1C: { // INC E
      MAKE_INC_N_OPCODE_IMPL(e)
    }
    case 0x24: { // INC H
      MAKE_INC_N_OPCODE_IMPL(h)
    }
    case 0x2C: { // INC L
      MAKE_INC_N_OPCODE_IMPL(l)
    }
    case 0x34: { // INC (HL)
      uint8_t old = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      uint16_t new = old + 1;
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, (uint8_t)new);
      cpu->registers.f |= ((uint8_t)new == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xF) + (1 & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 12;
      break;
    }

    /* DEC n ---------------------------------------------------------------------------------*/
    case 0x3D: { // DEC A
      MAKE_DEC_N_OPCODE_IMPL(a)
    }
    case 0x05: { // DEC B
      MAKE_DEC_N_OPCODE_IMPL(b)
    }
    case 0x0D: { // DEC C
      MAKE_DEC_N_OPCODE_IMPL(c)
    }
    case 0x15: { // DEC D
      MAKE_DEC_N_OPCODE_IMPL(d)
    }
    case 0x1D: { // DEC E
      MAKE_DEC_N_OPCODE_IMPL(e)
    }
    case 0x25: { // DEC H
      MAKE_DEC_N_OPCODE_IMPL(h)
    }
    case 0x2D: { // DEC L
      MAKE_DEC_N_OPCODE_IMPL(l)
    }
    case 0x35: { // DEC (HL)
      uint8_t old = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
      int16_t new = old - 1;
      writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, (uint8_t)new);
      cpu->registers.f |= ((uint8_t)new == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= ((new & 0xF) < 0x0) << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 12;
      break;
    }

    /* 16-Bit Arithmetic **********************************************************************/
    /* ADD HL, n -----------------------------------------------------------------------------*/
    case 0x09: { // ADD HL, BC
      MAKE_ADD_HL_N_OPCODE_IMPL(b, c)
    }
    case 0x19: { // ADD HL, DE
      MAKE_ADD_HL_N_OPCODE_IMPL(d, e)
    }
    case 0x29: { // ADD HL, HL
      MAKE_ADD_HL_N_OPCODE_IMPL(h, l)
    }
    case 0x39: { // ADD HL, SP
      uint16_t old = (cpu->registers.h << 8) | cpu->registers.l;
      uint16_t value = cpu->registers.sp;
      uint16_t result = old + value;
      cpu->registers.h = (result & 0xFF00) >> 8;
      cpu->registers.l = (result & 0x00FF);
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xFFF) + (value & 0xFFF)) > 0xFFF) << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= (((old & 0xFFFF) + (value & 0xFFFF)) > 0xFFFF) << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 8;
      break;
    }

    /* ADD SP, n -----------------------------------------------------------------------------*/
    case 0xE8: { // ADD SP, n
      // TODO: Check this for correctness, particularly the setting of the H and C flags of register F
      // and the use of int32_t and conversion of int32_t to uint16_t
      int8_t value = (int8_t)readByte(m, cpu->registers.pc++);
      uint16_t old = cpu->registers.sp;
      int32_t new = old + value;
      cpu->registers.sp = new;
      cpu->registers.f |= 0 << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      if (value >= 0) {
        cpu->registers.f |= (((old & 0xF) + (value & 0xF)) > 0xF) << FLAG_REGISTER_H_BIT_SHIFT;
        cpu->registers.f |= (((old & 0xFF) + (value & 0xFF)) > 0xFF) << FLAG_REGISTER_C_BIT_SHIFT;
      } else {
        cpu->registers.f |= ((new & 0xF) <= (old & 0xF)) << FLAG_REGISTER_H_BIT_SHIFT;
        cpu->registers.f |= ((new & 0xFF) <= (old & 0xFF)) << FLAG_REGISTER_C_BIT_SHIFT;
      }
      cycles += 16;
      break;
    }

    /* INC nn --------------------------------------------------------------------------------*/
    case 0x03: { // INC BC
      MAKE_INC_NN_OPCODE_IMPL(b, c)
    }
    case 0x13: { // INC DE
      MAKE_INC_NN_OPCODE_IMPL(d, e)
    }
    case 0x23: { // INC HL
      MAKE_INC_NN_OPCODE_IMPL(h, l)
    }
    case 0x33: { // INC SP
      cpu->registers.sp++;
      cycles += 8;
      break;
    }

    /* DEC nn --------------------------------------------------------------------------------*/
    case 0x0B: { // DEC BC
      MAKE_DEC_NN_OPCODE_IMPL(b, c)
    }
    case 0x1B: { // DEC DE
      MAKE_DEC_NN_OPCODE_IMPL(d, e)
    }
    case 0x2B: { // DEC HL
      MAKE_DEC_NN_OPCODE_IMPL(h, l)
    }
    case 0x3B: { // DEC SP
      cpu->registers.sp--;
      cycles += 8;
      break;
    }

    /* Miscellaneous **************************************************************************/
    /* DAA -----------------------------------------------------------------------------------*/
    case 0x27: { // DAA
      uint8_t n = (cpu->registers.f & FLAG_REGISTER_N_BIT) >> FLAG_REGISTER_N_BIT_SHIFT;
      uint8_t c = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT;
      uint8_t h = (cpu->registers.f & FLAG_REGISTER_H_BIT) >> FLAG_REGISTER_H_BIT_SHIFT;
      uint8_t upperDigit = (cpu->registers.a & 0xF0) >> 4;
      uint8_t lowerDigit = cpu->registers.a & 0x0F;
      if (n == 0) {
        if (c == 0) {
          if ((upperDigit <= 0x9) && (h == 0) && (lowerDigit <= 0x9)) {
            cpu->registers.a += 0x00; // Adding 0 simply for consistency and awareness that there is no change here - this could be removed (if it's not optimised out)
            cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
          } else if (
            ((upperDigit <= 0x8) && (h == 0) && (lowerDigit >= 0xA) && (lowerDigit <= 0xF)) ||
            ((upperDigit <= 0x9) && (h == 1) && (lowerDigit <= 0x3))
            ) {
            cpu->registers.a += 0x06;
            cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
          } else if ((upperDigit >= 0xA) && (upperDigit <= 0xF) && (h == 0) && (lowerDigit <= 0x9)) {
            cpu->registers.a += 0x60;
            cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
          } else if (
            ((upperDigit >= 0x9) && (upperDigit <= 0xF) && (h == 0) && (lowerDigit >= 0xA) && (lowerDigit <= 0xF)) ||
            ((upperDigit >= 0xA) && (upperDigit <= 0xF) && (h == 1) && (lowerDigit <= 0x3))
            ) {
            cpu->registers.a += 0x66;
            cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
          } else {
            fprintf(stderr, "FATAL ERROR: ERROR IN DAA - UNSUPPORTED CONDITIONS FOR OPERATION! n=%u c=%u h=%u upper=0x%X lower=0x%X (ERROR LOC. 1)\n", n, c, h, upperDigit, lowerDigit);
            exit(EXIT_FAILURE);
          }
        } else { // (c == 1)
          if ((upperDigit <= 0x2) && (h == 0) && (lowerDigit <= 0x9)) {
            cpu->registers.a += 0x60;
            cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
          } else if (
            ((upperDigit <= 0x2) && (h == 0) && (lowerDigit >= 0xA) && (lowerDigit <= 0xF)) ||
            ((upperDigit <= 0x3) && (h == 1) && (lowerDigit <= 0x3))
            ) {
            cpu->registers.a += 0x66;
            cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
          } else {
            fprintf(stderr, "FATAL ERROR: ERROR IN DAA - UNSUPPORTED CONDITIONS FOR OPERATION! n=%u c=%u h=%u upper=0x%X lower=0x%X (ERROR LOC. 2)\n", n, c, h, upperDigit, lowerDigit);
            exit(EXIT_FAILURE);
          }
        }
      } else { // (n == 1)
        if (c == 0) {
          if ((upperDigit <= 0x9) && (h == 0) && (lowerDigit <= 0x9)) {
            cpu->registers.a += 0x00; // Adding 0 simply for consistency and awareness that there is no change here - this could be removed (if it's not optimised out)
            cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
          } else if ((upperDigit <= 0x8) && (h == 1) && (lowerDigit >= 0x6) && (lowerDigit <= 0xF)) {
            cpu->registers.a += 0xFA;
            cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
          } else {
            fprintf(stderr, "FATAL ERROR: ERROR IN DAA - UNSUPPORTED CONDITIONS FOR OPERATION! n=%u c=%u h=%u upper=0x%X lower=0x%X (ERROR LOC. 3)\n", n, c, h, upperDigit, lowerDigit);
            exit(EXIT_FAILURE);
          }
        } else { // (c == 1)
          if ((upperDigit >= 0x7) && (upperDigit <= 0xF) && (h == 0) && (lowerDigit <= 0x9)) {
            cpu->registers.a += 0xA0;
            cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
          } else if ((upperDigit >= 0x6) && (upperDigit <= 0xF) && (h == 1) && (lowerDigit >= 0x6) && (lowerDigit <= 0xF)) {
            cpu->registers.a += 0x9A;
            cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
          } else {
            fprintf(stderr, "FATAL ERROR: ERROR IN DAA - UNSUPPORTED CONDITIONS FOR OPERATION! n=%u c=%u h=%u upper=0x%X lower=0x%X (ERROR LOC. 4)\n", n, c, h, upperDigit, lowerDigit);
            exit(EXIT_FAILURE);
          }
        }
      }
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* CPL -----------------------------------------------------------------------------------*/
    case 0x2F: { // CPL
      cpu->registers.a = ~cpu->registers.a;
      cpu->registers.f |= 1 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* CCF -----------------------------------------------------------------------------------*/
    case 0x3F: { // CCF
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f ^= 1 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* SCF -----------------------------------------------------------------------------------*/
    case 0x37: { // SCF
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cpu->registers.f |= 1 << FLAG_REGISTER_C_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* NOP -----------------------------------------------------------------------------------*/
    case 0x00: { // NOP
      cycles += 4;
      break;
    }
    /* HALT ----------------------------------------------------------------------------------*/
    case 0x76: { // HALT
      cpu->halt = true;
      cycles += 4;
      break;
    }
    
    /* STOP ----------------------------------------------------------------------------------*/
    case 0x10: { // STOP
      // NOTE: Some Game Boy CPU manuals document the opcode for this instruction as 10 00 but there
      // seems to be no reason for this to not be a single byte opcode so some assemblers simply code
      // it as 10. There is also no indication in the number of required clock cycles of a a read of an additional byte.
      cpu->stop = true;
      cycles += 4;
      break;
    }
    
    /* DI ------------------------------------------------------------------------------------*/
    case 0xF3: { // DI
      cpu->di = 1;
      cycles += 4;
      break;
    }
    
    /* EI ------------------------------------------------------------------------------------*/
    case 0xFB: { // EI
      cpu->ei = 1;
      cycles += 4;
      break;
    }

    /* Rotates and Shifts *********************************************************************/
    /* RLCA ----------------------------------------------------------------------------------*/
    case 0x07: { // RLCA
      cpu->registers.f |= (cpu->registers.a & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); // NOTE: Set the C bit of F before we modify A
      cpu->registers.a = (cpu->registers.a << 1) | ((cpu->registers.a & BIT_7) >> BIT_7_SHIFT);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* RLA -----------------------------------------------------------------------------------*/
    case 0x17: { // RLA
      uint8_t oldCarryBit = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT;
      cpu->registers.f |= (cpu->registers.a & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); // NOTE: Set the C bit of F before we modify A
      cpu->registers.a = (cpu->registers.a << 1) | oldCarryBit;
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* RRCA ----------------------------------------------------------------------------------*/
    case 0x0F: { // RRCA
      cpu->registers.f |= (cpu->registers.a & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; // NOTE: Set the C bit of F before we modify A
      cpu->registers.a = ((cpu->registers.a & BIT_0) << BIT_7_SHIFT) | (cpu->registers.a >> 1);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* RRA -----------------------------------------------------------------------------------*/
    case 0x1F: { // RRA
      uint8_t oldCarryBit = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT;
      cpu->registers.f |= (cpu->registers.a & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; // NOTE: Set the C bit of F before we modify A
      cpu->registers.a = (oldCarryBit << BIT_7_SHIFT) | (cpu->registers.a >> 1);
      cpu->registers.f |= (cpu->registers.a == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
      cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
      cycles += 4;
      break;
    }

    /* Jumps **********************************************************************************/
    /* JP nn ---------------------------------------------------------------------------------*/
    case 0xC3: { // JP nn
      uint16_t address = readWord(m, cpu->registers.pc++);
      cpu->registers.pc = address;
      cycles += 12;
      break;
    }

    /* JP cc, nn -----------------------------------------------------------------------------*/
    case 0xC2: { // JP NZ, nn
      uint16_t address = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      if (((cpu->registers.f & FLAG_REGISTER_Z_BIT) >> FLAG_REGISTER_Z_BIT_SHIFT) == 0) {
        cpu->registers.pc = address;
      }
      cycles += 12;
      break;
    }
    case 0xCA: { // JP Z, nn
      uint16_t address = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      if (((cpu->registers.f & FLAG_REGISTER_Z_BIT) >> FLAG_REGISTER_Z_BIT_SHIFT) == 1) {
        cpu->registers.pc = address;
      }
      cycles += 12;
      break;
    }
    case 0xD2: { // JP NC, nn
      uint16_t address = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      if (((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT) == 0) {
        cpu->registers.pc = address;
      }
      cycles += 12;
      break;
    }
    case 0xDA: { // JP C, nn
      uint16_t address = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      if (((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT) == 1) {
        cpu->registers.pc = address;
      }
      cycles += 12;
      break;
    }

    /* JP (HL) -------------------------------------------------------------------------------*/
    case 0xE9: { // JP (HL)
      cpu->registers.pc = (cpu->registers.h << 8) | cpu->registers.l;
      cycles += 4;
      break;
    }

    /* JR n ----------------------------------------------------------------------------------*/
    case 0x18: { // JR n
      int8_t value = (int8_t)readByte(m, cpu->registers.pc++);
      cpu->registers.pc += value;
      cycles += 8;
      break;
    }

    /* JR cc, n-------------------------------------------------------------------------------*/
    case 0x20: { // JR NZ, n
      int8_t value = (int8_t)readByte(m, cpu->registers.pc++);
      if (((cpu->registers.f & FLAG_REGISTER_Z_BIT) >> FLAG_REGISTER_Z_BIT_SHIFT) == 0) {
        cpu->registers.pc += value;
      }
      cycles += 8;
      break;
    }
    case 0x28: { // JR Z, n
      int8_t value = (int8_t)readByte(m, cpu->registers.pc++);
      if (((cpu->registers.f & FLAG_REGISTER_Z_BIT) >> FLAG_REGISTER_Z_BIT_SHIFT) == 1) {
        cpu->registers.pc += value;
      }
      cycles += 8;
      break;
    }
    case 0x30: { // JR NC, n
      int8_t value = (int8_t)readByte(m, cpu->registers.pc++);
      if (((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT) == 0) {
        cpu->registers.pc += value;
      }
      cycles += 8;
      break;
    }
    case 0x38: { // JR C, n
      int8_t value = (int8_t)readByte(m, cpu->registers.pc++);
      if (((cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT) == 1) {
        cpu->registers.pc += value;
      }
      cycles += 8;
      break;
    }

    /* Calls **********************************************************************************/
    /* CALL nn -------------------------------------------------------------------------------*/
    case 0xCD: { // CALL nn
      uint16_t address = readWord(m, cpu->registers.pc);
      cpu->registers.pc += 2;
      writeByte(m, --cpu->registers.sp, ((cpu->registers.pc & 0xFF00) >> 8));
      writeByte(m, --cpu->registers.sp, (cpu->registers.pc & 0x00FF));
      cpu->registers.pc = address;
      cycles += 12;
      break;
    }

    /* CALL cc, nn ---------------------------------------------------------------------------*/
    case 0xC4: { // CALL NZ, nn
      MAKE_CALL_CC_NN_OPCODE_IMPL(FLAG_REGISTER_Z_BIT, FLAG_REGISTER_Z_BIT_SHIFT, 0)
    }
    case 0xCC: { // CALL Z, nn
      MAKE_CALL_CC_NN_OPCODE_IMPL(FLAG_REGISTER_Z_BIT, FLAG_REGISTER_Z_BIT_SHIFT, 1)
    }
    case 0xD4: { // CALL NC, nn
      MAKE_CALL_CC_NN_OPCODE_IMPL(FLAG_REGISTER_C_BIT, FLAG_REGISTER_C_BIT_SHIFT, 0)
    }
    case 0xDC: { // CALL C, nn
      MAKE_CALL_CC_NN_OPCODE_IMPL(FLAG_REGISTER_C_BIT, FLAG_REGISTER_C_BIT_SHIFT, 1)
    }

    /* Restarts *******************************************************************************/
    /* RST n ---------------------------------------------------------------------------------*/
    case 0xC7: { // RST 00
      MAKE_RST_N_OPCODE_IMPL(0x0000)
    }
    case 0xCF: { // RST 08
      MAKE_RST_N_OPCODE_IMPL(0x0008)
    }
    case 0xD7: { // RST 10
      MAKE_RST_N_OPCODE_IMPL(0x0010)
    }
    case 0xDF: { // RST 18
      MAKE_RST_N_OPCODE_IMPL(0x0018)
    }
    case 0xE7: { // RST 20
      MAKE_RST_N_OPCODE_IMPL(0x0020)
    }
    case 0xEF: { // RST 28
      MAKE_RST_N_OPCODE_IMPL(0x0028)
    }
    case 0xF7: { // RST 30
      MAKE_RST_N_OPCODE_IMPL(0x0030)
    }
    case 0xFF: { // RST 38
      MAKE_RST_N_OPCODE_IMPL(0x0038)
    }

    /* Returns ********************************************************************************/
    /* RET -----------------------------------------------------------------------------------*/
    case 0xC9: { // RET
      uint8_t addressLow = readByte(m, cpu->registers.sp++);
      uint8_t addressHigh = readByte(m, cpu->registers.sp++);
      cpu->registers.pc = (addressHigh << 8) | addressLow;
      cycles += 8;
      break;
    }

    /* RET cc --------------------------------------------------------------------------------*/
    case 0xC0: { // RET NZ
      MAKE_RET_CC_OPCODE_IMPL(FLAG_REGISTER_Z_BIT, FLAG_REGISTER_Z_BIT_SHIFT, 0)
    }
    case 0xC8: { // RET Z
      MAKE_RET_CC_OPCODE_IMPL(FLAG_REGISTER_Z_BIT, FLAG_REGISTER_Z_BIT_SHIFT, 1)
    }
    case 0xD0: { // RET NC
      MAKE_RET_CC_OPCODE_IMPL(FLAG_REGISTER_C_BIT, FLAG_REGISTER_C_BIT_SHIFT, 0)
    }
    case 0xD8: { // RET C
      MAKE_RET_CC_OPCODE_IMPL(FLAG_REGISTER_C_BIT, FLAG_REGISTER_C_BIT_SHIFT, 1)
    }

    /* RETI ----------------------------------------------------------------------------------*/
    case 0xD9: { // RETI
      // TODO: Check how IME is set - is there a single instruction delay before interrupts are enabled as with DI and EI?
      // Potentially not because this replicates the combination of the combination of "EI, RET" instructions,
      // in which EI would cause interrupts to be enabled AFTER the RET had executed, so the effect
      // of the single instruction delay would/should be replicated here.
      uint8_t addressLow = readByte(m, cpu->registers.sp++);
      uint8_t addressHigh = readByte(m, cpu->registers.sp++);
      cpu->registers.pc = (addressHigh << 8) | addressLow;
      cpu->ime = true;
      cycles += 8;
      break;
    }

    /* CB-Prefixed Opcodes ********************************************************************/
    case 0xCB: {
      uint8_t opcode2 = readByte(m, cpu->registers.pc++);
      printf("[0x%04X][0x%02X] %s\n", cpu->registers.pc - 1, opcode2, CB_OPCODE_MNEMONICS[opcode]);
      
      switch (opcode2) {
        /* Miscellaneous **********************************************************************/
        /* SWAP n ----------------------------------------------------------------------------*/
        case 0x37: { // SWAP A
          MAKE_SWAP_N_OPCODE_IMPL(a)
        }
        case 0x30: { // SWAP B
          MAKE_SWAP_N_OPCODE_IMPL(b)
        }
        case 0x31: { // SWAP C
          MAKE_SWAP_N_OPCODE_IMPL(c)
        }
        case 0x32: { // SWAP D
          MAKE_SWAP_N_OPCODE_IMPL(d)
        }
        case 0x33: { // SWAP E
          MAKE_SWAP_N_OPCODE_IMPL(e)
        }
        case 0x34: { // SWAP H
          MAKE_SWAP_N_OPCODE_IMPL(h)
        }
        case 0x35: { // SWAP L
          MAKE_SWAP_N_OPCODE_IMPL(l)
        }
        case 0x36: { // SWAP (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_C_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* Rotates and Shifts *****************************************************************/
        /* RLC n -----------------------------------------------------------------------------*/
        case 0x07: { // RLC A
          MAKE_RLC_N_OPCODE_IMPL(a)
        }
        case 0x00: { // RLC B
          MAKE_RLC_N_OPCODE_IMPL(b)
        }
        case 0x01: { // RLC C
          MAKE_RLC_N_OPCODE_IMPL(c)
        }
        case 0x02: { // RLC D
          MAKE_RLC_N_OPCODE_IMPL(d)
        }
        case 0x03: { // RLC E
          MAKE_RLC_N_OPCODE_IMPL(e)
        }
        case 0x04: { // RLC H
          MAKE_RLC_N_OPCODE_IMPL(h)
        }
        case 0x05: { // RLC L
          MAKE_RLC_N_OPCODE_IMPL(l)
        }
        case 0x06: { // RLC (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          cpu->registers.f |= (value & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); // NOTE: Set the C bit of F before we modify A
          value = (value << 1) | ((value & BIT_7) >> BIT_7_SHIFT);
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* RL n ------------------------------------------------------------------------------*/
        case 0x17: { // RL A
          MAKE_RL_N_OPCODE_IMPL(a)
        }
        case 0x10: { // RL B
          MAKE_RL_N_OPCODE_IMPL(b)
        }
        case 0x11: { // RL C
          MAKE_RL_N_OPCODE_IMPL(c)
        }
        case 0x12: { // RL D
          MAKE_RL_N_OPCODE_IMPL(d)
        }
        case 0x13: { // RL E
          MAKE_RL_N_OPCODE_IMPL(e)
        }
        case 0x14: { // RL H
          MAKE_RL_N_OPCODE_IMPL(h)
        }
        case 0x15: { // RL L
          MAKE_RL_N_OPCODE_IMPL(l)
        }
        case 0x16: { // RL (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          uint8_t oldCarryBit = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT;
          cpu->registers.f |= (value & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT); // NOTE: Set the C bit of F before we modify A
          value = (value << 1) | oldCarryBit;
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* RRC n -----------------------------------------------------------------------------*/
        case 0x0F: { // RRC A
          MAKE_RRC_N_OPCODE_IMPL(a)
        }
        case 0x08: { // RRC B
          MAKE_RRC_N_OPCODE_IMPL(b)
        }
        case 0x09: { // RRC C
          MAKE_RRC_N_OPCODE_IMPL(c)
        }
        case 0x0A: { // RRC D
          MAKE_RRC_N_OPCODE_IMPL(d)
        }
        case 0x0B: { // RRC E
          MAKE_RRC_N_OPCODE_IMPL(e)
        }
        case 0x0C: { // RRC H
          MAKE_RRC_N_OPCODE_IMPL(h)
        }
        case 0x0D: { // RRC L
          MAKE_RRC_N_OPCODE_IMPL(l)
        }
        case 0x0E: { // RRC (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          cpu->registers.f |= (value & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; // NOTE: Set the C bit of F before we modify A
          value = ((value & BIT_0) << BIT_7_SHIFT) | (value >> 1);
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* RR n ------------------------------------------------------------------------------*/
        case 0x1F: { // RR A
          MAKE_RR_N_OPCODE_IMPL(a)
        }
        case 0x18: { // RR B
          MAKE_RR_N_OPCODE_IMPL(b)
        }
        case 0x19: { // RR C
          MAKE_RR_N_OPCODE_IMPL(c)
        }
        case 0x1A: { // RR D
          MAKE_RR_N_OPCODE_IMPL(d)
        }
        case 0x1B: { // RR E
          MAKE_RR_N_OPCODE_IMPL(e)
        }
        case 0x1C: { // RR H
          MAKE_RR_N_OPCODE_IMPL(h)
        }
        case 0x1D: { // RR L
          MAKE_RR_N_OPCODE_IMPL(l)
        }
        case 0x1E: { // RR (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          uint8_t oldCarryBit = (cpu->registers.f & FLAG_REGISTER_C_BIT) >> FLAG_REGISTER_C_BIT_SHIFT;
          cpu->registers.f |= (value & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT; // NOTE: Set the C bit of F before we modify A
          value = (oldCarryBit << BIT_7_SHIFT) | (value >> 1);
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* SLA n -----------------------------------------------------------------------------*/
        case 0x27: { // SLA A
          MAKE_SLA_N_OPCODE_IMPL(a)
        }
        case 0x20: { // SLA B
          MAKE_SLA_N_OPCODE_IMPL(b)
        }
        case 0x21: { // SLA C
          MAKE_SLA_N_OPCODE_IMPL(c)
        }
        case 0x22: { // SLA D
          MAKE_SLA_N_OPCODE_IMPL(d)
        }
        case 0x23: { // SLA E
          MAKE_SLA_N_OPCODE_IMPL(e)
        }
        case 0x24: { // SLA H
          MAKE_SLA_N_OPCODE_IMPL(h)
        }
        case 0x25: { // SLA L
          MAKE_SLA_N_OPCODE_IMPL(l)
        }
        case 0x26: { // SLA (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          cpu->registers.f |= (value & BIT_7) >> (BIT_7_SHIFT - FLAG_REGISTER_C_BIT_SHIFT);
          value <<= 1;
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* SRA n -----------------------------------------------------------------------------*/
        case 0x2F: { // SRA A
          MAKE_SRA_N_OPCODE_IMPL(a)
        }
        case 0x28: { // SRA B
          MAKE_SRA_N_OPCODE_IMPL(b)
        }
        case 0x29: { // SRA C
          MAKE_SRA_N_OPCODE_IMPL(c)
        }
        case 0x2A: { // SRA D
          MAKE_SRA_N_OPCODE_IMPL(d)
        }
        case 0x2B: { // SRA E
          MAKE_SRA_N_OPCODE_IMPL(e)
        }
        case 0x2C: { // SRA H
          MAKE_SRA_N_OPCODE_IMPL(h)
        }
        case 0x2D: { // SRA L
          MAKE_SRA_N_OPCODE_IMPL(l)
        }
        case 0x2E: { // SRA (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          cpu->registers.f |= (value & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT;
          value = (value & BIT_7) | (value >> 1);
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* SRL n -----------------------------------------------------------------------------*/
        case 0x3F: { // SRL A
          MAKE_SRL_N_OPCODE_IMPL(a)
        }
        case 0x38: { // SRL B
          MAKE_SRL_N_OPCODE_IMPL(b)
        }
        case 0x39: { // SRL C
          MAKE_SRL_N_OPCODE_IMPL(c)
        }
        case 0x3A: { // SRL D
          MAKE_SRL_N_OPCODE_IMPL(d)
        }
        case 0x3B: { // SRL E
          MAKE_SRL_N_OPCODE_IMPL(e)
        }
        case 0x3C: { // SRL H
          MAKE_SRL_N_OPCODE_IMPL(h)
        }
        case 0x3D: { // SRL L
          MAKE_SRL_N_OPCODE_IMPL(l)
        }
        case 0x3E: { // SRL (HL)
          uint8_t value = readByte(m, (cpu->registers.h << 8) | cpu->registers.l);
          cpu->registers.f |= (value & BIT_0) << FLAG_REGISTER_C_BIT_SHIFT;
          value = value >> 1;
          writeByte(m, (cpu->registers.h << 8) | cpu->registers.l, value);
          cpu->registers.f |= (value == 0) << FLAG_REGISTER_Z_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_N_BIT_SHIFT;
          cpu->registers.f |= 0 << FLAG_REGISTER_H_BIT_SHIFT;
          cycles += 16;
          break;
        }

        /* Bit Opcodes ************************************************************************/
        /* BIT b, r --------------------------------------------------------------------------*/
        MAKE_BIT_B_R_OPCODE_GROUP(0, 0x47)
        MAKE_BIT_B_R_OPCODE_GROUP(1, 0x4F)
        MAKE_BIT_B_R_OPCODE_GROUP(2, 0x57)
        MAKE_BIT_B_R_OPCODE_GROUP(3, 0x5F)
        MAKE_BIT_B_R_OPCODE_GROUP(4, 0x67)
        MAKE_BIT_B_R_OPCODE_GROUP(5, 0x6F)
        MAKE_BIT_B_R_OPCODE_GROUP(6, 0x77)
        MAKE_BIT_B_R_OPCODE_GROUP(7, 0x7F)

        /* SET b, r --------------------------------------------------------------------------*/
        MAKE_SET_B_R_OPCODE_GROUP(0, 0xC7)
        MAKE_SET_B_R_OPCODE_GROUP(1, 0xCF)
        MAKE_SET_B_R_OPCODE_GROUP(2, 0xD7)
        MAKE_SET_B_R_OPCODE_GROUP(3, 0xDF)
        MAKE_SET_B_R_OPCODE_GROUP(4, 0xE7)
        MAKE_SET_B_R_OPCODE_GROUP(5, 0xEF)
        MAKE_SET_B_R_OPCODE_GROUP(6, 0xF7)
        MAKE_SET_B_R_OPCODE_GROUP(7, 0xFF)

        /* RES b, r --------------------------------------------------------------------------*/
        MAKE_RES_B_R_OPCODE_GROUP(0, 0x87)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0x8F)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0x97)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0x9F)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0xA7)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0xAF)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0xB7)
        MAKE_RES_B_R_OPCODE_GROUP(0, 0xBF)

        /**************************************************************************************/
        default: {
          fprintf(stderr, "FATAL ERROR: ENCOUNTERED UNKNOWN CB-PREFIXED OPCODE: 0x%02X\n", opcode2);
          exit(EXIT_FAILURE);
          break;
        }
      }
      break;
    }

    /******************************************************************************************/
    default: {
      fprintf(stderr, "FATAL ERROR: ENCOUNTERED UNKNOWN OPCODE: 0x%02X\n", opcode);
      exit(EXIT_FAILURE);
      break;
    }

  }

  return cycles;
}

void cpuUpdateIME(CPU* cpu)
{
  if (cpu->di == 1) {
    cpu->di++;
  } else if (cpu->di == 2) {
    cpu->ime = false;
    cpu->di = 0;
  }
  
  if (cpu->ei == 1) {
    cpu->ei++;
  } else if (cpu->ei == 2) {
    cpu->ime = true;
    cpu->ei = 0;
  }
}

void cpuFlagInterrupt(MemoryController* m, uint8_t interruptBit)
{
  uint8_t interruptsFlagged = readByte(m, IO_REG_ADDRESS_IF);
  interruptsFlagged |= interruptBit;
  writeByte(m, IO_REG_ADDRESS_IF, interruptsFlagged);
}

void cpuUnflagInterrupt(MemoryController* m, uint8_t interruptBit)
{
  uint8_t interruptsFlagged = readByte(m, IO_REG_ADDRESS_IF);
  interruptsFlagged ^= interruptBit; // TODO: Check this use of XOR to reset a single bit under the assumption that "interruptBit" is the correct value for the single bit being reset
  writeByte(m, IO_REG_ADDRESS_IF, interruptsFlagged);
}

void cpuHandleInterrupts(CPU* cpu, MemoryController* m)
{
  if (cpu->ime) {
    uint8_t interruptsFlagged = readByte(m, IO_REG_ADDRESS_IF);
    uint8_t interruptsEnabled = readByte(m, IO_REG_ADDRESS_IE);
    
    // Test each bit in order of interrupt priority (which, handily, is in the order of least- to most-significant bits)
    for (uint8_t bitOffset = 0; bitOffset < 5; bitOffset++) {
      
      if ((interruptsFlagged & interruptsEnabled) & (1 << bitOffset)) {
        // Disable ALL interrupts during the interrupt
        cpu->ime = false;
        
        // Push the program counter onto the stack
        writeByte(m, --cpu->registers.sp, ((cpu->registers.pc & 0xFF00) >> 8));
        writeByte(m, --cpu->registers.sp, (cpu->registers.pc & 0x00FF));
        
        // Jump to the starting address of the interrupt - here the interrupt address is at the offset of the matching bit location in the IE register
        const uint16_t interruptAddresses[] = {
          VBLANK_INTERRUPT_START_ADDRESS,
          LCDC_STATUS_INTERRUPT_START_ADDRESS,
          TIMER_OVERFLOW_INTERRUPT_START_ADDRESS,
          SERIAL_TRANSFER_COMPLETION_INTERRUPT_START_ADDRESS,
          HIGH_TO_LOW_P10_TO_P13_INTERRUPT_START_ADDRESS
        };
        cpu->registers.pc = interruptAddresses[bitOffset];
        
        // Reset the IF register bit of the interrupt being handled
        cpuUnflagInterrupt(m, 1 << bitOffset);
        
        break;
      }
    }
  }
}