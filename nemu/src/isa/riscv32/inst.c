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
//施工重点
#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_B, TYPE_J, TYPE_R,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0) //do {} while (0)用于防止这个宏在其他控制语句中被错误执行，将rs1对应寄存器的值赋给src1指向的位置
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0) //imm[11:0]
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0) // imm[31:12]后面全为0
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0) //拼成完整的imm[11:0]
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | BITS(i, 7, 7) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1; } while(0) //拼成imm[12:1]
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i, 30, 21) << 1;} while(0) //拼成imm[20:1]

void ftrace_jal(int rd, uint32_t pc, uint32_t dst);
void ftrace_jalr(int rd, uint32_t pc, uint32_t dst, uint32_t inst);

static inline void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val; //32位指令
  int rs1 = BITS(i, 19, 15); //寄存器1
  int rs2 = BITS(i, 24, 20); //寄存器2
  *rd     = BITS(i, 11, 7); //目标寄存器
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_R: src1R(); src2R();       ; break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
} // __VA_ARGS__是一个特殊的宏，可作为语句执行

#define OPCODE (cur_inst & 0x7f)
#define FUNC3 (cur_inst & 0x7000)
#define FUNC7 (cur_inst & 0xfe000000)
#define BREAK goto *(__instpat_end)


  INSTPAT_START();

  uint32_t cur_inst = s->isa.inst.val;
  // printf("OPCODE=%x", OPCODE);
  switch (OPCODE) {
    case 0x33: // R
      decode_operand(s, &rd, &src1, &src2, &imm, TYPE_R);
      switch (FUNC3) {
        case 0x0: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 + src2; BREAK;
            case 0x40000000: R(rd) = src1 - src2; BREAK;
            case 0x2000000: R(rd) = src1 * src2; BREAK;
          }
        case 0x1000: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 << (src2 & 0x1f); BREAK;
            case 0x2000000: R(rd) = (1ll * (int32_t) src1 * (int32_t) src2) >> 32; BREAK;
          }
        case 0x2000: 
          switch (FUNC7) {
            case 0x0: R(rd) = (int32_t) src1 < (int32_t) src2; BREAK;
            case 0x2000000: R(rd) = (1ll * (int32_t) src1 * (uint32_t) src2) >> 32; BREAK;
          }
        
        case 0x3000: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 < src2; BREAK;
            case 0x2000000: R(rd) = (1ull * (uint32_t) src1 * (uint32_t) src2) >> 32; BREAK;
          }
        
        case 0x4000: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 ^ src2; BREAK;
            case 0x2000000: R(rd) = (int32_t) src1 / (int32_t) src2; BREAK;
          }
        
        case 0x5000: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 >> (src2 & 0x1f); BREAK;
            case 0x40000000: R(rd) = (int32_t) src1 >> (src2 & 0x1f); BREAK;
            case 0x2000000: R(rd) = src1 / src2; BREAK;
          }
        case 0x6000: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 | src2; BREAK;
            case 0x2000000: R(rd) = (int32_t) src1 % (int32_t) src2; BREAK; 
          }
        
        case 0x7000: 
          switch (FUNC7) {
            case 0x0: R(rd) = src1 & src2; BREAK;
            case 0x2000000: R(rd) = src1 % src2; BREAK;
          }  
      }

      case 0x17: //U
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_U);
        R(rd) = s->pc + imm; BREAK;
      case 0x37: //U
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_U);
        R(rd) = imm; BREAK;

      case 0x13: //I
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_I);
        switch (FUNC3) {
          case 0x0: R(rd) = src1 + imm; BREAK;
          case 0x1000: 
            switch (FUNC7) {
              case 0x0: R(rd) = src1 << imm; BREAK;
            }
          case 0x2000: R(rd) = (int32_t) src1 < (int32_t) imm; BREAK;
          case 0x3000: R(rd) = src1 < imm; BREAK;
          case 0x4000: R(rd) = src1 ^ imm; BREAK;
          case 0x5000: 
            switch (FUNC7) {
              case 0x0: R(rd) = src1 >> imm; BREAK;
              case 0x40000000: R(rd) = (int32_t) src1 >> (imm & 0x1f); BREAK;
            }
          case 0x6000: R(rd) = src1 | imm; BREAK;
          case 0x7000: R(rd) = src1 & imm; BREAK;
        }
      case 0x3: //I
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_I);
        switch (FUNC3) {
          case 0x0: R(rd) = (word_t) ((int32_t) Mr(src1 + imm, 1)) << 24 >> 24; BREAK;
          case 0x1000: R(rd) = (word_t) ((int32_t) Mr(src1 + imm, 2) << 16 >> 16); BREAK;
          case 0x2000: R(rd) = Mr(src1 + imm, 4); BREAK;
          case 0x4000: R(rd) = Mr(src1 + imm, 1); BREAK;
          case 0x5000: R(rd) = Mr(src1 + imm, 2); BREAK;
        }
      case 0x67: //I
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_I);
        switch (FUNC3) {
          case 0x0: R(rd) = s->snpc, s-> dnpc = src1 + imm; IFDEF(CONFIG_FTRACE, ftrace_jalr(rd, s->pc, s->dnpc, s->isa.inst.val)); BREAK;
        }
      case 0x23: //S
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_S);
        switch (FUNC3) {
          case 0x0: Mw(src1 + imm, 1, src2); BREAK;
          case 0x1000: Mw(src1 + imm, 2, src2); BREAK;
          case 0x2000: Mw(src1 + imm, 4, src2); BREAK;
        }
      case 0x63: //B
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_B);
        switch (FUNC3) {
          case 0x0: s->dnpc = src1 == src2 ? s->pc + imm : s->dnpc; BREAK;
          case 0x1000: s->dnpc = src1 == src2 ? s->dnpc : s->pc + imm; BREAK;
          case 0x4000: s->dnpc = (int32_t) src1 < (int32_t) src2 ? s->pc + imm : s->dnpc; BREAK;
          case 0x5000: s->dnpc = (int32_t) src1 >= (int32_t) src2 ? s->pc + imm : s->dnpc; BREAK;
          case 0x6000: s->dnpc = src1 < src2 ? s->pc + imm : s->dnpc; BREAK;
          case 0x7000: s->dnpc = src1 >= src2 ? s->pc + imm : s->dnpc; BREAK;
        }
      case 0x6f:
        decode_operand(s, &rd, &src1, &src2, &imm, TYPE_J);
        R(rd) = s->snpc, s->dnpc = s->pc + imm; IFDEF(CONFIG_FTRACE, ftrace_jal(rd, s->pc, s->dnpc)); BREAK;
      case 0x73:
        NEMUTRAP(s->pc, R(10)); BREAK;
      Assert(0, "INVALID INST");
    }


  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  // printf("%x\n", s->isa.inst.val);
  return decode_exec(s);
}
