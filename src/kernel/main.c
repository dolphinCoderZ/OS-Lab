#include "../include/conix/types.h"

extern void interrupt_init();
extern void clock_init();
extern void time_init();
extern void ide_init();
extern void rtc_init();
extern void keyboard_init();
extern void memory_map_init();
extern void task_init();
extern void set_interrupt_state(bool state);
extern void mapping_init();
extern void syscall_init();
extern void tss_init();
extern void arena_init();
extern void buffer_init();
extern void hang();

void kernel_init() {
  tss_init();
  memory_map_init();
  mapping_init();
  arena_init();
  interrupt_init();
  clock_init();
  time_init();
  ide_init();
  keyboard_init();
  buffer_init();

  // time_init();
  // rtc_init();

  task_init();
  syscall_init();

  set_interrupt_state(true);
}