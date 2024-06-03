#include "../include/conix/assert.h"
#include "../include/conix/bitmap.h"
#include "../include/conix/buffer.h"
#include "../include/conix/fs.h"

idx_t balloc(dev_t dev) {
  super_block_t *sb = get_super(dev);
  assert(sb);

  buffer_t *buf = NULL;
  idx_t bit = EOF;
  bitmap_t map;

  for (size_t i = 0; i < ZMAP_NR; ++i) {
    buf = sb->zmap[i];
    assert(buf);

    // 整个buffer作为位图
    bitmap_make(&map, buf->data, BLOCK_SIZE,
                i * BLOCK_BITS + sb->desc->firstdatazone - 1);

    bit = bitmap_scan(&map, 1);
    if (bit != EOF) {
      assert(bit < sb->desc->zones);
      buf->dirty = true;
      break;
    }
  }
  bwrite(buf);
  return bit;
}

void bfree(dev_t dev, idx_t idx) {
  super_block_t *sb = get_super(dev);
  assert(sb != NULL);
  assert(idx < sb->desc->zones);

  buffer_t *buf;
  bitmap_t map;
  for (size_t i = 0; i < ZMAP_NR; ++i) {
    if (idx > BLOCK_BITS * (i + 1)) {
      continue;
    }

    buf = sb->zmap[i];
    assert(buf);

    bitmap_make(&map, buf->data, BLOCK_SIZE,
                BLOCK_BITS * i + sb->desc->firstdatazone - 1);
    // 置0
    assert(bitmap_test(&map, idx));
    bitmap_set(&map, idx, 0);

    buf->dirty = true;
    break;
  }
  bwrite(buf);
}

idx_t ialloc(dev_t dev) {
  super_block_t *sb = get_super(dev);
  assert(sb);

  buffer_t *buf = NULL;
  idx_t bit = EOF;
  bitmap_t map;

  for (size_t i = 0; i < IMAP_NR; ++i) {
    buf = sb->imap[i];
    assert(buf);

    bitmap_make(&map, buf->data, BLOCK_BITS, i * BLOCK_BITS);
    bit = bitmap_scan(&map, 1);
    if (bit != EOF) {
      assert(bit < sb->desc->inodes);
      buf->dirty = true;
      break;
    }
  }
  bwrite(buf);
  return bit;
}

void ifree(dev_t dev, idx_t idx) {
  super_block_t *sb = get_super(dev);
  assert(sb);
  assert(idx < sb->desc->inodes);

  buffer_t *buf;
  bitmap_t map;

  for (size_t i = 0; i < IMAP_NR; ++i) {
    if (idx > BLOCK_BITS * (i + 1)) {
      continue;
    }

    buf = sb->imap[i];
    assert(buf);

    bitmap_make(&map, buf->data, BLOCK_BITS, i * BLOCK_BITS);
    assert(bitmap_test(&map, idx));
    bitmap_set(&map, idx, 0);
    buf->dirty = true;
    break;
  }
  bwrite(buf);
}

idx_t bmap(inode_t *inode, idx_t block, bool create) {
  assert(block >= 0 && block < TOTAL_BLOCK);

  uint16 index = block;
  uint16 *array = inode->desc->zone;
  buffer_t *buf = inode->buf;

  buf->count += 1;
  int level = 0;

  int divider = 1;
  if (block < DIRECT_BLOCK) {
    goto reckon;
  }

  block -= DIRECT_BLOCK;
  if (block < INDIRECT1_BLOCK) {
    index = DIRECT_BLOCK;
    level = 1;
    divider = 1;
    goto reckon;
  }

  block -= INDIRECT1_BLOCK;
  assert(block < INDIRECT2_BLOCK);
  index = DIRECT_BLOCK + 1;
  level = 2;
  divider = BLOCK_INDEXES;

reckon:
  for (; level >= 0; level--) {
    if (!array[index] && create) {
      array[index] = balloc(inode->dev);
      buf->dirty = true;
    }

    brelse(buf);

    if (level == 0 || !array[index]) {
      return array[index];
    }

    buf = bread(inode->dev, array[index]);
    index = block / divider;
    block = block % divider;
    divider /= BLOCK_INDEXES;
    array = (uint16 *)buf->data;
  }
}
