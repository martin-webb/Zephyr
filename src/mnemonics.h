const char* OPCODE_MNEMONICS[256] = {
  // 00
  "NOP", // x0
  "LD BC, nn", // x1
  "LD (BC), A", // x2
  "INC BC", // x3
  "INC B", // x4
  "DEC B", // x5
  "LD B, n", // x6
  "RLCA", // x7
  "LD (nn), SP", // x8
  "ADD HL, BC", // x9
  "LD A, (BC)", // xA
  "DEC BC", // xB
  "INC C", // xC
  "DEC C", // xD
  "LD C, n", // xE
  "RRCA", // xF
  
  // 10
  "STOP", // x0 (may also be encoded as 10 00, but assemblers typically just use 10)
  "LD DE, nn", // x1
  "LD (DE), A", // x2
  "INC DE", // x3
  "INC D", // x4
  "DEC D", // x5
  "LD D, n", // x6
  "RLA", // x7
  "JR n", // x8
  "ADD HL, DE", // x9
  "LD A, (DE)", // xA
  "DEC DE", // xB
  "INC E", // xC
  "DEC E", // xD
  "LD E, n", // xE
  "RRA", // xF
  
  // 20
  "JR NZ, *", // x0
  "LD HL, nn", // x1
  "LDI (HL), A", // x2 - also LD (HLI), A or LD (HL+), A
  "INC HL", // x3
  "INC H", // x4
  "DEC H", // x5
  "LD H, n", // x6
  "DAA", // x7
  "JR Z, *", // x8
  "ADD HL, (HL)", // x9
  "LDI A, (HL)", // xA - also LD A, (HLI) or LD A, (HL+)
  "DEC HL", // xB
  "INC L", // xC
  "DEC L", // xD
  "LD L, n", // xE
  "CPL", // xF
  
  // 30
  "JR NC, *", // x0
  "LD SP, nn", // x1
  "LDD (HL), A", // x2 - also LD (HLD), A or LD (HL-), A
  "INC SP", // x3
  "INC (HL)", // x4
  "DEC (HL)", // x5
  "LD (HL), n", // x6
  "SCF", // x7
  "JR C, *", // x8
  "ADD HL, SP", // x9
  "LDD A, (HL)", // xA - also LD A, (HLD) or LD A, (HL-)
  "DEC SP", // xB
  "INC A", // xC
  "DEC A", // xD
  "LD A, #", // xE
  "CCF", // xF
  
  // 40
  "LD B, B", // x0
  "LD B, C", // x1
  "LD B, D", // x2
  "LD B, E", // x3
  "LD B, H", // x4
  "LD B, L", // x5
  "LD B, (HL)", // x6
  "LD B, A", // x7
  "LD C, B", // x8
  "LD C, C", // x9
  "LD C, D", // xA
  "LD C, E", // xB
  "LD C, H", // xC
  "LD C, L", // xD
  "LD C, (HL)", // xE
  "LD C, A", // xF
  
  // 50
  "LD D, B", // x0
  "LD D, C", // x1
  "LD D, D", // x2
  "LD D, E", // x3
  "LD D, H", // x4
  "LD D, L", // x5
  "LD D, (HL)", // x6
  "LD D, A", // x7
  "LD E, B", // x8
  "LD E, C", // x9
  "LD E, D", // xA
  "LD E, E", // xB
  "LD E, H", // xC
  "LD E, L", // xD
  "LD E, (HL)", // xE
  "LD E, A", // xF
  
  // 60
  "LD H, B", // x0
  "LD H, C", // x1
  "LD H, D", // x2
  "LD H, E", // x3
  "LD H, H", // x4
  "LD H, L", // x5
  "LD H, (HL)", // x6
  "LD H, A", // x7
  "LD L, B", // x8
  "LD L, C", // x9
  "LD L, D", // xA
  "LD L, E", // xB
  "LD L, H", // xC
  "LD L, L", // xD
  "LD L, (HL)", // xE
  "LD L, A", // xF
  
  // 70
  "LD (HL), B", // x0
  "LD (HL), C", // x1
  "LD (HL), D", // x2
  "LD (HL), E", // x3
  "LD (HL), H", // x4
  "LD (HL), L", // x5
  "HALT", // x6
  "LD (HL), A", // x7
  "LD A, B", // x8
  "LD A, C", // x9
  "LD A, D", // xA
  "LD A, E", // xB
  "LD A, H", // xC
  "LD A, L", // xD
  "LD A, (HL)", // xE
  "LD A, A", // xF
  
  // 80
  "ADD A, B", // x0
  "ADD A, C", // x1
  "ADD A, D", // x2
  "ADD A, E", // x3
  "ADD A, H", // x4
  "ADD A, L", // x5
  "ADD A, (HL)", // x6
  "ADD A, A", // x7
  "ADC A, B", // x8
  "ADC A, C", // x9
  "ADC A, D", // xA
  "ADC A, E", // xB
  "ADC A, H", // xC
  "ADC A, L", // xD
  "ADC A, (HL)", // xE
  "ADC A, A", // xF
  
  // 90
  "SUB B", // x0
  "SUB C", // x1
  "SUB D", // x2
  "SUB E", // x3
  "SUB H", // x4
  "SUB L", // x5
  "SUB (HL)", // x6
  "SUB A", // x7
  "SBC A, B", // x8
  "SBC A, C", // x9
  "SBC A, D", // xA
  "SBC A, E", // xB
  "SBC A, H", // xC
  "SBC A, L", // xD
  "SBC A, (HL)", // xE
  "SBC A, A", // xF
  
  // A0
  "AND B", // x0
  "AND C", // x1
  "AND D", // x2
  "AND E", // x3
  "AND H", // x4
  "AND L", // x5
  "AND (HL)", // x6
  "AND A", // x7
  "XOR B", // x8
  "XOR C", // x9
  "XOR D", // xA
  "XOR E", // xB
  "XOR H", // xC
  "XOR L", // xD
  "XOR (HL)", // xE
  "XOR A", // xF
  
  // B0
  "OR B", // x0
  "OR C", // x1
  "OR D", // x2
  "OR E", // x3
  "OR H", // x4
  "OR L", // x5
  "OR (HL)", // x6
  "OR A", // x7
  "CP B", // x8
  "CP C", // x9
  "CP D", // xA
  "CP E", // xB
  "CP H", // xC
  "CP L", // xD
  "CP (HL)", // xE
  "CP A", // xF
  
  // C0
  "RET NZ", // x0
  "POP BC", // x1
  "JP NZ, nn", // x2
  "JP nn", // x3
  "CALL NZ, nn", // x4
  "PUSH BC", // x5
  "ADD A, #", // x6
  "RST 00H", // x7
  "RET Z", // x8
  "RET", // x9
  "JP Z, nn", // xA
  "PREFIX CB", // xB
  "CALL Z, nn", // xC
  "CALL nn", // xD
  "ADC A, #", // xE
  "RST 08H", // xF
  
  // D0
  "RET NC", // x0
  "POP DE", // x1
  "JP NC, nn", // x2
  "UNKNOWN", // x3
  "CALL NC, nn", // x4
  "PUSH DE", // x5
  "SUB #", // x6
  "RST 10H", // x7
  "RET C", // x8
  "RETI", // x9
  "JP C, nn", // xA
  "UNKNOWN", // xB
  "CALL C, nn", // xC
  "UNKNOWN", // xD
  "SBC A, #", // xE
  "RST 18H", // xF
  
  // E0
  "LD ($FF00 + n), A", // x0
  "POP HL", // x1
  "LD ($FF00 + C), A", // x2
  "UNKNOWN", // x3
  "UNKNOWN", // x4
  "PUSH HL", // x5
  "AND #", // x6
  "RST 20H", // x7
  "ADD SP, #", // x8
  "JP (HL)", // x9
  "LD (nn), A", // xA
  "UNKNOWN", // xB
  "UNKNOWN", // xC
  "UNKNOWN", // xD
  "XOR #", // xE
  "RST 28H", // xF
  
  // F0
  "LD A, ($FF00 + n)", // x0
  "POP AF", // x1
  "LD A, (C)", // x2
  "DI", // x3
  "UNKNOWN", // x4
  "PUSH AF", // x5
  "OR #", // x6
  "RST 30H", // x7
  "LDHL SP, n", // x8 - also LD HL, SP + n
  "LD SP, HL", // x9
  "LD A, (nn)", // xA
  "EI", // xB
  "UNKNOWN", // xC
  "UNKNOWN", // xD
  "CP #", // xE
  "RST 38H"  // xF
};

const char* CB_OPCODE_MNEMONICS[256] = {
  // 00
  "RLC B", // x0
  "RLC C", // x1
  "RLC D", // x2
  "RLC E", // x3
  "RLC H", // x4
  "RLC L", // x5
  "RLC (HL)", // x6
  "RLC A", // x7
  "RRC B", // x8
  "RRC C", // x9
  "RRC D", // xA
  "RRC E", // xB
  "RRC H", // xC
  "RRC L", // xD
  "RRC (HL)", // xE
  "RRC A", // xF
  
  // 10
  "RL B", // x0
  "RL C", // x1
  "RL D", // x2
  "RL E", // x3
  "RL H", // x4
  "RL L", // x5
  "RL (HL)", // x6
  "RL A", // x7
  "RR B", // x8
  "RR C", // x9
  "RR D", // xA
  "RR E", // xB
  "RR H", // xC
  "RR L", // xD
  "RR (HL)", // xE
  "RR A", // xF
  
  // 20
  "SLA B", // x0
  "SLA C", // x1
  "SLA D", // x2
  "SLA E", // x3
  "SLA H", // x4
  "SLA L", // x5
  "SLA (HL)", // x6
  "SLA A", // x7
  "SRA B", // x8
  "SRA C", // x9
  "SRA D", // xA
  "SRA E", // xB
  "SRA H", // xC
  "SRA L", // xD
  "SRA (HL)", // xE
  "SRA A", // xF
  
  // 30
  "SWAP B", // x0
  "SWAP C", // x1
  "SWAP D", // x2
  "SWAP E", // x3
  "SWAP H", // x4
  "SWAP L", // x5
  "SWAP (HL)", // x6
  "SWAP A", // x7
  "SRL B", // x8
  "SRL C", // x9
  "SRL D", // xA
  "SRL E", // xB
  "SRL H", // xC
  "SRL L", // xD
  "SRL (HL)", // xE
  "SRL A", // xF
  
  // 40
  "BIT 0, B", // x0
  "BIT 0, C", // x1
  "BIT 0, D", // x2
  "BIT 0, E", // x3
  "BIT 0, H", // x4
  "BIT 0, L", // x5
  "BIT 0, (HL)", // x6
  "BIT 0, A", // x7
  "BIT 1, B", // x8
  "BIT 1, C", // x9
  "BIT 1, D", // xA
  "BIT 1, E", // xB
  "BIT 1, H", // xC
  "BIT 1, L", // xD
  "BIT 1, (HL)", // xE
  "BIT 1, A", // xF
  
  // 50
  "BIT 2, B", // x0
  "BIT 2, C", // x1
  "BIT 2, D", // x2
  "BIT 2, E", // x3
  "BIT 2, H", // x4
  "BIT 2, L", // x5
  "BIT 2, (HL)", // x6
  "BIT 2, A", // x7
  "BIT 3, B", // x8
  "BIT 3, C", // x9
  "BIT 3, D", // xA
  "BIT 3, E", // xB
  "BIT 3, H", // xC
  "BIT 3, L", // xD
  "BIT 3, (HL)", // xE
  "BIT 3, A", // xF
  
  // 60
  "BIT 4, B", // x0
  "BIT 4, C", // x1
  "BIT 4, D", // x2
  "BIT 4, E", // x3
  "BIT 4, H", // x4
  "BIT 4, L", // x5
  "BIT 4, (HL)", // x6
  "BIT 4, A", // x7
  "BIT 5, B", // x8
  "BIT 5, C", // x9
  "BIT 5, D", // xA
  "BIT 5, E", // xB
  "BIT 5, H", // xC
  "BIT 5, L", // xD
  "BIT 5, (HL)", // xE
  "BIT 5, A", // xF
  
  // 70
  "BIT 6, B", // x0
  "BIT 6, C", // x1
  "BIT 6, D", // x2
  "BIT 6, E", // x3
  "BIT 6, H", // x4
  "BIT 6, L", // x5
  "BIT 6, (HL)", // x6
  "BIT 6, A", // x7
  "BIT 7, B", // x8
  "BIT 7, C", // x9
  "BIT 7, D", // xA
  "BIT 7, E", // xB
  "BIT 7, H", // xC
  "BIT 7, L", // xD
  "BIT 7, (HL)", // xE
  "BIT 7, A", // xF
  
  // 80
  "RES 0, B", // x0
  "RES 0, C", // x1
  "RES 0, D", // x2
  "RES 0, E", // x3
  "RES 0, H", // x4
  "RES 0, L", // x5
  "RES 0, (HL)", // x6
  "RES 0, A", // x7
  "RES 1, B", // x8
  "RES 1, C", // x9
  "RES 1, D", // xA
  "RES 1, E", // xB
  "RES 1, H", // xC
  "RES 1, L", // xD
  "RES 1, (HL)", // xE
  "RES 1, A", // xF
  
  // 90
  "RES 2, B", // x0
  "RES 2, C", // x1
  "RES 2, D", // x2
  "RES 2, E", // x3
  "RES 2, H", // x4
  "RES 2, L", // x5
  "RES 2, (HL)", // x6
  "RES 2, A", // x7
  "RES 3, B", // x8
  "RES 3, C", // x9
  "RES 3, D", // xA
  "RES 3, E", // xB
  "RES 3, H", // xC
  "RES 3, L", // xD
  "RES 3, (HL)", // xE
  "RES 3, A", // xF
  
  // A0
  "RES 4, B", // x0
  "RES 4, C", // x1
  "RES 4, D", // x2
  "RES 4, E", // x3
  "RES 4, H", // x4
  "RES 4, L", // x5
  "RES 4, (HL)", // x6
  "RES 4, A", // x7
  "RES 5, B", // x8
  "RES 5, C", // x9
  "RES 5, D", // xA
  "RES 5, E", // xB
  "RES 5, H", // xC
  "RES 5, L", // xD
  "RES 5, (HL)", // xE
  "RES 5, A", // xF
  
  // B0
  "RES 6, B", // x0
  "RES 6, C", // x1
  "RES 6, D", // x2
  "RES 6, E", // x3
  "RES 6, H", // x4
  "RES 6, L", // x5
  "RES 6, (HL)", // x6
  "RES 6, A", // x7
  "RES 7, B", // x8
  "RES 7, C", // x9
  "RES 7, D", // xA
  "RES 7, E", // xB
  "RES 7, H", // xC
  "RES 7, L", // xD
  "RES 7, (HL)", // xE
  "RES 7, A", // xF
  
  // C0
  "SET 0, B", // x0
  "SET 0, C", // x1
  "SET 0, D", // x2
  "SET 0, E", // x3
  "SET 0, H", // x4
  "SET 0, L", // x5
  "SET 0, (HL)", // x6
  "SET 0, A", // x7
  "SET 1, B", // x8
  "SET 1, C", // x9
  "SET 1, D", // xA
  "SET 1, E", // xB
  "SET 1, H", // xC
  "SET 1, L", // xD
  "SET 1, (HL)", // xE
  "SET 1, A", // xF
  
  // D0
  "SET 2, B", // x0
  "SET 2, C", // x1
  "SET 2, D", // x2
  "SET 2, E", // x3
  "SET 2, H", // x4
  "SET 2, L", // x5
  "SET 2, (HL)", // x6
  "SET 2, A", // x7
  "SET 3, B", // x8
  "SET 3, C", // x9
  "SET 3, D", // xA
  "SET 3, E", // xB
  "SET 3, H", // xC
  "SET 3, L", // xD
  "SET 3, (HL)", // xE
  "SET 3, A", // xF
  
  // E0
  "SET 4, B", // x0
  "SET 4, C", // x1
  "SET 4, D", // x2
  "SET 4, E", // x3
  "SET 4, H", // x4
  "SET 4, L", // x5
  "SET 4, (HL)", // x6
  "SET 4, A", // x7
  "SET 5, B", // x8
  "SET 5, C", // x9
  "SET 5, D", // xA
  "SET 5, E", // xB
  "SET 5, H", // xC
  "SET 5, L", // xD
  "SET 5, (HL)", // xE
  "SET 5, A", // xF
  
  // F0
  "SET 6, B", // x0
  "SET 6, C", // x1
  "SET 6, D", // x2
  "SET 6, E", // x3
  "SET 6, H", // x4
  "SET 6, L", // x5
  "SET 6, (HL)", // x6
  "SET 6, A", // x7
  "SET 7, B", // x8
  "SET 7, C", // x9
  "SET 7, D", // xA
  "SET 7, E", // xB
  "SET 7, H", // xC
  "SET 7, L", // xD
  "SET 7, (HL)", // xE
  "SET 7, A", // xF
};