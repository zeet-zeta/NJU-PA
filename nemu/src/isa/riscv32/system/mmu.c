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

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  printf("vaddr = %x\n", vaddr);
  word_t satp = cpu.satp;
  paddr_t root_ppn = satp << 12;
  paddr_t first_pte_addr = root_ppn + ((vaddr >> 22) << 2);
  word_t first_pte = paddr_read(first_pte_addr, 4);
  assert(first_pte & PTE_V);
  paddr_t second_pte_addr = (first_pte >> 10 << 12) + ((vaddr >> 12 & 0x3ff) << 2);
  word_t second_pte = paddr_read(second_pte_addr, 4);
  assert(second_pte & PTE_V);
  paddr_t paddr = (second_pte >> 10 << 12) + (vaddr & 0xfff);
  return paddr;
}
