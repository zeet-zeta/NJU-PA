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
#include <memory/vaddr.h>
#include <memory/paddr.h>

#define PTE_V 0x1

// paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
//   word_t satp = cpu.satp;
//   paddr_t root_ppn = satp << 12;
//   paddr_t first_pte_addr = root_ppn + (vaddr >> 22) * 4;
//   word_t first_pte = paddr_read(first_pte_addr, 4);
//   Assert(first_pte & PTE_V, "vaddr = %x first_pte_addr = %x first_pte = %x root_ppn = %x", vaddr, first_pte_addr, first_pte, root_ppn);
//   paddr_t second_pte_addr = (first_pte & ~0x0fff) + ((vaddr >> 12 & 0x3ff) * 4);
//   word_t second_pte = paddr_read(second_pte_addr, 4);
//   Assert(first_pte & PTE_V, "vaddr = %x second_pte_addr = %x first_pte = %x root_ppn = %x", vaddr, second_pte_addr, first_pte, root_ppn);
//   paddr_t paddr = (second_pte & ~0x0fff) + (vaddr & 0xfff);
//   return paddr;
// }

#ifndef PGSIZE
#define PGSIZE 4096
#endif
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  word_t pbase = ((word_t)cpu.satp) << 12;
  word_t vpn = (word_t)vaddr >> 12;
  word_t vpo = (word_t)vaddr & (PGSIZE - 1);
  word_t vpn1 = vpn >> 10;
  word_t vpn0 = vpn & (0x3ff);
  // Log("pbase: %x, vaddr: %x", pbase, vaddr);
  // Log("vpn1: %u, vpn0: %u", vpn1, vpn0);
  // Log("trans pbase: %x", (pbase + 4 * vpn1));
  word_t f_level = paddr_read(pbase | 4 * vpn1, 4);
  Assert(f_level & 1, "failed first translate at vaddr: %x, with base: %x ",
         vaddr, pbase);
  f_level = f_level & ~(PGSIZE - 1);
  // Log("trans f_level: %x", (f_level + 4 * vpn0));
  word_t s_level = paddr_read(f_level | 4 * vpn0, 4);
  Assert(s_level & 1, "failed second translate at vaddr: %x,with base: %x",
         vaddr, pbase);
  s_level = s_level & ~(PGSIZE - 1);
  // if ((s_level | vpo) != vaddr) {
  //   Log("trans: %x", (s_level | vpo));
  //   Log("orig: %x", vaddr);
  //   // assert(0);
  // }
  // assert((s_level | vpo) == vaddr);
  word_t ret = s_level | vpo;
  Assert((ret >= 0x80000000 && ret <= 0x88000000) ||
             (ret >= 0xa0000000 && ret <= 0xa0001000) ||
             (ret >= 0xa1000000 && ret <= 0xa1200000),
         "trans vaddr %x out of bound, with pabse %x", vaddr, pbase);
  return ret;
}
