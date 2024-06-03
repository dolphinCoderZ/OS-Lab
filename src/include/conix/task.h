#ifndef CONIX_TASK_H
#define CONIX_TASK_H

#include "fs.h"
#include "list.h"
#include "types.h"

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

#define TASK_FILE_NR 16

// 定义函数指针类型
typedef void (*target_t)();

typedef enum task_state_t {
  TASK_INIT,
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
  TASK_SLEEPING,
  TASK_WAITING,
  TASK_DIED,
} task_state_t;

// PCB
typedef struct task_t {
  uint32 *stack;    // 内核栈
  list_node_t node; // 任务阻塞节点
  task_state_t state;
  uint32 priority;
  uint32 ticks;   // 剩余时间片
  uint32 jiffies; // 上次执行时全局时间片
  char name[TASK_NAME_LEN];
  uint32 uid;
  uint32 gid;
  pid_t pid;
  pid_t ppid;
  int status;
  pid_t waitpid; // 进程等待回收的pid
  char *pwd;
  uint32 pde;
  struct bitmap_t *vmap; // 进程虚拟内存位图
  uint32 brk;            // 进程堆内存的最高地址
  struct inode_t *ipwd;
  struct inode_t *iroot;
  uint16 umask;
  struct file_t *files[TASK_FILE_NR];
  uint32 magic; // 内核魔数，用于检测栈溢出
} task_t;

// 进程切换时需要保存的寄存器
typedef struct task_frame_t {
  uint32 edi;
  uint32 esi;
  uint32 ebx;
  uint32 ebp;
  void (*eip)(void); // 跳转目标函数位置
} task_frame_t;

// 中断帧
typedef struct intr_frame_t {
  uint32 vector;

  uint32 edi;
  uint32 esi;
  uint32 ebp;
  // 虽然 pushad 把 esp 也压入，但 esp 是不断变化的，所以会被 popad 忽略
  uint32 esp_dummy;
  uint32 ebx;
  uint32 edx;
  uint32 ecx;
  uint32 eax;
  uint32 gs;
  uint32 fs;
  uint32 es;
  uint32 ds;
  uint32 vector0;
  uint32 error;
  uint32 eip;
  uint32 cs;
  uint32 eflags;
  uint32 esp;
  uint32 ss;
} intr_frame_t;

task_t *running_task();
void schedule();

void task_exit(int status);
void task_yield();
void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);

void task_sleep(uint32 ms);
void task_wakeup();

void task_to_user_mode(target_t target);

pid_t task_fork();

pid_t sys_getpid();
pid_t sys_getppid();
pid_t task_waitpid(pid_t pid, int *status);

fd_t task_get_fd(task_t *task);
void task_put_fd(task_t *task, fd_t fd);

#endif