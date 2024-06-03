#ifndef CONIX__MEMORY_H
#define CONIX__MEMORY_H

#include "bitmap.h"
#include "types.h"

#define PAGE_SIZE 0x1000
#define MEMORY_BASE 0x100000

// �ں˴�С16M
#define KERNEL_MEMORY_SIZE 0x1000000
// �ں˻����ַ
#define KERNEL_BUFFER_MEM 0x800000
#define KERNEL_BUFFER_SIZE 0x400000

// �ں�������̵�ַ
#define KERNEL_RAMDISK_MEM (KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE)
#define KERNEL_RAMDISK_SIZE 0x400000

// �û�ջ����ַ256M
#define USER_STACK_TOP 0x8000000
#define USER_STACK_SIZE 0x200000
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

// �ں�ҳĿ¼������ַ
#define KERNEL_PAGE_DIR 0x1000
// �ں�ҳ������
static uint32 KERNEL_PAGE_TABLE[] = {
    0x2000, // ��һҳ��ʼ��ַ
    0x3000, // �ڶ�ҳ��ʼ��ַ
    0x4000,
    0x5000,
};

// ҳ����4B
typedef struct page_entry_t {
  uint8 present : 1;  // ���ڴ���
  uint8 write : 1;    // 0 ֻ��  1 �ɶ���д
  uint8 user : 1;     // 1 ������   0 �����û� DPL<3
  uint8 pwt : 1;      // 1 ֱдģʽ    0 ��дģʽ
  uint8 pcd : 1;      // ��ֹ��ҳ����
  uint8 accessed : 1; // �����ʹ���ͳ��ʹ��Ƶ��
  uint8 dirty : 1;    // ��ҳ
  uint8 pat : 1;      // ҳ��С
  uint8 global : 1;   // ȫ�֣����н��̶��õ�
  uint8 shared : 1;   // �����ڴ�ҳ
  uint8 privat : 1;   // ˽���ڴ�ҳ
  uint8 flag : 1;
  uint32 index : 20; // ҳ����
} page_entry_t;

// ȱҳ��ַ����cr2
uint32 get_cr2();

uint32 get_cr3();
// ����ҳĿ¼��ַ
void set_cr3(uint32 pde);

// ����count���������ں�ҳ
uint32 alloc_kpage(uint32 count);
// �ͷ�count���������ں�ҳ
void free_kpage(uint32 vaddr, uint32 count);

// ��vaddrӳ�������ڴ�
void link_page(uint32 vaddr);
void unlink_page(uint32 vaddr);

void flush_tlb(uint32 vaddr);

page_entry_t *copy_pde();

void free_pde();

int32 sys_brk(void *addr);

#endif