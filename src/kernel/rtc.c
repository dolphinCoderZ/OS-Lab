#include "../include/conix/rtc.h"
#include "../include/conix/assert.h"
#include "../include/conix/debug.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/io.h"
#include "../include/conix/stdlib.h"
#include "../include/conix/time.h"

#define LOG_DEBUG(fmt, args...) DEBUGK(fmt, ##args)

#define CMOS_ADDR 0x70 // CMOS ��ַ�Ĵ���
#define CMOS_DATA 0x71 // CMOS ���ݼĴ���

#define CMOS_SECOND 0x01
#define CMOS_MINUTE 0x03
#define CMOS_HOUR 0x05

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

// �� cmos �Ĵ�����ֵ
uint8 cmos_read(uint8 addr) {
  outb(CMOS_ADDR, CMOS_NMI | addr);
  return inb(CMOS_DATA);
}

// д cmos �Ĵ�����ֵ
void cmos_write(uint8 addr, uint8 value) {
  outb(CMOS_ADDR, CMOS_NMI | addr);
  outb(CMOS_DATA, value);
}

static uint32 volatile count = 0;
extern void start_beep();
void rtc_handler(int vec) {

  assert(vec == 0x28); // ʵʱʱ���ж�������
  send_eoi(vec);

  // �� CMOS �Ĵ��� C�������� CMOS ���������ж�
  // cmos_read(CMOS_C);

  start_beep();

  // LOG_DEBUG("rtc handler %d...\n", count++);
}

void set_alarm(uint32 seconds) {
  tm time;
  time_read(&time);

  uint8 sec = seconds % 60;
  seconds /= 60;
  uint8 min = seconds % 10;
  seconds /= 60;
  uint32 hour = seconds;

  time.tm_sec += sec;
  if (time.tm_sec >= 60) {
    time.tm_sec %= 60;
    time.tm_min += 1;
  }

  time.tm_min += min;
  if (time.tm_min >= 60) {
    time.tm_min %= 60;
    time.tm_hour += 1;
  }

  time.tm_hour += hour;
  if (time.tm_hour >= 24) {
    time.tm_hour %= 24;
  }

  cmos_write(CMOS_HOUR, bin_to_bcd(time.tm_hour));
  cmos_write(CMOS_MINUTE, bin_to_bcd(time.tm_min));
  cmos_write(CMOS_SECOND, bin_to_bcd(time.tm_sec));

  cmos_write(CMOS_B, 0b00100010); // �������ж�
  cmos_read(CMOS_C);
}

void rtc_init() {
  // cmos_write(CMOS_B, 0b01000010); // �������ж�

  // set_alarm(1);

  // �����ж�Ƶ��
  // outb(CMOS_A, (inb(CMOS_A) & 0xf) | 0b1110);

  // �����жϴ�����
  set_interrupt_handler(IRQ_RTC, rtc_handler);
  // ���ն�������
  set_interrupt_mask(IRQ_RTC, true);
  set_interrupt_mask(IRQ_CASCADE, true);
}