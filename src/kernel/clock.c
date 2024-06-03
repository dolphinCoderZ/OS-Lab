#include "../include/conix/assert.h"
#include "../include/conix/conix.h"
#include "../include/conix/debug.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/io.h"
#include "../include/conix/task.h"

#define PIT_CHAN0_REG 0X40
#define PIT_CHAN2_REG 0X42
#define PIT_CTRL_REG 0X43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)

#define SPEAKER_REG 0x61
#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)
#define BEEP_MS 100

uint32 volatile jiffies = 0;
uint32 jiffy = JIFFY;

uint32 volatile beeping = 0;

void start_beep() {
  if (!beeping) {
    outb(SPEAKER_REG, inb(SPEAKER_REG) | 0b11);
  }
  beeping = jiffies + 5;
}

void stop_beep() {
  if (beeping && jiffies > beeping) {
    outb(SPEAKER_REG, inb(SPEAKER_REG) & 0xfc);
    beeping = 0;
  }
}

void clock_handler(int vec) {
  assert(vec == 0x20);
  send_eoi(vec);

  jiffies++;

  task_wakeup();

  task_t *task = running_task();
  assert(task->magic == CONIX_MAGIC);

  task->jiffies = jiffies;
  task->ticks--;

  if (!task->ticks) {
    task->ticks = task->priority;
    schedule();
  }
}

extern uint32 startup_time;
time_t sys_time() { return startup_time + (jiffies * JIFFY) / 1000; }

void pit_init() {
  // ÅäÖÃ¼ÆÊýÆ÷ 0 Ê±ÖÓ
  outb(PIT_CTRL_REG, 0b00110100);
  outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
  outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);

  // ÅäÖÃ¼ÆÊýÆ÷ 2 ·äÃùÆ÷
  outb(PIT_CTRL_REG, 0b10110110);
  outb(PIT_CHAN2_REG, (uint8)BEEP_COUNTER);
  outb(PIT_CHAN2_REG, (uint8)(BEEP_COUNTER >> 8));
}

void clock_init() {
  pit_init();
  set_interrupt_handler(IRQ_CLOCK, clock_handler);
  set_interrupt_mask(IRQ_CLOCK, true);
}
