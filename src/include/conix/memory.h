#ifndef CONIX__MEMORY_H
#define CONIX__MEMORY_H

#include "bitmap.h"
#include "types.h"

#define PAGE_SIZE 0x1000
#define MEMORY_BASE 0x100000

// 内核大小16M
#define KERNEL_MEMORY_SIZE 0x1000000
// 内核缓存地址
#define KERNEL_BUFFER_MEM 0x800000
#define KERNEL_BUFFER_SIZE 0x400000

// 内核虚拟磁盘地址
#define KERNEL_RAMDISK_MEM (KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE)
#define KERNEL_RAMDISK_SIZE 0x400000

// 用户栈顶地址256M
#define USER_STACK_TOP 0x8000000
#define USER_STACK_SIZE 0x200000
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

// 内核页目录索引地址
#define KERNEL_PAGE_DIR 0x1000
// 内核页表索引
static uint32 KERNEL_PAGE_TABLE[] = {
    0x2000, // 第一页起始地址
    0x3000, // 第二页起始地址
    0x4000,
    0x5000,
};

// 页表项4B
typedef struct page_entry_t {
  uint8 present : 1;  // 在内存中
  uint8 write : 1;    // 0 只读  1 可读可写
  uint8 user : 1;     // 1 所有人   0 超级用户 DPL<3
  uint8 pwt : 1;      // 1 直写模式    0 回写模式
  uint8 pcd : 1;      // 禁止该页缓冲
  uint8 accessed : 1; // 被访问过，统计使用频率
  uint8 dirty : 1;    // 脏页
  uint8 pat : 1;      // 页大小
  uint8 global : 1;   // 全局，所有进程都用到
  uint8 shared : 1;   // 共享内存页
  uint8 privat : 1;   // 私有内存页
  uint8 flag : 1;
  uint32 index : 20; // 页索引
} page_entry_t;

// 缺页地址放在cr2
uint32 get_cr2();

uint32 get_cr3();
// 设置页目录地址
void set_cr3(uint32 pde);

// 分配count个连续的内核页
uint32 alloc_kpage(uint32 count);
// 释放count个连续的内核页
void free_kpage(uint32 vaddr, uint32 count);

// 将vaddr映射物理内存
void link_page(uint32 vaddr);
void unlink_page(uint32 vaddr);

void flush_tlb(uint32 vaddr);

page_entry_t *copy_pde();

void free_pde();

int32 sys_brk(void *addr);

#endif