#include "../include/conix/arena.h"
#include "../include/conix/assert.h"
#include "../include/conix/conix.h"
#include "../include/conix/memory.h"
#include "../include/conix/stdlib.h"
#include "../include/conix/string.h"

extern uint32 free_pages;
static arena_descriptor_t descriptors[DESC_COUNT];

void arena_init() {
  uint32 block_size = 16;

  for (size_t i = 0; i < DESC_COUNT; ++i) {
    arena_descriptor_t *desc = &descriptors[i];
    desc->block_size = block_size;
    desc->total_block = (PAGE_SIZE - sizeof(arena_t)) / block_size;
    list_init(&desc->free_list);
    block_size <<= 1;
  }
}

static void *get_arena_block(arena_t *arena, uint32 idx) {
  assert(arena->desc->total_block > idx);
  void *addr = (void *)(arena + 1);
  uint32 gap = idx * arena->desc->block_size;
  return addr + gap;
}

static arena_t *get_block_arena(block_t *block) {
  return (arena_t *)((uint32)block & 0xfffff000);
}

void *kmalloc(size_t size) {
  arena_descriptor_t *desc = NULL;
  arena_t *arena;
  block_t *block;
  char *addr;

  if (size > 1024) {
    uint32 asize = size + sizeof(arena_t);
    uint32 count = div_round_up(asize, PAGE_SIZE);

    arena = (arena_t *)alloc_kpage(count);
    memset(arena, 0, count * PAGE_SIZE);
    arena->large = true;
    arena->count = count;
    arena->desc = NULL;
    arena->magic = CONIX_MAGIC;

    addr = (char *)((uint32)arena + sizeof(arena_t));
    return addr;
  }

  for (size_t i = 0; i < DESC_COUNT; ++i) {
    desc = &descriptors[i];
    if (desc->block_size >= size) {
      break;
    }
  }

  assert(desc != NULL);

  if (list_empty(&desc->free_list)) {
    arena = (arena_t *)alloc_kpage(1);
    memset(arena, 0, PAGE_SIZE);

    arena->desc = desc;
    arena->large = false;
    arena->count = desc->total_block;
    arena->magic = CONIX_MAGIC;

    for (size_t i = 0; i < desc->total_block; ++i) {
      block = get_arena_block(arena, i);
      assert(!list_search(&arena->desc->free_list, block));
      list_push(&arena->desc->free_list, block);
      assert(list_search(&arena->desc->free_list, block));
    }
  }

  block = list_pop(&desc->free_list);
  arena = get_block_arena(block);

  assert(arena->magic == CONIX_MAGIC && !arena->large);
  arena->count--;
  return block;
}

void kfree(void *ptr) {
  assert(ptr);
  block_t *block = (block_t *)ptr;
  arena_t *arena = get_block_arena(block);

  assert(arena->large == 1 || arena->large == 0);
  assert(arena->magic == CONIX_MAGIC);

  if (arena->large) {
    free_kpage((uint32)arena, arena->count);
    return;
  }

  list_push(&arena->desc->free_list, block);
  arena->count++;

  // 所有块都释放，就释放页面
  if (arena->count == arena->desc->total_block) {
    for (size_t i = 0; i < arena->desc->total_block; ++i) {
      block = get_arena_block(arena, i);
      assert(list_search(&arena->desc->free_list, block));
      list_remove(block);
      assert(!list_search(&arena->desc->free_list, block));
    }
    free_kpage((uint32)arena, 1);
  }
}
