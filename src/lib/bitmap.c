#include "../include/conix/bitmap.h"
#include "../include/conix/assert.h"
#include "../include/conix/string.h"

void bitmap_make(bitmap_t *map, char *bits, uint32 length, uint32 offset) {
  map->bits = bits;
  map->length = length;
  map->offset = offset;
}

void bitmap_init(bitmap_t *map, char *bits, uint32 length, uint32 start) {
  memset(bits, 0, length);
  bitmap_make(map, bits, length, start);
}

bool bitmap_test(bitmap_t *map, idx_t index) {
  assert(index >= map->offset);

  // λͼ����
  idx_t idx = index - map->offset;

  // ��λλͼ�е��ֽ�
  uint32 bytes = idx / 8;
  // �ֽ��е���һλ
  uint8 bits = idx % 8;

  assert(bytes < map->length);
  return (map->bits[bytes] & (1 << bits));
}

void bitmap_set(bitmap_t *map, idx_t index, bool value) {
  assert(value == 0 || value == 1);
  assert(index >= map->offset);

  // λͼ����
  idx_t idx = index - map->offset;

  // ��λλͼ�е��ֽ�
  uint32 bytes = idx / 8;
  // �ֽ��е���һλ
  uint8 bits = idx % 8;

  if (value) {
    map->bits[bytes] |= (1 << bits);
  } else {
    map->bits[bytes] &= ~(1 << bits);
  }
}

// ��λͼ���ҵ�������countλ
int bitmap_scan(bitmap_t *map, uint32 count) {
  int start = EOF;
  uint32 bits_left = map->length * 8;
  uint32 next_bit = 0;
  uint32 counter = 0;

  while (bits_left-- > 0) {
    if (!bitmap_test(map, map->offset + next_bit)) {
      counter++;
    } else {
      counter = 0;
    }
    next_bit++;

    if (counter == count) {
      start = next_bit - count;
      break;
    }
  }

  // û�ҵ�
  if (start == EOF) {
    return EOF;
  }

  // �ҵ���λȫ����1
  bits_left = count;
  next_bit = start;
  while (bits_left--) {
    bitmap_set(map, map->offset + next_bit, true);
    next_bit++;
  }
  return map->offset + start;
}
