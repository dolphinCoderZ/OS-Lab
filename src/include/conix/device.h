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

#define REQ_READ 0  // 块设备读
#define REQ_WRITE 1 // 块设备写

#define DIRECT_UP 0
#define DIRECT_DOWN 1

typedef struct request_t {
  dev_t dev;
  uint32 type;  // 请求类型
  uint32 idx;   // 扇区位置
  uint32 count; // 扇区数量
  int flags;
  uint8 *buf; // 缓冲区
  struct task_t *task;
  list_node_t node;
} request_t;

typedef struct device_t {
  char name[NAMELEN];
  int type;
  int subtype;
  dev_t dev;
  dev_t parent;
  void *ptr;           // 设备指针
  list_t request_list; // 块设备请求链表
  bool direct;         // 磁盘寻道方向
  int (*ioctl)(void *dev, int cmd, void *args, int flags);
  int (*read)(void *dev, void *buf, size_t count, idx_t idx, int flags);
  int (*write)(void *dev, void *buf, size_t count, idx_t idx, int flags);
} device_t;

dev_t device_install(int type, int subtype, void *ptr, char *name, dev_t parent,
                     void *ioctl, void *read, void *write);

// 根据类型查找第idx个设备
device_t *device_find(int subtype, idx_t idx);
// 根据设备号查找设备
device_t *device_get(dev_t dev);

int device_ioctl(dev_t dev, int cmd, void *args, int flags);

int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags);

int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags);

void device_request(dev_t dev, void *buf, uint8 count, idx_t idx, int flags,
                    uint32 type);

#endif