#include "../include/conix/arena.h"
#include "../include/conix/debug.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/mutex.h"
#include "../include/conix/printk.h"
#include "../include/conix/stdio.h"
#include "../include/conix/syscall.h"

#define LOG_DEBUG(fmt, args...) DEBUGK(fmt, ##args)

void idle_thread() {
  set_interrupt_state(true);
  uint32 counter = 0;

  while (1) {
    LOG_DEBUG("idle task... %d\n", counter++);
    asm volatile("sti\n"
                 "hlt\n"); // 暂停CPU，进入暂停状态，等待外中断

    yield();
  }
}

static void user_init_thread() {
  uint32 count = 0;

  char ch;
  while (1) {
    // printf("init thread %d %d %d\n", getpid(), getppid(), count++);
    // sleep(1000);
    BMB;
  }
}

extern uint32 keyboard_read(char *buf, uint32 count);
char ch;
extern void task_to_user_mode(target_t target);

void init_thread() {
  set_interrupt_state(true);
  uint32 counter = 0;
  while (1) {
    LOG_DEBUG("test thread %d\n", counter++);
    sleep(2000);

    // char tmp[100];
    // task_to_user_mode(user_init_thread);
  }
}

void test_thread() {
  set_interrupt_state(true);
  uint32 counter = 0;
  while (1) {
    sleep(2000);
  }
}
