#include "../include/conix/assert.h"
#include "../include/conix/console.h"
#include "../include/conix/debug.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/memory.h"
#include "../include/conix/syscall.h"
#include "../include/conix/task.h"

#define LOG_DEBUG(fmt, args...) DEBUGK(fmt, ##args)

#define SYSCALL_SIZE 256

handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(uint32 nr) {
  if (nr >= SYSCALL_SIZE) {
    panic("syscall nr error\n");
  }
}

static void sys_default() { panic("syscall not implemented!!!\n"); }

task_t *task = NULL;
static uint32 sys_test() {
  if (!task) {
    task = running_task();
    task_block(task, NULL, TASK_BLOCKED);
  } else {
    task_unblock(task);
    task = NULL;
  }
  return 255;
}

extern time_t sys_time();
extern mode_t sys_umask();
extern int sys_mkdir();
extern int sys_rmdir();
extern int sys_link();
extern int sys_unlink();
extern fd_t sys_open();
extern fd_t sys_creat();
extern void sys_close();
extern int32 sys_read();
extern int32 sys_write();
extern int32 sys_lseek();
extern int sys_chdir();
extern int sys_chroot();
extern char *sys_getcwd();

void syscall_init() {
  for (size_t i = 0; i < SYSCALL_SIZE; ++i) {
    syscall_table[i] = sys_default;
  }

  syscall_table[SYS_NR_TEST] = sys_test;
  syscall_table[SYS_NR_SLEEP] = task_sleep;
  syscall_table[SYS_NR_YIELD] = task_yield;

  syscall_table[SYS_NR_EXIT] = task_exit;
  syscall_table[SYS_NR_FORK] = task_fork;
  syscall_table[SYS_NR_GETPID] = sys_getpid;
  syscall_table[SYS_NR_GETPPID] = sys_getppid;
  syscall_table[SYS_NR_WAITPID] = task_waitpid;

  syscall_table[SYS_NR_BRK] = sys_brk;
  syscall_table[SYS_NR_TIME] = sys_time;

  syscall_table[SYS_NR_OPEN] = sys_open;
  syscall_table[SYS_NR_CREAT] = sys_creat;
  syscall_table[SYS_NR_CLOSE] = sys_close;
  syscall_table[SYS_NR_READ] = sys_read;
  syscall_table[SYS_NR_WRITE] = sys_write;
  syscall_table[SYS_NR_LSEEK] = sys_lseek;

  syscall_table[SYS_NR_MKDIR] = sys_mkdir;
  syscall_table[SYS_NR_RMDIR] = sys_rmdir;
  syscall_table[SYS_NR_UMASK] = sys_umask;
  syscall_table[SYS_NR_LINK] = sys_link;
  syscall_table[SYS_NR_UNLINK] = sys_unlink;

  syscall_table[SYS_NR_CHDIR] = sys_chdir;
  syscall_table[SYS_NR_CHROOT] = sys_chroot;
  syscall_table[SYS_NR_GETCWD] = sys_getcwd;
}
