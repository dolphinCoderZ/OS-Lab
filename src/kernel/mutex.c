#include "../include/conix/mutex.h"
#include "../include/conix/assert.h"
#include "../include/conix/conix.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/task.h"

void mutex_init(mutex_t *mutex) {
  mutex->value = false;
  list_init(&mutex->waiters);
}

void mutex_lock(mutex_t *mutex) {
  // 关中断，保证原子操作
  bool intr = interrupt_disable();

  task_t *cur = running_task();
  while (mutex->value == true) {
    task_block(cur, &mutex->waiters, TASK_BLOCKED);
  }

  assert(mutex->value == false);
  mutex->value++;
  assert(mutex->value == true);

  set_interrupt_state(intr);
}

void mutex_unlock(mutex_t *mutex) {
  bool intr = interrupt_disable();

  mutex->value--;
  assert(mutex->value == false);

  if (!list_empty(&mutex->waiters)) {
    task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);
    assert(task->magic == CONIX_MAGIC);
    task_unblock(task);
    // 保证新进程获得互斥量，防止饿死
    task_yield();
  }

  set_interrupt_state(intr);
}

void lock_init(lock_t *lock) {
  lock->holder = NULL;
  lock->repeat = 0;
  mutex_init(&lock->mutex);
}

void lock_acquire(lock_t *lock) {
  task_t *cur = running_task();

  if (lock->holder != cur) {
    mutex_lock(&lock->mutex);
    lock->holder = cur;
    assert(lock->repeat == 0);
    lock->repeat = 1;
  } else {
    lock->repeat++;
  }
}

void lock_release(lock_t *lock) {
  task_t *cur = running_task();
  assert(lock->holder == cur);

  if (lock->repeat > 1) {
    lock->repeat--;
    return;
  }

  assert(lock->repeat == 1);
  lock->holder = NULL;
  lock->repeat = 0;
  mutex_unlock(&lock->mutex);
}
