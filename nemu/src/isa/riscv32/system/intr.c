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

#include <isa.h>

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */

  // return 0;
  cpu.mcause = NO;
  cpu.mepc = epc + 4;
  return cpu.mtvec;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}

word_t *isa_csr_translate(word_t csr_addr) {
  switch (csr_addr) {
    case 0x305: return &cpu.mtvec;
    case 0x300: return &cpu.mstatus;
    case 0x341: return &cpu.mepc;
    case 0x342: return &cpu.mcause;
    case 0x180: return &cpu.satp;
    default: Assert(0, "unrecognized CSR: %x", csr_addr);
  }
}