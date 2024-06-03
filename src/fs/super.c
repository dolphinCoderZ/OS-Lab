#include "../include/conix/assert.h"
#include "../include/conix/buffer.h"
#include "../include/conix/device.h"
#include "../include/conix/fs.h"
#include "../include/conix/string.h"

#define SUPER_NR 16

static super_block_t super_table[SUPER_NR];
static super_block_t *root; // 根文件系统超级块

static super_block_t *get_free_super() {
  for (size_t i = 0; i < SUPER_NR; ++i) {
    super_block_t *sb = &super_table[i];
    if (sb->dev == EOF) {
      return sb;
    }
  }
  panic("no more super block");
}

super_block_t *get_super(dev_t dev) {
  for (size_t i = 0; i < SUPER_NR; ++i) {
    super_block_t *sb = &super_table[i];
    if (sb->dev == dev) {
      return sb;
    }
  }
  return NULL;
}

super_block_t *read_super(dev_t dev) {
  super_block_t *sb = get_super(dev);
  if (sb) {
    return sb;
  }

  sb = get_free_super();
  buffer_t *buf = bread(dev, 1);
  sb->buf = buf;
  sb->desc = (super_desc_t *)buf->data;
  sb->dev = dev;
  assert(sb->desc->magic == MINIX1_MAGIC);

  memset(sb->imap, 0, sizeof(sb->imap));
  memset(sb->zmap, 0, sizeof(sb->zmap));

  int idx = 2;
  for (int i = 0; i < sb->desc->imap_blocks; ++i) {
    assert(i < IMAP_NR);
    if ((sb->imap[i] = bread(dev, idx))) {
      idx++;
    } else {
      break;
    }
  }

  for (int i = 0; i < sb->desc->zmap_blocks; ++i) {
    assert(i < ZMAP_NR);
    if ((sb->zmap[i] = bread(dev, idx))) {
      idx++;
    } else {
      break;
    }
  }

  return sb;
}

static void mount_root() {
  device_t *device = device_find(DEV_IDE_PART, 0);
  assert(device);
  root = read_super(device->dev);

  // 初始化根目录inode
  root->iroot = iget(device->dev, 1);
  root->imount = iget(device->dev, 1);
}

void super_init() {
  for (size_t i = 0; i < SUPER_NR; ++i) {
    super_block_t *sb = &super_table[i];
    sb->dev = EOF;
    sb->desc = NULL;
    sb->buf = NULL;
    sb->iroot = NULL;
    sb->imount = NULL;
    list_init(&sb->inode_list);
  }

  mount_root();
}