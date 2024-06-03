#include "../include/conix/interrupt.h"
#include "../include/conix/assert.h"
#include "../include/conix/debug.h"
#include "../include/conix/global.h"
#include "../include/conix/io.h"
#include "../include/conix/printk.h"
#include "../include/conix/stdlib.h"

#define LOG_DEBUG(fmt, args...) DEBUGK(fmt, ##args)
// #define LOG_DEBUG(fmt, args...)

#define ENTRY_SIZE 0x30

#define PIC_M_CTRL 0x20 // ��Ƭ�Ŀ��ƶ˿�
#define PIC_M_DATA 0x21 // ��Ƭ�����ݶ˿�
#define PIC_S_CTRL 0xa0 // ��Ƭ�Ŀ��ƶ˿�
#define PIC_S_DATA 0xa1 // ��Ƭ�����ݶ˿�
#define PIC_EOI 0x20    // ֪ͨ�жϿ������жϽ���

gate_t idt[IDT_SIZE];
pointer_t idt_ptr;

handler_t handler_table[IDT_SIZE];

// �쳣�����ж�
extern handler_t handler_entry_table[ENTRY_SIZE];
// ϵͳ����
extern void syscall_handler();
extern void page_fault();

static char *messages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

// ֪ͨ�жϿ��������жϴ������
void send_eoi(int vec) {
  if (vec >= 0x20 && vec < 0x28) {
    outb(PIC_M_CTRL, PIC_EOI);
  }
  if (vec >= 0x28 && vec < 0x30) {
    outb(PIC_M_CTRL, PIC_EOI);
    outb(PIC_S_CTRL, PIC_EOI);
  }
}

void set_interrupt_handler(uint32 irq, handler_t handler) {
  assert(irq >= 0 && irq < 16);
  handler_table[IRQ_MASTER_NR + irq] = handler;
}

void set_interrupt_mask(uint32 irq, bool enable) {
  assert(irq >= 0 && irq < 16);
  uint16 port;
  if (irq < 8) {
    port = PIC_M_DATA;
  } else {
    port = PIC_S_DATA;
    irq -= 8;
  }

  if (enable) {
    outb(port, inb(port) & ~(1 << irq));
  } else {
    outb(port, inb(port) | (1 << irq));
  }
}

bool interrupt_disable() {
  asm volatile("pushfl\n"
               "cli\n"
               "popl %eax\n"
               "shrl $9, %eax\n"
               "andl $1, %eax\n");
}

bool get_interrupt_state() {
  asm volatile("pushfl\n"
               "popl %eax\n"
               "shrl $9, %eax\n"
               "andl $1, %eax\n");
}

void set_interrupt_state(bool state) {
  if (state) {
    asm volatile("sti\n");
  } else {
    asm volatile("cli\n");
  }
}

void default_handler(int vec) {
  send_eoi(vec);
  DEBUGK("[%x] default interrupt called...\n", vec);
}

void exception_handler(int vec, uint32 edi, uint32 esi, uint32 ebp, uint32 esp,
                       uint32 ebx, uint32 edx, uint32 ecx, uint32 eax,
                       uint32 gs, uint32 fs, uint32 es, uint32 ds,
                       uint32 vector0, uint32 error, uint32 eip, uint32 cs,
                       uint32 eflags) {

  char *msg = NULL;
  if (vec < 22) {
    msg = messages[vec];
  } else {
    msg = messages[15];
  }

  printk("\nEXCEPTION : %s \n", messages[vec]);
  printk("   VECTOR : 0x%02X\n", vec);
  printk("    ERROR : 0x%08X\n", error);
  printk("   EFLAGS : 0x%08X\n", eflags);
  printk("       CS : 0x%02X\n", cs);
  printk("      EIP : 0x%08X\n", eip);
  printk("      ESP : 0x%08X\n", esp);

  hang();
}

// ��ʼ���жϿ�����
void pic_init() {
  outb(PIC_M_CTRL, 0b00010001);
  outb(PIC_M_DATA, 0x20);
  outb(PIC_M_DATA, 0b00000100);
  outb(PIC_M_DATA, 0b00000001);

  outb(PIC_S_CTRL, 0b00010001);
  outb(PIC_S_DATA, 0x28);
  outb(PIC_S_DATA, 2);
  outb(PIC_S_DATA, 0b00000001);

  outb(PIC_M_DATA, 0b11111111);
  outb(PIC_S_DATA, 0b11111111); // �ر������ж�
}

// ��ʼ���ж����������жϴ���������
void idt_init() {
  for (size_t i = 0; i < ENTRY_SIZE; ++i) {
    gate_t *gate = &idt[i];
    handler_t handler = handler_entry_table[i];

    gate->offset0 = (uint32)handler & 0xffff;
    gate->offset1 = ((uint32)handler >> 16) & 0xffff;
    gate->selector = 1 << 3;
    gate->reserved = 0;
    gate->type = 0b1110; // �ж���
    gate->segment = 0;   // ϵͳ��
    gate->DPL = 0;
    gate->present = 1;
  }

  for (size_t i = 0; i < 0x20; ++i) {
    handler_table[i] = exception_handler;
  }

  handler_table[0xe] = page_fault;

  for (size_t i = 0x20; i < ENTRY_SIZE; ++i) {
    handler_table[i] = default_handler;
  }

  // ��ʼ��ϵͳ����
  gate_t *gate = &idt[0x80];
  gate->offset0 = ((uint32)syscall_handler & 0xffff);
  gate->offset1 = ((uint32)syscall_handler >> 16) & 0xffff;
  gate->selector = 1 << 3;
  gate->reserved = 0;
  gate->type = 0b1110;
  gate->segment = 0;
  gate->DPL = 3; // �û�̬
  gate->present = 1;

  idt_ptr.base = (uint32)idt;
  idt_ptr.limit = sizeof(idt) - 1;

  asm volatile("lidt idt_ptr");
}

void interrupt_init() {
  pic_init();
  idt_init();
}
