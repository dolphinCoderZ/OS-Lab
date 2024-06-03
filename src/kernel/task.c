#include "../include/conix/task.h"
#include "../include/conix/arena.h"
#include "../include/conix/assert.h"
#include "../include/conix/bitmap.h"
#include "../include/conix/conix.h"
#include "../include/conix/debug.h"
#include "../include/conix/global.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/list.h"
#include "../include/conix/memory.h"
#include "../include/conix/printk.h"
#include "../include/conix/string.h"
#include "../include/conix/syscall.h"

#define NR_TASKS 64

extern uint32 volatile jiffies;
extern uint32 jiffy;
extern bitmap_t kernel_map;
extern void task_switch(task_t *next);

extern tss_t tss;

static task_t *task_table[NR_TASKS];
static list_t block_list;
static list_t sleep_list;
static task_t *idle_task;

// 获得一个空闲任务
static task_t *get_free_task() {
  for (size_t i = 0; i < NR_TASKS; ++i) {
    if (task_table[i] == NULL) {
      task_t *task = (task_t *)alloc_kpage(1);
      memset(task, 0, PAGE_SIZE);
      task->pid = i;
      task_table[i] = task;
      return task;
    }
  }
  panic("No more task");
}

extern void interrupt_exit();
static void task_build_stack(task_t *task) {
  uint32 addr = (uint32)task + PAGE_SIZE;
  addr -= sizeof(intr_frame_t);
  intr_frame_t *iframe = (intr_frame_t *)addr;
  iframe->eax = 0;

  addr -= sizeof(task_frame_t);
  task_frame_t *frame = (task_frame_t *)addr;

  frame->ebp = 0xaa55aa55;
  frame->ebx = 0xaa55aa55;
  frame->edi = 0xaa55aa55;
  frame->esi = 0xaa55aa55;
  frame->eip = interrupt_exit;
  task->stack = (uint32 *)frame;
}

pid_t task_fork() {
  task_t *task = running_task();

  assert(task->node.next == NULL && task->node.prev == NULL &&
         task->state == TASK_RUNNING);

  // 拷贝内核栈和pcb
  task_t *child = get_free_task();
  pid_t pid = child->pid;
  memcpy(child, task, PAGE_SIZE);

  child->pid = pid;
  child->ppid = task->pid;
  child->ticks = child->priority;
  child->state = TASK_READY;

  // 拷贝用户进程虚拟内存位图
  child->vmap = kmalloc(sizeof(bitmap_t));
  memcpy(child->vmap, task->vmap, sizeof(bitmap_t));

  void *buf = (void *)alloc_kpage(1);
  memcpy(buf, task->vmap->bits, PAGE_SIZE);
  child->vmap->bits = buf;

  // 拷贝页目录
  child->pde = (uint32)copy_pde();

  // 拷贝pwd
  child->pwd = (char *)alloc_kpage(1);
  strncpy(child->pwd, task->pwd, PAGE_SIZE);
  // 工作目录引用+1
  task->ipwd->count++;
  task->iroot->count++;
  // 文件引用+1
  for (size_t i = 0; i < TASK_FILE_NR; ++i) {
    file_t *file = task->files[i];
    if (file) {
      file->count++;
    }
  }

  task_build_stack(child);

  return child->pid;
}

void task_exit(int status) {
  task_t *task = running_task();

  assert(task->node.next == NULL && task->node.prev == NULL &&
         task->state == TASK_RUNNING);

  task->state = TASK_DIED;
  task->status = status;

  // 释放页目录
  free_pde();
  // 释放虚拟位图
  free_kpage((uint32)task->vmap->bits, 1);
  kfree(task->vmap);

  free_kpage((uint32)task->pwd, 1);
  iput(task->ipwd);
  iput(task->iroot);

  for (size_t i = 0; i < TASK_FILE_NR; ++i) {
    file_t *file = task->files[i];
    if (file) {
      close(i);
    }
  }

  // 将当前进程的子进程ppid赋值未当前进程的ppid
  for (size_t i = 0; i < NR_TASKS; ++i) {
    task_t *child = task_table[i];
    if (!child) {
      continue;
    }
    if (child->ppid != task->pid) {
      continue;
    }
    child->ppid = task->ppid;
  }

  // 子进程退出，通知一下父进程，-1无差别回收子进程
  task_t *parent = task_table[task->ppid];
  if (parent->state == TASK_WAITING &&
      (parent->waitpid == -1 || parent->waitpid == task->pid)) {
    task_unblock(parent);
  }

  schedule();
}

pid_t task_waitpid(pid_t pid, int *status) {
  task_t *task = running_task();
  task_t *child = NULL;

  while (1) {
    bool has_child = false;
    for (size_t i = 2; i < NR_TASKS; ++i) {
      task_t *ptr = task_table[i];
      if (!ptr) {
        continue;
      }
      if (ptr->ppid != task->pid) {
        continue;
      }
      if (pid != ptr->pid && pid != -1) {
        continue;
      }

      if (ptr->state == TASK_DIED) {
        child = ptr;
        task_table[i] = NULL;
        goto rollback;
      }
      has_child = true;
    }

    if (has_child) {
      task->waitpid = pid;
      task_block(task, NULL, TASK_WAITING);
      continue;
    }
    break;
  }

  return -1;

rollback:
  *status = child->status;
  uint32 ret = child->pid;
  free_kpage((uint32)child, 1);
  return ret;
}

pid_t sys_getpid() { return running_task()->pid; }

pid_t sys_getppid() { return running_task()->ppid; }

fd_t task_get_fd(task_t *task) {
  fd_t i;
  for (i = 3; i < TASK_FILE_NR; ++i) {
    if (!task->files[i]) {
      break;
    }
  }
  if (i == TASK_FILE_NR) {
    panic("Exceed task max open files");
  }
  return i;
}

void task_put_fd(task_t *task, fd_t fd) {
  if (fd < 3) {
    return;
  }
  assert(fd < TASK_FILE_NR);
  task->files[fd] = NULL;
}

static task_t *task_search(task_state_t state) {
  assert(!get_interrupt_state()); // 不可中断

  task_t *task = NULL;
  task_t *current = running_task();

  for (size_t i = 0; i < NR_TASKS; ++i) {
    task_t *ptr = task_table[i];
    if (ptr == NULL || ptr->state != state || current == ptr) {
      continue;
    }

    if (task == NULL || task->ticks < ptr->ticks ||
        task->jiffies > ptr->jiffies) {
      task = ptr;
    }
  }

  if (task == NULL && state == TASK_READY) {
    task = idle_task;
  }

  return task;
}

task_t *running_task() {
  asm volatile("movl %esp, %eax\n"
               "andl $0xfffff000, %eax\n");
}

void task_activate(task_t *task) {
  assert(task->magic == CONIX_MAGIC);

  if (task->pde != get_cr3()) {
    set_cr3(task->pde);
  }
  if (task->uid != KERNEL_USER) {
    tss.esp0 = (uint32)task + PAGE_SIZE;
  }
}

extern void task_switch(task_t *next);
void schedule() {
  assert(!get_interrupt_state()); // 不可中断

  task_t *cur = running_task();
  task_t *next = task_search(TASK_READY);
  assert(next != NULL);
  assert(next->magic == CONIX_MAGIC);

  if (cur->state == TASK_RUNNING) {
    cur->state = TASK_READY;
  }

  next->state = TASK_RUNNING;
  if (next == cur) {
    return;
  }
  task_activate(next);
  task_switch(next);
}

static task_t *task_create(target_t target, const char *name, uint32 priority,
                           uint32 uid) {
  task_t *task = get_free_task();

  uint32 stack = (uint32)task + PAGE_SIZE;
  stack -= sizeof(task_frame_t);

  task_frame_t *frame = (task_frame_t *)stack;
  frame->ebx = 0x11111111;
  frame->esi = 0x22222222;
  frame->edi = 0x33333333;
  frame->ebp = 0x44444444;
  frame->eip = (void *)target;

  strcpy((char *)task->name, name);

  task->stack = (uint32 *)stack;
  task->priority = priority;
  task->ticks = task->priority;
  task->jiffies = 0;
  task->state = TASK_READY;
  task->uid = uid;
  task->gid = 0;
  task->vmap = &kernel_map;
  task->pde = KERNEL_PAGE_DIR;
  task->brk = KERNEL_MEMORY_SIZE;
  task->iroot = task->ipwd = get_root_inode();
  task->iroot->count += 2;

  task->pwd = (void *)alloc_kpage(1);
  strcpy(task->pwd, "/");

  task->umask = 0022;

  task->magic = CONIX_MAGIC;

  return task;
}

// 调用该函数的地方不能有任何局部变量
// 调用前栈顶需要准备足够的空间
void task_to_user_mode(target_t target) {
  task_t *task = running_task();

  // 创建用户进程虚拟内存位图
  task->vmap = kmalloc(sizeof(bitmap_t));
  void *buf = (void *)alloc_kpage(1);
  bitmap_init(task->vmap, buf, PAGE_SIZE, KERNEL_MEMORY_SIZE / PAGE_SIZE);

  // 创建用户进程页表
  task->pde = (uint32)copy_pde();
  set_cr3(task->pde);

  uint32 addr = (uint32)task + PAGE_SIZE;
  addr -= sizeof(intr_frame_t);
  intr_frame_t *iframe = (intr_frame_t *)(addr);

  iframe->vector = 0x20;
  iframe->edi = 1;
  iframe->esi = 2;
  iframe->ebp = 3;
  iframe->esp_dummy = 4;
  iframe->ebx = 5;
  iframe->edx = 6;
  iframe->ecx = 7;
  iframe->eax = 8;

  iframe->gs = 0;
  iframe->ds = USER_DATA_SELECTOR;
  iframe->es = USER_DATA_SELECTOR;
  iframe->fs = USER_DATA_SELECTOR;
  iframe->ss = USER_DATA_SELECTOR;
  iframe->cs = USER_CODE_SELECTOR;
  iframe->error = CONIX_MAGIC;

  iframe->eip = (uint32)target;
  iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
  iframe->esp = USER_STACK_TOP;

  asm volatile("movl %0, %%esp\n"
               "jmp interrupt_exit\n" ::"m"(iframe));
}

static void task_setup() {
  task_t *task = running_task();
  task->magic = CONIX_MAGIC;
  task->ticks = 1;

  memset(task_table, 0, sizeof(task_table));
}

void task_yield() { schedule(); }

void task_block(task_t *task, list_t *blist, task_state_t state) {
  assert(!get_interrupt_state());
  assert(task->node.next == NULL);
  assert(task->node.prev == NULL);
  if (blist == NULL) {
    blist = &block_list;
  }

  assert(state != TASK_READY && state != TASK_RUNNING);
  list_push(blist, &task->node);
  task->state = state;

  task_t *cur = running_task();
  if (cur == task) {
    schedule();
  }
}

void task_unblock(task_t *task) {
  assert(!get_interrupt_state());

  list_remove(&task->node);

  assert(task->node.next == NULL);
  assert(task->node.prev == NULL);

  task->state = TASK_READY;
}

void task_sleep(uint32 ms) {
  assert(!get_interrupt_state());
  uint32 ticks = ms / jiffy; // 需要睡眠的时间片
  ticks = ticks > 0 ? ticks : 1;

  // 唤醒时刻
  task_t *cur = running_task();
  cur->ticks = jiffies + ticks;

  list_t *list = &sleep_list;
  list_node_t *anchor = &list->tail;

  for (list_node_t *ptr = list->head.next; ptr != &list->tail;
       ptr = ptr->next) {
    task_t *task = element_entry(task_t, node, ptr);

    if (task->ticks > cur->ticks) {
      anchor = ptr;
      break;
    }
  }
  assert(cur->node.next == NULL);
  assert(cur->node.prev == NULL);
  list_insert_before(anchor, &cur->node);

  cur->state = TASK_SLEEPING;
  schedule();
}

void task_wakeup() {
  assert(!get_interrupt_state());

  list_t *list = &sleep_list;
  for (list_node_t *ptr = list->head.next; ptr != &list->tail;) {
    task_t *task = element_entry(task_t, node, ptr);

    if (task->ticks > jiffies) {
      break;
    }

    // 找到时间片小于全局时间片的任务并唤醒
    ptr = ptr->next;
    task->ticks = 0;
    task_unblock(task);
  }
}

extern void idle_thread();
extern void init_thread();
extern void test_thread();

void task_init() {
  list_init(&block_list);
  list_init(&sleep_list);

  task_setup();

  // idle_task = task_create(idle_thread, "idle", 5, KERNEL_USER);

  task_create(init_thread, "init", 5, NORMAL_USER);
  task_create(test_thread, "test", 5, NORMAL_USER);
  task_create(test_thread, "test", 5, NORMAL_USER);
}
