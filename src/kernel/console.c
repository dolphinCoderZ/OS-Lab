#include "../include/conix/console.h"
#include "../include/conix/debug.h"
#include "../include/conix/device.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/io.h"
#include "../include/conix/string.h"

#define CRT_ADDR_REG 0x3D4 // CRT(6845)索引寄存器
#define CRT_DATA_REG 0x3D5 // CRT(6845)数据寄存器

#define CRT_START_ADDR_H 0xC // 显示内存起始位置 - 高位
#define CRT_START_ADDR_L 0xD // 显示内存起始位置 - 低位
#define CRT_CURSOR_H 0xE     // 光标位置 - 高位
#define CRT_CURSOR_L 0xF     // 光标位置 - 低位

#define MEM_BASE 0xB8000              // 显卡内存起始位置
#define MEM_SIZE 0x4000               // 显卡内存大小
#define MEM_END (MEM_BASE + MEM_SIZE) // 显卡内存结束位置
#define WIDTH 80                      // 屏幕文本列数
#define HEIGHT 25                     // 屏幕文本行数
#define ROW_SIZE (WIDTH * 2)          // 每行字节数
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // 屏幕字节数

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

static uint32 screen; // 当前显示器开始的内存位置
static uint32 pos;    // 当前光标的内存位置
static uint32 x, y;

static uint8 attr = 7; // 字符样式
static uint16 erase = 0x0720;

// 获取显示器在内存中的开始位置
static void get_screen() {
  outb(CRT_ADDR_REG, CRT_START_ADDR_H); // 开始位置高地址
  screen = inb(CRT_DATA_REG) << 8;      // 开始位置高八位
  outb(CRT_ADDR_REG, CRT_START_ADDR_L); // 开始位置低地址
  screen |= inb(CRT_DATA_REG);

  screen <<= 1; // 两个字节等于一个字符
  screen += MEM_BASE;
}

// 设置显示器在内存中的开始位置
static void set_screen() {
  outb(CRT_ADDR_REG, CRT_START_ADDR_H);
  outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);
  outb(CRT_ADDR_REG, CRT_START_ADDR_L);
  outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

// 获取当前光标位置
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
  // 最后一行全部设置为空格
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

      *((char *)pos) = ch; // 设置字符ASCII
      pos++;
      *((char *)pos) = attr; // 设置字符属性
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
