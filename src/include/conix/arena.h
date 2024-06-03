#ifndef CONIX_ARENA_H
#define CONIX_ARENA_H

#include "list.h"
#include "types.h"

#define DESC_COUNT 7

typedef list_node_t block_t; // �ڴ��

typedef struct arena_descriptor_t {
  uint32 total_block; // һҳ�ڴ�ֳɶ��ٿ�
  uint32 block_size;
  list_t free_list;
} arena_descriptor_t;

typedef struct arena_t {
  arena_descriptor_t *desc;
  uint32 count; // ʣ���
  uint32 large;
  uint32 magic;
} arena_t;

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif