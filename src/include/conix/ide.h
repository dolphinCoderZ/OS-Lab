#ifndef CONIX_IDE_H
#define CONIX_IDE_H

#include "mutex.h"
#include "task.h"
#include "types.h"

#define SECTOR_SIZE 512

#define IDE_CTRL_NR 2
#define IDE_DISK_NR 2
#define IDE_PART_NR 4 // ÿ�����̷���������ֻ֧�����������ܹ� 4 ��

typedef struct part_entry_t {
  uint8 bootable;             // ������־
  uint8 start_head;           // ������ʼ��ͷ��
  uint8 start_sector : 6;     // ������ʼ������
  uint16 start_cylinder : 10; // ������ʼ�����
  uint8 system;               // ���������ֽ�
  uint8 end_head;             // �����Ľ�����ͷ��
  uint8 end_sector : 6;       // ��������������
  uint16 end_cylinder : 10;   // �������������
  uint32 start;               // ������ʼ���������� LBA
  uint32 count;               // ����ռ�õ�������
} _packed part_entry_t;

typedef struct boot_sector_t {
  uint8 code[446];
  part_entry_t entry[4];
  uint16 signature;
} _packed boot_sector_t;

typedef struct ide_part_t {
  char name[8];            // ��������
  struct ide_disk_t *disk; // ����ָ��
  uint32 system;           // ��������
  uint32 start;            // ������ʼ���������� LBA
  uint32 count;            // ����ռ�õ�������
} ide_part_t;

typedef struct ide_disk_t {
  char name[8];
  struct ide_ctrl_t *ctrl;
  uint8 selector;
  bool master;
  uint32 total_lba;              // ������������
  ide_part_t parts[IDE_PART_NR]; // Ӳ�̷���
} ide_disk_t;

typedef struct ide_ctrl_t {
  char name[8];
  lock_t lock;
  uint16 iobase; // IO �Ĵ�����ַ
  ide_disk_t disks[IDE_DISK_NR];
  ide_disk_t *active;
  struct task_t *waiter;
} ide_ctrl_t;

int ide_pio_read(ide_disk_t *disk, void *buf, uint8 count, idx_t lba);
int ide_pio_write(ide_disk_t *disk, void *buf, uint8 count, idx_t lba);

#endif