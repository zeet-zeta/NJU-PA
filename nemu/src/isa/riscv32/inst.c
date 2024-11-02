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


static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
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

  INSTPAT_START();

  //U-type 2
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);

  //R-type 10
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << (src2 & 0x1f));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = (int32_t) src1 < (int32_t) src2);

  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = src1 < src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> (src2 & 0x1f));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int32_t) src1 >> (src2 & 0x1f));

  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);

  //I-type 25
  //short imm
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = (int32_t) src1 < (int32_t) imm);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = src1 < imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);

  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << imm);
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> imm);

  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (int32_t) src1 >> (imm & 0x1f)); //屏蔽0100000的影响

  //load
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = (word_t) ((int32_t) Mr(src1 + imm, 1) << 24 >> 24));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = (word_t) ((int32_t) Mr(src1 + imm, 2) << 16 >> 16));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));

  //jump
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->snpc, s-> dnpc = src1 + imm);

  //csr todo


  //S-type 3
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2)); //src2是要存储到内存的数据

  //B-type 6
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc = src1 == src2 ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s->dnpc = src1 == src2 ? s->dnpc : s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, s->dnpc = (int32_t) src1 < (int32_t) src2 ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc = (int32_t) src1 >= (int32_t) src2 ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc = src1 < src2 ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc = src1 >= src2 ? s->pc + imm : s->dnpc);

  //J-type 1
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc, s->dnpc = s->pc + imm);

  //补充乘除法
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (1ll * (int32_t) src1 * (int32_t) src2) >> 32);
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R, R(rd) = (1ll * (int32_t) src1 * (uint32_t) src2) >> 32);
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = (1ull * (uint32_t) src1 * (uint32_t) src2) >> 32);
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (int32_t) src1 / (int32_t) src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (int32_t) src1 % (int32_t) src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1 % src2);

  //N-type 自己定义的
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  printf("%x\n", s->isa.inst.val);
  return decode_exec(s);
}
