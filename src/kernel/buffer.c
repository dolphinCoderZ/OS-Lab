#include "../include/conix/buffer.h"
#include "../include/conix/assert.h"
#include "../include/conix/device.h"
#include "../include/conix/memory.h"

#define HASH_COUNT 31

static buffer_t *buffer_start = (buffer_t *)KERNEL_BUFFER_MEM;
static uint32 buffer_count = 0;

// 当前buffer_t结构体的位置，从头部开始
static buffer_t *buffer_ptr = (buffer_t *)KERNEL_BUFFER_MEM;
// 当前数据缓冲区位置，从尾部开始
static void *buffer_data =
    (void *)(KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE - BLOCK_SIZE);

static list_t free_list;              // 缓存链表
static list_t wait_list;              // 等待进程链表
static list_t hash_table[HASH_COUNT]; // 缓存哈希表

uint32 hash(dev_t dev, idx_t block) { return (dev ^ block) % HASH_COUNT; }

static buffer_t *get_from_hash_table(dev_t dev, idx_t block) {
  uint32 idx = hash(dev, block);
  list_t *list = &hash_table[idx];
  buffer_t *bf = NULL;

  for (list_node_t *node = list->head.next; node != &list->tail;
       node = node->next) {
    buffer_t *ptr = element_entry(buffer_t, hnode, node);
    if (ptr->dev == dev && ptr->block == block) {
      bf = ptr;
      break;
    }
  }

  if (!bf) {
    return NULL;
  }

  // 如果在空闲缓冲链表，则摘下
  if (list_search(&free_list, &bf->rnode)) {
    list_remove(&bf->rnode);
  }

  return bf;
}

// 放回hash
static void hash_locate(buffer_t *bf) {
  uint32 idx = hash(bf->dev, bf->block);
  list_t *list = &hash_table[idx];
  assert(!list_search(&free_list, &bf->rnode));
  list_push(list, &bf->rnode);
}

static void hash_remove(buffer_t *bf) {
  uint32 idx = hash(bf->dev, bf->block);
  list_t *list = &hash_table[idx];
  assert(list_search(list, &bf->rnode));
  list_remove(&bf->rnode);
}

// 初始化时，获得缓冲
static buffer_t *get_new_buffer() {
  buffer_t *bf = NULL;

  if ((uint32)buffer_ptr + sizeof(buffer_t) < (uint32)buffer_data) {
    bf = buffer_ptr;
    bf->data = buffer_data;
    bf->dev = EOF;
    bf->block = 0;
    bf->count = 0;
    bf->dirty = false;
    bf->valid = false;
    lock_init(&bf->lock);
    buffer_count++;
    buffer_ptr++;
    buffer_data -= BLOCK_SIZE;
  }
  return bf;
}

static buffer_t *get_free_buffer() {
  buffer_t *bf = NULL;
  while (1) {
    // 初始时，所有缓冲都没用过，可以直接获取
    bf = get_new_buffer();
    if (bf) {
      return bf;
    }

    // 用过的缓存通过hash来管理
    if (!list_empty(&free_list)) {
      bf = element_entry(buffer_t, rnode, list_popback(&free_list));
      hash_remove(bf);
      bf->valid = false;
      return bf;
    }

    task_block(running_task(), &wait_list, TASK_BLOCKED);
  }
}

// 获取设备dev，第block对应的缓存
buffer_t *getblk(dev_t dev, idx_t block) {
  buffer_t *bf = get_from_hash_table(dev, block);
  if (bf) {
    bf->count++;
    return bf;
  }

  // 没有对应的缓存，就将该设备的第block进行缓存
  bf = get_free_buffer();
  assert(bf->count == 0);
  assert(bf->dirty == 0);

  bf->count = 1;
  bf->dev = dev;
  bf->block = block;
  hash_locate(bf); // 放入hash进行管理
  return bf;
}

buffer_t *bread(dev_t dev, idx_t block) {
  buffer_t *bf = getblk(dev, block);
  assert(bf != NULL);

  if (bf->valid) {
    return bf;
  }

  lock_acquire(&bf->lock);
  if (!bf->valid) {
    device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0,
                   REQ_READ);
    bf->dirty = false;
    bf->valid = true;
  }
  lock_release(&bf->lock);
  return bf;
}

void bwrite(buffer_t *bf) {
  assert(bf);
  if (!bf->dirty) {
    return;
  }

  // 写入磁盘
  device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0,
                 REQ_WRITE);
  bf->dirty = false;
  bf->valid = true;
}

void brelse(buffer_t *bf) {
  if (!bf) {
    return;
  }

  bf->count--;
  assert(bf->count >= 0);
  if (bf->count == 0) {
    list_push(&free_list, &bf->rnode);
  }

  if (bf->dirty) {
    bwrite(bf); // 立马写，保持强一致性
  }

  // 如果有等待使用缓冲的进程，则唤醒
  if (!list_empty(&wait_list)) {
    task_t *task = element_entry(task_t, node, list_popback(&wait_list));
    task_unblock(task);
  }
}

void buffer_init() {
  list_init(&free_list);
  list_init(&wait_list);

  for (size_t i = 0; i < HASH_COUNT; ++i) {
    list_init(&hash_table[i]);
  }
}
