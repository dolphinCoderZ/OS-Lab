#ifndef CONIX_INTERRUPT_H
#define CONIX_INTERRUPT_H

#include "types.h"

#define IDT_SIZE 256

#define IRQ_CLOCK 0      // 时钟
#define IRQ_KEYBOARD 1   // 键盘
#define IRQ_CASCADE 2    // 8259 从片控制器
#define IRQ_SERIAL_2 3   // 串口 2
#define IRQ_SERIAL_1 4   // 串口 1
#define IRQ_PARALLEL_2 5 // 并口 2
#define IRQ_SB16 5       // SB16 声卡
#define IRQ_FLOPPY 6     // 软盘控制器
#define IRQ_PARALLEL_1 7 // 并口 1
#define IRQ_RTC 8        // 实时时钟
#define IRQ_REDIRECT 9   // 重定向 IRQ2
#define IRQ_NIC 11       // 网卡
#define IRQ_MOUSE 12     // 鼠标
#define IRQ_MATH 13      // 协处理器 x87
#define IRQ_HARDDISK 14  // ATA 硬盘第一通道
#define IRQ_HARDDISK2 15 // ATA 硬盘第二通道

#define IRQ_MASTER_NR 0x20 // 主片起始向量号(外中断)
#define IRQ_SLAVE_NR 0x28  // 从片起始向量号

typedef struct gate_t {
  uint16 offset0;  // 段内偏移0-15位
  uint16 selector; // 代码段选择子
  uint8 reserved;
  uint8 type : 4;    // 任务门/中断门/陷阱门
  uint8 segment : 1; // 对应GDT中0， 表示系统段
  uint8 DPL : 2;     // 使用int指令访问的最低权限
  uint8 present : 1; // 是否有效
  uint16 offset1;    // 段内偏移16-31位
} _packed gate_t;

typedef void *handler_t;

void send_eoi(int vec);

void set_interrupt_handler(uint32 irq, handler_t handler);
void set_interrupt_mask(uint32 irq, bool enable);

bool interrupt_disable();
bool get_interrupt_state();
void set_interrupt_state(bool state);

#endif