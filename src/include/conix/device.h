#ifndef CONIX_DEVICE_H
#define CONIX_DEVICE_H

#include "list.h"
#include "types.h"

#define NAMELEN 16

enum device_type_t {
  DEV_NULL,
  DEV_CHAR,
  DEV_BLOCK,
};

enum device_subtype_t {
  DEV_CONSOLE = 1,
  DEV_KEYBOARD,
  DEV_IDE_DISK,
  DEV_IDE_PART,
};

enum device_cmd_t {
  DEV_CMD_SECTOR_START = 1,
  DEV_CMD_SECTOR_COUNT,
};

#define REQ_READ 0  // ���豸��
#define REQ_WRITE 1 // ���豸д

#define DIRECT_UP 0
#define DIRECT_DOWN 1

typedef struct request_t {
  dev_t dev;
  uint32 type;  // ��������
  uint32 idx;   // ����λ��
  uint32 count; // ��������
  int flags;
  uint8 *buf; // ������
  struct task_t *task;
  list_node_t node;
} request_t;

typedef struct device_t {
  char name[NAMELEN];
  int type;
  int subtype;
  dev_t dev;
  dev_t parent;
  void *ptr;           // �豸ָ��
  list_t request_list; // ���豸��������
  bool direct;         // ����Ѱ������
  int (*ioctl)(void *dev, int cmd, void *args, int flags);
  int (*read)(void *dev, void *buf, size_t count, idx_t idx, int flags);
  int (*write)(void *dev, void *buf, size_t count, idx_t idx, int flags);
} device_t;

dev_t device_install(int type, int subtype, void *ptr, char *name, dev_t parent,
                     void *ioctl, void *read, void *write);

// �������Ͳ��ҵ�idx���豸
device_t *device_find(int subtype, idx_t idx);
// �����豸�Ų����豸
device_t *device_get(dev_t dev);

int device_ioctl(dev_t dev, int cmd, void *args, int flags);

int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags);

int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags);

void device_request(dev_t dev, void *buf, uint8 count, idx_t idx, int flags,
                    uint32 type);

#endif