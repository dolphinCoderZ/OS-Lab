#ifndef CONIX_BUFFER_H
#define CONIX_BUFFER_H

#include "list.h"
#include "mutex.h"
#include "types.h"

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE)

typedef struct buffer_t {
  char *data; // ��������
  dev_t dev;
  idx_t block;       // ���
  int count;         // ���ü���
  list_node_t hnode; // hash�����ڵ�
  list_node_t rnode; // ����ڵ�
  lock_t lock;
  bool dirty; // �Ƿ�����̲�һ��
  bool valid;
} buffer_t;

buffer_t *getblk(dev_t dev, idx_t block);
buffer_t *bread(dev_t dev, idx_t block);
void bwrite(buffer_t *bf);
void brelse(buffer_t *bf);

#endif