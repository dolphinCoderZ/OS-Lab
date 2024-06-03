#include "../include/conix/arena.h"
#include "../include/conix/assert.h"
#include "../include/conix/buffer.h"
#include "../include/conix/fs.h"
#include "../include/conix/stat.h"
#include "../include/conix/stdlib.h"
#include "../include/conix/string.h"
#include "../include/conix/syscall.h"

#define INODE_NR 64

static inode_t inode_table[INODE_NR];

static inode_t *get_free_inode() {
  for (size_t i = 0; i < INODE_NR; ++i) {
    inode_t *inode = &inode_table[i];
    if (inode->dev == EOF) {
      return inode;
    }
  }

  panic("no more inode");
}

static void put_free_inode(inode_t *inode) {
  assert(inode != inode_table);
  assert(inode->count == 0);
  inode->dev = EOF;
}

inode_t *get_root_inode() { return inode_table; }

// 计算inode nr对应的块号
static idx_t inode_block(super_block_t *sb, idx_t nr) {
  return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks +
         (nr - 1) / BLOCK_INODES;
}

static inode_t *find_inode(dev_t dev, idx_t nr) {
  super_block_t *sb = get_super(dev);
  assert(sb);
  list_t *list = &sb->inode_list;
  for (list_node_t *node = list->head.next; node != &list->tail;
       node = node->next) {
    inode_t *inode = element_entry(inode_t, node, node);
    if (inode->nr == nr) {
      return inode;
    }
  }

  return NULL;
}

inode_t *iget(dev_t dev, idx_t nr) {
  inode_t *inode = find_inode(dev, nr);
  if (inode) {
    inode->count++;
    inode->atime = time();
    return inode;
  }

  super_block_t *sb = get_super(dev);
  assert(sb);
  assert(nr <= sb->desc->inodes);

  inode = get_free_inode();
  inode->dev = dev;
  inode->nr = nr;
  inode->count++;

  list_push(&sb->inode_list, &inode->node);

  idx_t block = inode_block(sb, inode->nr);
  buffer_t *buf = bread(inode->dev, block);
  inode->buf = buf;
  inode->desc = &((inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

  inode->ctime = inode->desc->mtime;
  inode->atime = time();

  return inode;
}

void iput(inode_t *inode) {
  if (!inode) {
    return;
  }

  if (inode->buf->dirty) {
    bwrite(inode->buf);
  }

  inode->count--;
  if (inode->count) {
    return;
  }

  brelse(inode->buf);
  list_remove(&inode->node);
  put_free_inode(inode);
}

void inode_init() {
  for (size_t i = 0; i < INODE_NR; ++i) {
    inode_t *inode = &inode_table[i];
    inode->dev = EOF;
  }
}

int inode_read(inode_t *inode, char *buf, uint32 len, off_t offset) {
  if (offset >= inode->desc->size) {
    return EOF;
  }
  uint32 begin = offset;

  uint32 left = MIN(len, inode->desc->size - offset);
  while (left) {
    // 所在文件块
    idx_t nr = bmap(inode, offset / BLOCK_SIZE, false);
    assert(nr);

    // 读文件块缓冲
    buffer_t *bf = bread(inode->dev, nr);
    // 块内偏移
    uint32 start = offset % BLOCK_SIZE;

    uint32 chars = MIN(BLOCK_SIZE - start, left);
    offset += chars;
    left -= chars;

    char *ptr = bf->data + start;
    memcpy(buf, ptr, chars);
    buf += chars;
    brelse(bf);
  }

  inode->atime = time();
  return offset - begin;
}

int inode_write(inode_t *inode, char *buf, uint32 len, off_t offset) {
  assert(ISFILE(inode->desc->mode));

  uint32 begin = offset;
  uint32 left = len;

  while (left) {
    idx_t nr = bmap(inode, offset / BLOCK_SIZE, true);
    assert(nr);

    buffer_t *bf = bread(inode->dev, nr);
    bf->dirty = true;

    uint32 start = offset % BLOCK_SIZE;
    char *ptr = bf->data + start;

    uint32 chars = MIN(BLOCK_SIZE - start, left);
    offset += chars;
    left -= chars;

    if (offset > inode->desc->size) {
      inode->desc->size = offset;
      inode->buf->dirty = true;
    }

    memcpy(ptr, buf, chars);
    buf += chars;
    brelse(bf);
  }

  inode->desc->mtime = inode->atime = time();
  bwrite(inode->buf);
  return offset - begin;
}

static void inode_bfree(inode_t *inode, uint16 *array, int index, int level) {
  if (!array[index]) {
    return;
  }
  if (!level) {
    bfree(inode->dev, array[index]);
    return;
  }

  buffer_t *buf = bread(inode->dev, array[index]);
  for (size_t i = 0; i < BLOCK_INODES; ++i) {
    inode_bfree(inode, (uint16 *)buf->data, i, level - 1);
  }
  brelse(buf);
  bfree(inode->dev, array[index]);
}

void inode_truncate(inode_t *inode) {
  if (!ISFILE(inode->desc->mode) && !ISDIR(inode->desc->mode)) {
    return;
  }

  // 释放直接块
  for (size_t i = 0; i < DIRECT_BLOCK; ++i) {
    inode_bfree(inode, inode->desc->zone, i, 0);
    inode->desc->zone[i] = 0;
  }
  // 一级间接块
  inode_bfree(inode, inode->desc->zone, DIRECT_BLOCK, 1);
  inode->desc->zone[DIRECT_BLOCK] = 0;

  // 二级间接块
  inode_bfree(inode, inode->desc->zone, DIRECT_BLOCK + 1, 2);
  inode->desc->zone[DIRECT_BLOCK + 1] = 0;

  inode->desc->size = 0;
  inode->buf->dirty = true;
  inode->desc->mtime = time();
  bwrite(inode->buf);
}
