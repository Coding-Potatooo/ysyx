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
#include <cpu/difftest.h>
#include "../local-include/reg.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
  // why pass in pc??? for what??? pc is already in cpu.pc!!!
  bool check_res = true;
  assert(sizeof(ref_r->gpr) == sizeof(cpu.gpr));
  for (int i = 0; i < sizeof(ref_r->gpr) / sizeof(ref_r->gpr[0]); i++)
  {
    if (cpu.gpr[i] != ref_r->gpr[i])
    {
      check_res = false;
      printf("\033[31mDifftest mismatch happen at %4s gpr[%2d] : %08x, ref_gpr[%2d]: %08x\033[0m\n", reg_name(i), i, cpu.gpr[i], i, ref_r->gpr[i]);
      break;
    }
  }
  check_res &= (cpu.pc == ref_r->pc);
  if (!check_res)
  {
    printf("pc: %08x, ref_pc: %08x\n", cpu.pc, ref_r->pc);
    for (int i = 0; i < sizeof(ref_r->gpr) / sizeof(ref_r->gpr[0]); i++)
    {
      printf("%4s gpr[%2d] : %08x, ref_gpr[%2d]: %08x\n", reg_name(i), i, cpu.gpr[i], i, ref_r->gpr[i]);
    }
  }

  #define DDDD_DEBUG
  #ifdef DDDD_DEBUG
  if (cpu.pc == 0x80000aa4) {
    printf("pc: %08x, ref_pc: %08x\n", cpu.pc, ref_r->pc);
    for (int i = 0; i < sizeof(ref_r->gpr) / sizeof(ref_r->gpr[0]); i++)
    {
      printf("%4s gpr[%2d] : %08x, ref_gpr[%2d]: %08x\n", reg_name(i), i, cpu.gpr[i], i, ref_r->gpr[i]);
    }
  }
  if (cpu.pc == 0x80000aa8) {
    printf("pc: %08x, ref_pc: %08x\n", cpu.pc, ref_r->pc);
    for (int i = 0; i < sizeof(ref_r->gpr) / sizeof(ref_r->gpr[0]); i++)
    {
      printf("%4s gpr[%2d] : %08x, ref_gpr[%2d]: %08x\n", reg_name(i), i, cpu.gpr[i], i, ref_r->gpr[i]);
    }
  }
  if (cpu.pc == 0x80000aac) {
    printf("pc: %08x, ref_pc: %08x\n", cpu.pc, ref_r->pc);
    for (int i = 0; i < sizeof(ref_r->gpr) / sizeof(ref_r->gpr[0]); i++)
    {
      printf("%4s gpr[%2d] : %08x, ref_gpr[%2d]: %08x\n", reg_name(i), i, cpu.gpr[i], i, ref_r->gpr[i]);
    }
  }
  if (cpu.pc == 0x80000ab0) {
    printf("pc: %08x, ref_pc: %08x\n", cpu.pc, ref_r->pc);
    for (int i = 0; i < sizeof(ref_r->gpr) / sizeof(ref_r->gpr[0]); i++)
    {
      printf("%4s gpr[%2d] : %08x, ref_gpr[%2d]: %08x\n", reg_name(i), i, cpu.gpr[i], i, ref_r->gpr[i]);
    }
  }
  #endif


  return check_res;
}

void isa_difftest_attach()
{
}
