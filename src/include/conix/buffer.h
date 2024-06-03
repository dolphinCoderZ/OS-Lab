#ifndef CONIX_BUFFER_H
#define CONIX_BUFFER_H

#include "list.h"
#include "mutex.h"
#include "types.h"

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE)

typedef struct buffer_t {
  char *data; // 缓存数据
  dev_t dev;
  idx_t block;       // 块号
  int count;         // 引用计数
  list_node_t hnode; // hash拉链节点
  list_node_t rnode; // 缓冲节点
  lock_t lock;
  bool dirty; // 是否与磁盘不一致
  bool valid;
} buffer_t;

buffer_t *getblk(dev_t dev, idx_t block);
buffer_t *bread(dev_t dev, idx_t block);
void bwrite(buffer_t *bf);
void brelse(buffer_t *bf);

#endif