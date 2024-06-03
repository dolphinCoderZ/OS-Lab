#ifndef CONIX_MUTEX_H
#define CONIX_MUTEX_H

#include "list.h"
#include "task.h"
#include "types.h"

// ª•≥‚¡ø
typedef struct mutex_t {
  bool value;
  list_t waiters;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

// ª•≥‚À¯
typedef struct lock_t {
  struct task_t *holder;
  mutex_t mutex;
  uint32 repeat;
} lock_t;

void lock_init(lock_t *lock);
void lock_acquire(lock_t *lock);
void lock_release(lock_t *lock);

#endif