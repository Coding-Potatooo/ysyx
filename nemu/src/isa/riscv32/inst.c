/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

/*
Total 6 types of instructions.
four core instruction formats(R I S U)
B, J are based on the handling of immediates.
*/
enum
{
  TYPE_R,
  TYPE_I,
  TYPE_S,
  TYPE_B,
  TYPE_U,
  TYPE_J,

  TYPE_N, // none
};

/*
unpriv-isa-asciidoc_20240411
The RISC-V ISA keeps the source (rs1 and rs2) and destination (rd) registers at the same position in all
 formats to simplify decoding.
*/
#define src1R()     \
  do                \
  {                 \
    *src1 = R(rs1); \
  } while (0)
#define src2R()     \
  do                \
  {                 \
    *src2 = R(rs2); \
  } while (0)

/*
unpriv-isa-asciidoc_20240411
Except for the 5-bit immediates used in CSR instructions (Chapter 7), immediates are always !!sign-extended!!, and are generally packed towards the leftmost available bits in
 the instruction and have been allocated to reduce hardware complexity.

In particular, the sign bit for all immediates is always in bit 31 of the instruction to speed sign-extension circuitry.
*/
/*
to finish immediate extraction part, refer to unpriv-isa-asciidoc_20240411 - 2.3. Immediate Encoding Variants
especially!!!
"Figure 1. Types of immediate produced by RISC-V instructions"
!!!
 Immediate types shows the immediates produced by each of the base instruction formats, and is labeled to show which instruction bit (inst[y]) produces each bit of the immediate value.
*/
#define immI()                        \
  do                                  \
  {                                   \
    *imm = SEXT(BITS(i, 31, 20), 12); \
  } while (0)
#define immS()                                               \
  do                                                         \
  {                                                          \
    *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
  } while (0)

#define immB()                                                                                                  \
  do                                                                                                            \
  {                                                                                                             \
    *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | BITS(i, 7, 7) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1; \
  } while (0)
#define immU()                              \
  do                                        \
  {                                         \
    *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
  } while (0)

#define immJ()                                                                                                                             \
  do                                                                                                                                       \
  {                                                                                                                                        \
    *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 24, 21) << 1; \
  } while (0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type)
{
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd = BITS(i, 11, 7);
  switch (type)
  {
    /*
    Refer to unpriv-isa-asciidoc_20240411, 2.2. Base Instruction Formats
    */
  case TYPE_R:
    src1R();
    src2R();
    break;
  case TYPE_I:
    src1R();
    immI();
    break;
  case TYPE_S:
    src1R();
    src2R();
    immS();
    break;
  case TYPE_U:
    immU();
    break;
  case TYPE_B:
    src1R();
    src2R();
    immB();
    break;
  case TYPE_J:
    immJ();
    break;
  }
}

static int decode_exec(Decode *s)
{
// #define DEBUG_DDDD
#ifdef DEBUG_DDDD
  printf("DEBUG_DDDD:instruction[%08x]  @pc[%08x]\n", (s)->isa.inst.val, s->pc);

#endif

  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* ... stands for the execute body */) \
  {                                                                             \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type));            \
    __VA_ARGS__; /*the execute body*/                                           \
  }

  INSTPAT_START();
  /*
  INSTPAT should be arranged in the order of descending used frequency of corresponding instruction.
  */

  /*
  AUIPC (add upper immediate to pc) is used to build pc-relative addresses and uses the *U-type* format.
  AUIPC forms a 32-bit offset from the U-immediate, filling in the lowest 12 bits with zeros, adds this offset to the address of the AUIPC instruction, then places the result in register rd.
  */
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm); /*add upper immediate to pc*/
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(rd) = imm);           /*load upper immediate*/

  /*
  Plain unconditional jumps (assembler pseudoinstruction J) are encoded as a JAL with rd=x0.
  Note that $zero is reset to zero at the end of decode_exec()... so J can be pattern matched and implemented by JAL.
  */
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, s->dnpc = s->pc + imm; R(rd) = s->pc + 4;); /*jump and link*/

  /*
  ret is a pseudoinstruction, which is expanded to jalr x0, 0(x1)
  according to JAL, the standard software calling convention uses 'x1' as the return address register.
  */
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr /*jump and link register*/, I,
          s->dnpc = (src1 + imm) & (((1ull << (32)) - 1) - 1); /*#define BITMASK(bits) ((1ull << (bits)) - 1); set the least-significant bit of the result to zero*/
          R(rd) = s->pc + 4;);

  /*
   All branch instructions use the B-type instruction format.
   The 12-bit B-immediate encodes signed offsets in multiples of 2 bytes. The offset is sign-extended and added to the address of the branch
   instruction to give the target address. The conditional branch range is ±4 KiB.

    Branch instructions compare two registers.
    BEQ and BNE take the branch if registers rs1 and rs2 are equal or unequal respectively.
    BLT and BLTU take the branch if rs1 is less than rs2, using signed and unsigned comparison respectively.
    BGE and BGEU take the branch if rs1 is greater than or equal to rs2, using signed and unsigned comparison respectively.
    Note, BGT, BGTU, BLE, and BLEU can be synthesized by reversing the operands to BLT, BLTU, BGE, and BGEU, respectively.
  */
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, s->dnpc = (src1 == src2) ? s->pc + imm : s->pc + 4);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, s->dnpc = (src1 != src2) ? s->pc + imm : s->pc + 4);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, s->dnpc = ((sword_t)src1 < (sword_t)src2) ? s->pc + imm : s->pc + 4);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, s->dnpc = ((sword_t)src1 >= (sword_t)src2) ? s->pc + imm : s->pc + 4);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, s->dnpc = (src1 < src2) ? s->pc + imm : s->pc + 4);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, s->dnpc = (src1 >= src2) ? s->pc + imm : s->pc + 4);
  /*
  Load and store instructions transfer a value between the registers and memory.
  Loads are encoded in the I-type format and stores are S-type.
  The effective address is obtained by adding register rs1 to the sign-extended 12-bit offset.
  Loads copy a value from memory to register rd. Stores copy the value in register rs2 to memory.
  */

  /* Registers, memory data types are all uint.
   */
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(rd) = Mr(src1 + imm, 4));             // The LW instruction loads a 32-bit value from memory into rd.
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(rd) = SEXT((Mr(src1 + imm, 2)), 16)); // LH loads a 16-bit value from memory, then sign-extends to 32-bits before storing in rd.
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(rd) = Mr(src1 + imm, 2));            // LHU loads a 16-bit value from memory but then zero extends to 32-bits before storing in rd.
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(rd) = SEXT((Mr(src1 + imm, 1)), 8));  // LB and LBU are defined analogously for 8-bit values.
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));

  /*
  The SW, SH, and SB instructions store 32-bit, 16-bit, and 8-bit values from the low bits of register rs2 to memory.
  */
  // Mw(vaddr_t addr, int len, word_t data)
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2)); /*store byte*/
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, src2)); /*store half word*/
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2)); /*store word*/

  /* li is a pseudoinstruction, it can be pattern matched and implemented by addi. */
  /* ADDI rd, rs1, 0 is used to implement the MV rd, rs1 assembler pseudoinstruction.*/
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R(rd) = (int32_t)src1 < (int32_t)imm ? 1 : 0); /*set less than immediate*/
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(rd) = src1 < imm ? 1 : 0);                  /*SLTIU is similar to SLTI but compares the values as unsigned numbers (i.e., the immediate is first sign-extended to XLEN bits then treated as an unsigned number). */
  /* ANDI, ORI, XORI are logical operations that perform bitwise AND, OR, and XOR on register rs1 and the sign-extended 12-bit immediate and place the result in rd.*/
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(rd) = src1 & imm);

  /* Shifts by a constant are encoded as a specialization of the I-type format. The operand to be shifted is in rs1, and the shift amount is encoded in the lower 5 bits of the I-immediate field.
  The right shift type is encoded in bit 30.
  SLLI is a logical left shift (zeros are shifted into the lower bits);
  SRLI is a logical right shift (zeros are shifted into the upper bits);
  SRAI is an arithmetic right shift (the original sign bit is copied into the vacated upper bits).*/
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli, I, R(rd) = src1 << (imm & BITMASK(5)));          /*Shift Left Logical Immediate*/
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli, I, R(rd) = src1 >> (imm & BITMASK(5)));          /*Shift Right Logical Immediate*/
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai, I, R(rd) = (int32_t)src1 >> (imm & BITMASK(5))); /*Shift Right Arithmetic Immediate*/

  /*
  RV32I defines several arithmetic R-type operations. All operations read the rs1 and rs2 registers as source operands and write the result into register rd.
  The funct7 and funct3 fields select the type of operation.
  */
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(rd) = src1 - src2);                           /*SUB performs the subtraction of rs2 from rs1.*/
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R(rd) = (int32_t)src1 < (int32_t)src2 ? 1 : 0); /*Set Less Than: SLT and SLTU perform signed and unsigned compares respectively, writing 1 to rd if rs1 < rs2, 0 otherwise. */
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R(rd) = src1 < src2 ? 1 : 0);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, R(rd) = src1 << (src2 & BITMASK(5)));          /*Shift Left Logical*/
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, R(rd) = src1 >> (src2 & BITMASK(5)));          /*Shift Right Logical*/
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra, R, R(rd) = (int32_t)src1 >> (src2 & BITMASK(5))); /*Shift Right Arithmetic*/
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R(rd) = src1 & src2);

  // INSTPAT("", fence, ,);
  // INSTPAT("", fence_tso, ,);

  // INSTPAT("", pause, ,);

  /* RV32M Standard Extension
  We separate integer multiply and divide out from the base to simplify low-end implementations,
  or for applications where integer multiply and divide operations are either infrequent or better handled in attached accelerators.
  */

  /*MUL performs an XLEN-bit×XLEN-bit multiplication of rs1 by rs2 and places the lower XLEN bits in the destination register.
  MULH, MULHU, and MULHSU perform the same multiplication but return the upper XLEN bits of the full 2×XLEN-bit product, for signed×signed, unsigned×unsigned, and rs1×unsigned rs2 multiplication, respectively.*/
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, R(rd) = src1 * src2);
  // INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh, R, R(rd) = ((int64_t)((uint64_t)src1) * (int64_t)((uint64_t)src2)) >> 32); // wrong implementation! but why?
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh, R, R(rd) = ((int64_t)(int32_t)src1 * (int64_t)(int32_t)src2) >> 32);
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu, R, R(rd) = ((int64_t)((int32_t)src1) * (uint64_t)src2) >> 32);
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu, R, R(rd) = ((uint64_t)src1 * (uint64_t)src2) >> 32);

  /*DIV and DIVU perform an XLEN bits by XLEN bits signed and unsigned integer division of rs1 by rs2, rounding towards zero.*/
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div_, R, R(rd) = (int32_t)src1 / (int32_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, R, R(rd) = src1 / src2);
  /*REM and REMU provide the remainder of the corresponding division operation. For REM, the sign of a nonzero result equals the sign of the dividend(divident / divisor).*/
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, R(rd) = (int32_t)src1 % (int32_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, R, R(rd) = src1 % src2);

  // INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall, N, );
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  /*
  在模式匹配过程的最后有一条inv的规则, 表示"若前面所有的模式匹配规则都无法成功匹配, 则将该指令视为非法指令
  INV must be placed at last.
  */
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s)
{
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
