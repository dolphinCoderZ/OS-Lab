#include "../include/conix/console.h"
#include "../include/conix/debug.h"
#include "../include/conix/device.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/io.h"
#include "../include/conix/string.h"

#define CRT_ADDR_REG 0x3D4 // CRT(6845)�����Ĵ���
#define CRT_DATA_REG 0x3D5 // CRT(6845)���ݼĴ���

#define CRT_START_ADDR_H 0xC // ��ʾ�ڴ���ʼλ�� - ��λ
#define CRT_START_ADDR_L 0xD // ��ʾ�ڴ���ʼλ�� - ��λ
#define CRT_CURSOR_H 0xE     // ���λ�� - ��λ
#define CRT_CURSOR_L 0xF     // ���λ�� - ��λ

#define MEM_BASE 0xB8000              // �Կ��ڴ���ʼλ��
#define MEM_SIZE 0x4000               // �Կ��ڴ��С
#define MEM_END (MEM_BASE + MEM_SIZE) // �Կ��ڴ����λ��
#define WIDTH 80                      // ��Ļ�ı�����
#define HEIGHT 25                     // ��Ļ�ı�����
#define ROW_SIZE (WIDTH * 2)          // ÿ���ֽ���
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // ��Ļ�ֽ���

#define ASCII_NUL 0x00
#define ASCII_ENQ 0x05
#define ASCII_ESC 0x1B // ESC
#define ASCII_BEL 0x07 // \a
#define ASCII_BS 0x08  // \b
#define ASCII_HT 0x09  // \t
#define ASCII_LF 0x0A  // \n
#define ASCII_VT 0x0B  // \v
#define ASCII_FF 0x0C  // \f
#define ASCII_CR 0x0D  // \r
#define ASCII_DEL 0x7F

static uint32 screen; // ��ǰ��ʾ����ʼ���ڴ�λ��
static uint32 pos;    // ��ǰ�����ڴ�λ��
static uint32 x, y;

static uint8 attr = 7; // �ַ���ʽ
static uint16 erase = 0x0720;

// ��ȡ��ʾ�����ڴ��еĿ�ʼλ��
static void get_screen() {
  outb(CRT_ADDR_REG, CRT_START_ADDR_H); // ��ʼλ�øߵ�ַ
  screen = inb(CRT_DATA_REG) << 8;      // ��ʼλ�ø߰�λ
  outb(CRT_ADDR_REG, CRT_START_ADDR_L); // ��ʼλ�õ͵�ַ
  screen |= inb(CRT_DATA_REG);

  screen <<= 1; // �����ֽڵ���һ���ַ�
  screen += MEM_BASE;
}

// ������ʾ�����ڴ��еĿ�ʼλ��
static void set_screen() {
  outb(CRT_ADDR_REG, CRT_START_ADDR_H);
  outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);
  outb(CRT_ADDR_REG, CRT_START_ADDR_L);
  outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

// ��ȡ��ǰ���λ��
static void get_cursor() {
  outb(CRT_ADDR_REG, CRT_CURSOR_H);
  pos = inb(CRT_DATA_REG) << 8;
  outb(CRT_ADDR_REG, CRT_CURSOR_L);
  pos |= inb(CRT_DATA_REG);

  get_screen();
  pos <<= 1;
  pos += MEM_BASE;

  uint32 delta = (pos - screen) >> 1;
  x = delta % WIDTH;
  y = delta % HEIGHT;
}

static void set_cursor() {
  outb(CRT_ADDR_REG, CRT_CURSOR_H);
  outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 9) & 0xff);
  outb(CRT_ADDR_REG, CRT_CURSOR_L);
  outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 1) & 0xff);
}

void console_clear() {
  screen = MEM_BASE;
  pos = MEM_BASE;
  x = y = 0;
  set_cursor();
  set_screen();

  uint16 *ptr = (uint16 *)MEM_BASE;
  while (ptr < (uint16 *)MEM_END) {
    *ptr++ = erase;
  }
}

static void scroll_up() {
  if (screen + SCR_SIZE + ROW_SIZE >= MEM_END) {
    memcpy((void *)MEM_BASE, (void *)screen, SCR_SIZE);
    pos -= (screen - MEM_BASE);
    screen = MEM_BASE;
  }

  uint32 *ptr = (uint32 *)(screen + SCR_SIZE);
  // ���һ��ȫ������Ϊ�ո�
  for (size_t i = 0; i < WIDTH; ++i) {
    *ptr++ = erase;
  }

  screen += ROW_SIZE;
  pos += ROW_SIZE;
  set_screen();
}

static void command_lf() {
  if (y + 1 < HEIGHT) {
    y++;
    pos += ROW_SIZE;
    return;
  }
  scroll_up();
}

static void command_cr() {
  pos -= (x << 1);
  x = 0;
}

static void command_bs() {
  if (x) {
    x--;
    pos -= 2;
    *(uint16 *)pos = erase;
  }
}

static void command_del() { *(uint16 *)pos = erase; }

extern void start_beep();
int32 console_write(void *dev, char *buf, uint32 count) {
  bool intr = interrupt_disable();

  char ch;
  int32 nr = 0;
  while (nr++ < count) {
    ch = *buf++;
    switch (ch) {
    case ASCII_NUL:
      break;
    case ASCII_BEL:
      start_beep();
      break;
    case ASCII_BS:
      command_bs();
      break;
    case ASCII_HT:
      break;
    case ASCII_LF:
      command_lf();
      command_cr();
      break;
    case ASCII_VT:
      break;
    case ASCII_FF:
      command_lf();
      break;
    case ASCII_CR:
      command_cr();
      break;
    case ASCII_DEL:
      command_del();
      break;
    default:
      if (x >= WIDTH) {
        x -= WIDTH;
        pos -= ROW_SIZE;
        command_lf();
      }

      *((char *)pos) = ch; // �����ַ�ASCII
      pos++;
      *((char *)pos) = attr; // �����ַ�����
      pos++;

      x++;
      break;
    }
  }

  set_cursor();
  set_interrupt_state(intr);
}

void console_init() {

  console_clear();

  device_install(DEV_CHAR, DEV_CONSOLE, NULL, "console", 0, NULL, NULL,
                 console_write);
}
