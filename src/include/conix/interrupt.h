#ifndef CONIX_INTERRUPT_H
#define CONIX_INTERRUPT_H

#include "types.h"

#define IDT_SIZE 256

#define IRQ_CLOCK 0      // ʱ��
#define IRQ_KEYBOARD 1   // ����
#define IRQ_CASCADE 2    // 8259 ��Ƭ������
#define IRQ_SERIAL_2 3   // ���� 2
#define IRQ_SERIAL_1 4   // ���� 1
#define IRQ_PARALLEL_2 5 // ���� 2
#define IRQ_SB16 5       // SB16 ����
#define IRQ_FLOPPY 6     // ���̿�����
#define IRQ_PARALLEL_1 7 // ���� 1
#define IRQ_RTC 8        // ʵʱʱ��
#define IRQ_REDIRECT 9   // �ض��� IRQ2
#define IRQ_NIC 11       // ����
#define IRQ_MOUSE 12     // ���
#define IRQ_MATH 13      // Э������ x87
#define IRQ_HARDDISK 14  // ATA Ӳ�̵�һͨ��
#define IRQ_HARDDISK2 15 // ATA Ӳ�̵ڶ�ͨ��

#define IRQ_MASTER_NR 0x20 // ��Ƭ��ʼ������(���ж�)
#define IRQ_SLAVE_NR 0x28  // ��Ƭ��ʼ������

typedef struct gate_t {
  uint16 offset0;  // ����ƫ��0-15λ
  uint16 selector; // �����ѡ����
  uint8 reserved;
  uint8 type : 4;    // ������/�ж���/������
  uint8 segment : 1; // ��ӦGDT��0�� ��ʾϵͳ��
  uint8 DPL : 2;     // ʹ��intָ����ʵ����Ȩ��
  uint8 present : 1; // �Ƿ���Ч
  uint16 offset1;    // ����ƫ��16-31λ
} _packed gate_t;

typedef void *handler_t;

void send_eoi(int vec);

void set_interrupt_handler(uint32 irq, handler_t handler);
void set_interrupt_mask(uint32 irq, bool enable);

bool interrupt_disable();
bool get_interrupt_state();
void set_interrupt_state(bool state);

#endif