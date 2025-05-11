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
#include <memory/host.h>
#include <memory/vaddr.h>
#include <device/map.h>
#include <utils.h>

#define IO_SPACE_MAX (2 * 1024 * 1024)

static uint8_t *io_space = NULL;
static uint8_t *p_space = NULL;

uint8_t* new_space(int size) {
  //size单位是字节
  uint8_t *p = p_space; //p_space指向空闲内存的开始
  /*
  为了把 size 向上对齐到页大小（PAGE_SIZE）的整数倍。也就是说，new_space是以页为单位来分配的。
  加 PAGE_SIZE - 1 是为了保证：
  如果 size 本身已经是页对齐的，就不变；如果不是，就进位到下一个页大小。
  */ 
  size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
  p_space += size;
  assert(p_space - io_space < IO_SPACE_MAX);
  return p;
}

static void check_bound(IOMap *map, paddr_t addr) {
  if (map == NULL) {
    Assert(map != NULL, "address (" FMT_PADDR ") is out of bound at pc = " FMT_WORD, addr, cpu.pc);
  } else {
    Assert(addr <= map->high && addr >= map->low,
        "address (" FMT_PADDR ") is out of bound {%s} [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
        addr, map->name, map->low, map->high, cpu.pc);
  }
}

static void invoke_callback(io_callback_t c, paddr_t offset, int len, bool is_write) {
  if (c != NULL) { c(offset, len, is_write); }
}

void init_map() {
  /*根据nemu$ make -B 链接的选项来看，map.c被链接到riscv32-nemu-interpreter，也就是最后malloc是host（Ubuntu）的内存
    为什么需要这样做呢？
    串口输出根本没有必要向host memory中写入数据...直接输出到stderr不就好了吗？
  */ 
  
  io_space = malloc(IO_SPACE_MAX);
  assert(io_space);
  p_space = io_space;
}

word_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  invoke_callback(map->callback, offset, len, false); // prepare data to read
  word_t ret = host_read(map->space + offset, len);   // 相当于从外设中读取数据
  #ifdef CONFIG_MTRACE
    dlog_write("IOMap_Read: Read  [0x%08x] from addr <0x%08x> [Device: %s, Offset: %08x] \t by inst@pc<0x%08x>\n", ret, addr, map->name, offset, cpu.pc);
  #endif
  return ret;
}

void map_write(paddr_t addr, int len, word_t data, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  host_write(map->space + offset, len, data);         // 相当于向外设中写入数据
  #ifdef CONFIG_MTRACE
    dlog_write("IOMap_Write: Write [0x%08x] to addr <0x%08x> [Device: %s, Offset: %08x] \t by inst@pc<0x%08x>\n", data, addr,map->name, offset, cpu.pc);
  #endif
  invoke_callback(map->callback, offset, len, true);
}
