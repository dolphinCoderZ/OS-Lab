#include "../include/conix/memory.h"
#include "../include/conix/assert.h"
#include "../include/conix/conix.h"
#include "../include/conix/debug.h"
#include "../include/conix/stdlib.h"
#include "../include/conix/string.h"
#include "../include/conix/task.h"
#include "../include/conix/types.h"

#define LOG_DEBUG(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID 1   // ards可用区域
#define ZONE_RESERED 2 // ards不可用区域

#define IDX(addr) ((uint32)addr >> 12)            // 取页索引
#define DIDX(addr) (((uint32)addr >> 22) & 0x3ff) // 页目录索引
#define TIDX(addr) (((uint32)addr >> 12) & 0x3ff) // 页表索引
#define PAGE(idx) ((uint32)idx << 12)             // 相应页起始地址
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

#define PDE_MASK 0xFFC00000

// 内核页表索引
#define KERNEL_MAP_BITS 0x6000

bitmap_t kernel_map;

typedef struct ards_t {
  uint64 base; // 内存基地址
  uint64 size; // 内存长度
  uint32 type;
} _packed ards_t;

static uint32 memory_base = 0; // 可用内存的基地址
static uint32 memory_size = 0; // 可用内存大小
static uint32 total_pages = 0;
static uint32 free_pages = 0;
#define used_pages (total_pages - free_pages)

// 初始化物理内存起始地址等参数
void memory_init(uint32 magic, uint32 addr) {
  uint32 count = 0;

  // 如果是conix loader进入的内核
  if (magic == CONIX_MAGIC) {
    count = *(uint32 *)addr;
    ards_t *ptr = (ards_t *)(addr + 4);

    for (size_t i = 0; i < count; ++i, ++ptr) {
      LOG_DEBUG("Memory base 0x%p size 0x%p type %d\n", (uint32)ptr->base,
                (uint32)ptr->size, (uint32)ptr->type);
      if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
        memory_base = (uint32)ptr->base;
        memory_size = (uint32)ptr->size;
      }
    }
  } else {
    panic("Memory init magic unknown 0x%p\n", magic);
  }

  LOG_DEBUG("ARDS count %d\n", count);
  LOG_DEBUG("Memory base 0x%p\n", memory_base);
  LOG_DEBUG("Memory size 0x%p\n", memory_size);
  time_t time();
  assert(memory_base == MEMORY_BASE);
  assert((memory_size & 0xfff) == 0); // 按页对齐(划分页)

  free_pages = IDX(memory_size);
  total_pages = free_pages + IDX(MEMORY_BASE);

  LOG_DEBUG("Total pages %d\n", total_pages);
  LOG_DEBUG("Free pages %d\n", free_pages);

  if (memory_size < KERNEL_MEMORY_SIZE) {
    panic("System memory is %dM too small, at least %dM needed\n",
          memory_size / MEMORY_BASE, KERNEL_MEMORY_SIZE / MEMORY_BASE);
  }
}

static uint32 start_page = 0;   // 可分配物理内存起始地址
static uint8 *memory_map;       // 物理内存数组
static uint32 memory_map_pages; // 物理内存数组占用的页数

void memory_map_init() {
  // 初始化物理内存数组
  memory_map = (uint8 *)memory_base; // 物理页使用一字节表示被引用数量
  // 物理内存数组占用的页数(向上取整)
  memory_map_pages = div_round_up(total_pages, PAGE_SIZE);
  LOG_DEBUG("Memory map page count %d\n", memory_map_pages);

  free_pages -= memory_map_pages;

  // 清空物理内存数组
  memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

  // 有效起始页面从前1M内存和物理内存数组所占用的页后开始
  start_page = IDX(MEMORY_BASE) + memory_map_pages;

  for (size_t i = 0; i < start_page; ++i) {
    memory_map[i] = 1; // 表示物理数组所使用的页已经被占用
  }

  uint32 len = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8;
  bitmap_init(&kernel_map, (uint8 *)KERNEL_MAP_BITS, len, IDX(MEMORY_BASE));
  bitmap_scan(&kernel_map, memory_map_pages);
}

static uint32 get_page() {
  for (size_t i = start_page; i < total_pages; ++i) {
    if (!memory_map[i]) {
      memory_map[i] = 1;
      free_pages--;
      assert(free_pages >= 0);
      uint32 page = ((uint32)i) << 12; // 所分配页的起始地址
      LOG_DEBUG("GET page 0x%p\n", page);
      return page;
    }
  }
  panic("OOM");
}

static void put_page(uint32 addr) {
  ASSERT_PAGE(addr); // addr必须是某页的起始地址
  uint32 idx = IDX(addr);

  assert(idx >= start_page && idx < total_pages);
  assert(memory_map[idx] >= 1); // 至少有一个引用计数

  memory_map[idx]--;
  if (memory_map[idx] == 0) {
    free_pages++;
  }

  assert(free_pages > 0 && free_pages < total_pages);
  LOG_DEBUG("PUT page 0x%p\n", addr);
}

// 缺页访问的位置在cr2寄存器中
uint32 get_cr2() { asm volatile("movl %cr2, %eax\n"); }

uint32 get_cr3() { asm volatile("movl %cr3, %eax\n"); }

// pde：页目录地址
void set_cr3(uint32 pde) {
  ASSERT_PAGE(pde);
  asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

// 设置cr0，启动分页
static void enable_page() {
  asm volatile("movl %cr0, %eax\n"
               "orl $0x80000000, %eax\n"
               "movl %eax, %cr0");
}

// 初始化页表项
static void entry_init(page_entry_t *entry, uint32 index) {
  *(uint32 *)entry = 0; // 全部置零
  entry->present = 1;
  entry->write = 1;
  entry->user = 1;
  entry->index = index;
}

void mapping_init() {
  // 初始化页目录
  page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
  memset(pde, 0, PAGE_SIZE);

  for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); ++didx) {
    page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
    memset(pte, 0, PAGE_SIZE);
    // 映射页目录表表项
    page_entry_t *dentry = &pde[didx];
    entry_init(dentry, IDX((uint32)pte));
    dentry->user = 0; // 只能被内核访问

    // 映射页表表项
    idx_t index = 0;
    for (size_t tidx = 0; tidx < 1024; ++tidx, ++index) {
      if (index == 0) {
        continue;
      }
      page_entry_t *tentry = &pte[tidx];
      entry_init(tentry, index);
      tentry->user = 0;
      memory_map[index] = 1;
    }
  }

  // 最后一页指向页目录自己，便于修改
  page_entry_t *entry = &pde[1023];
  entry_init(entry, IDX(KERNEL_PAGE_DIR));

  set_cr3((uint32)pde);
  enable_page();
}

static page_entry_t *get_pde() { return (page_entry_t *)(0xfffff000); }

// 获取虚拟地址对应的页表
static page_entry_t *get_pte(uint32 vaddr, bool create) {
  page_entry_t *pde = get_pde();
  uint32 idx = DIDX(vaddr);
  page_entry_t *entry = &pde[idx];

  assert(create || (!create && entry->present));

  page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

  if (!entry->present) {
    LOG_DEBUG("Get and create page table entry for 0x%p\n", vaddr);
    uint32 page = get_page();
    memset(table, 0, PAGE_SIZE);
  }

  return table;
}

void flush_tlb(uint32 vaddr) {
  asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

static uint32 scan_page(bitmap_t *map, uint32 count) {
  assert(count > 0);

  int32 index = bitmap_scan(map, count);
  if (index == EOF) {
    panic("Scan page faile!!!\n");
  }

  uint32 addr = PAGE(index);
  LOG_DEBUG("Scan page 0x%p count %d\n", addr, count);
  return addr;
}

static void reset_page(bitmap_t *map, uint32 addr, uint32 count) {
  ASSERT_PAGE(addr);
  assert(count > 0);
  uint32 index = IDX(addr);

  for (size_t i = 0; i < count; ++i) {
    assert(bitmap_test(map, index + i));
    bitmap_set(map, index + i, false);
  }
}

uint32 alloc_kpage(uint32 count) {
  assert(count > 0);
  uint32 vaddr = scan_page(&kernel_map, count);
  LOG_DEBUG("ALLOC kernel pages 0x%p count %d\n", vaddr, count);
  return vaddr;
}

void free_kpage(uint32 vaddr, uint32 count) {
  ASSERT_PAGE(vaddr);
  assert(count > 0);
  reset_page(&kernel_map, vaddr, count);
  LOG_DEBUG("FREE kernel pages 0x%p count %d\n", vaddr, count);
}

void link_page(uint32 vaddr) {
  ASSERT_PAGE(vaddr);

  page_entry_t *pte = get_pte(vaddr, true);
  page_entry_t *entry = &pte[TIDX(vaddr)];

  task_t *task = running_task();
  bitmap_t *map = task->vmap;
  uint32 index = IDX(vaddr);

  if (entry->present) {
    assert(bitmap_test(map, index));
  }

  assert(!bitmap_test(map, index));
  bitmap_set(map, index, true);

  uint32 paddr = get_page();
  entry_init(entry, IDX(paddr));
  flush_tlb(vaddr);

  LOG_DEBUG("LINK from 0x%p to 0x%p", vaddr, paddr);
}

void unlink_page(uint32 vaddr) {
  ASSERT_PAGE(vaddr);

  page_entry_t *pte = get_pte(vaddr, true);
  page_entry_t *entry = &pte[TIDX(vaddr)];

  task_t *task = running_task();
  bitmap_t *map = task->vmap;
  uint32 index = IDX(vaddr);

  if (!entry->present) {
    assert(!bitmap_test(map, index));
    return;
  }

  assert(entry->present && bitmap_test(map, index));

  entry->present = false;
  bitmap_set(map, index, false);

  uint32 paddr = PAGE(entry->index);
  LOG_DEBUG("UNLINK from 0x%p to 0x%p", vaddr, paddr);
  put_page(paddr);

  flush_tlb(vaddr);
}

static uint32 copy_page(void *page) {
  uint32 paddr = get_page();

  page_entry_t *entry = get_pte(0, false);
  entry_init(entry, IDX(paddr));
  memcpy((void *)0, (void *)page, PAGE_SIZE);

  entry->present = false;
  return paddr;
}

page_entry_t *copy_pde() {
  task_t *task = running_task();
  page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
  memcpy(pde, (void *)task->pde, PAGE_SIZE);

  page_entry_t *entry = &pde[1023];
  entry_init(entry, IDX(pde));

  page_entry_t *dentry;

  for (size_t didx = (sizeof(KERNEL_PAGE_TABLE) / 4); didx < 1023; ++didx) {
    dentry = &pde[didx];
    if (!dentry->present) {
      continue;
    }

    page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

    for (size_t tidx = 0; tidx < 1024; ++tidx) {
      entry = &pte[tidx];
      if (!entry->present) {
        continue;
      }

      assert(memory_map[entry->index] > 0);
      // 置为只读，写时会发生缺页异常
      entry->write = false;
      memory_map[entry->index]++;
    }
    // 复制页表
    uint32 paddr = copy_page(pte);
    dentry->index = IDX(paddr);
  }

  set_cr3(task->pde);
  return pde;
}

void free_pde() {
  task_t *task = running_task();
  assert(task->uid != KERNEL_USER);

  page_entry_t *pde = get_pde();

  for (size_t didx = 2; didx < 1023; ++didx) {
    page_entry_t *dentry = &pde[didx];
    if (!dentry->present) {
      continue;
    }

    page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

    for (size_t tidx = 0; tidx < 1024; ++tidx) {
      page_entry_t *entry = &pte[tidx];
      if (!entry->present) {
        continue;
      }

      assert(memory_map[entry->index] > 0);
      put_page(PAGE(entry->index));
    }
    put_page(PAGE(dentry->index));
  }

  free_kpage(task->pde, 1);
}

int32 sys_brk(void *addr) {
  LOG_DEBUG("task brk 0x%p\n", addr);

  uint32 brk = (uint32)addr;
  ASSERT_PAGE(brk);

  task_t *task = running_task();
  assert(task->uid != KERNEL_USER);
  assert(KERNEL_MEMORY_SIZE < brk && brk < USER_STACK_BOTTOM);

  uint32 old_brk = task->brk;

  if (old_brk < brk) {
    for (uint32 page = brk; brk < old_brk; brk += PAGE_SIZE) {
      unlink_page(page);
    }
  } else if (IDX((brk - old_brk)) > free_pages) {
    return -1;
  }

  task->brk = brk;
  return 0;
}

typedef struct page_error_code_t {
  uint8 present : 1;
  uint8 write : 1;
  uint8 user : 1;
  uint8 reserved0 : 1;
  uint8 fetch : 1;
  uint8 protection : 1;
  uint8 shadow : 1;
  uint16 reserved1 : 8;
  uint8 sgx : 1;
  uint16 reserved2;
} _packed page_error_code_t;

void page_fault(uint32 vector, uint32 edi, uint32 esi, uint32 ebp, uint32 esp,
                uint32 ebx, uint32 edx, uint32 ecx, uint32 eax, uint32 gs,
                uint32 fs, uint32 es, uint32 ds, uint32 vector0, uint32 error,
                uint32 eip, uint32 cs, uint32 eflags) {
  assert(vector == 0xe);
  uint32 vaddr = get_cr2();
  LOG_DEBUG("fault address 0x%p\n", vaddr);

  page_error_code_t *code = (page_error_code_t *)&error;
  task_t *task = running_task();

  assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr < USER_STACK_TOP);

  if (code->present) {
    assert(code->write);

    page_entry_t *pte = get_pte(vaddr, false);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    assert(entry->present);
    assert(memory_map[entry->index] > 0);
    if (memory_map[entry->index] == 1) {
      entry->write = 1;
      LOG_DEBUG("WRITE page for 0x%p\n", vaddr);
    } else {
      void *page = (void *)PAGE(IDX(vaddr));
      uint32 paddr = copy_page(page);
      memory_map[entry->index]--;
      entry_init(entry, IDX(paddr));
      flush_tlb(vaddr);
      LOG_DEBUG("COP page for 0x%p\n", vaddr);
    }
    return;
  }

  if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM)) {
    uint32 page = PAGE(IDX(vaddr));
    link_page(page);
    return;
  }
  panic("page fault!!!\n");
}