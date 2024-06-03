#ifndef CONIX_TASK_H
#define CONIX_TASK_H

#include "fs.h"
#include "list.h"
#include "types.h"

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

#define TASK_FILE_NR 16

// ���庯��ָ������
typedef void (*target_t)();

typedef enum task_state_t {
  TASK_INIT,
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
  TASK_SLEEPING,
  TASK_WAITING,
  TASK_DIED,
} task_state_t;

// PCB
typedef struct task_t {
  uint32 *stack;    // �ں�ջ
  list_node_t node; // ���������ڵ�
  task_state_t state;
  uint32 priority;
  uint32 ticks;   // ʣ��ʱ��Ƭ
  uint32 jiffies; // �ϴ�ִ��ʱȫ��ʱ��Ƭ
  char name[TASK_NAME_LEN];
  uint32 uid;
  uint32 gid;
  pid_t pid;
  pid_t ppid;
  int status;
  pid_t waitpid; // ���̵ȴ����յ�pid
  char *pwd;
  uint32 pde;
  struct bitmap_t *vmap; // ���������ڴ�λͼ
  uint32 brk;            // ���̶��ڴ����ߵ�ַ
  struct inode_t *ipwd;
  struct inode_t *iroot;
  uint16 umask;
  struct file_t *files[TASK_FILE_NR];
  uint32 magic; // �ں�ħ�������ڼ��ջ���
} task_t;

// �����л�ʱ��Ҫ����ļĴ���
typedef struct task_frame_t {
  uint32 edi;
  uint32 esi;
  uint32 ebx;
  uint32 ebp;
  void (*eip)(void); // ��תĿ�꺯��λ��
} task_frame_t;

// �ж�֡
typedef struct intr_frame_t {
  uint32 vector;

  uint32 edi;
  uint32 esi;
  uint32 ebp;
  // ��Ȼ pushad �� esp Ҳѹ�룬�� esp �ǲ��ϱ仯�ģ����Իᱻ popad ����
  uint32 esp_dummy;
  uint32 ebx;
  uint32 edx;
  uint32 ecx;
  uint32 eax;
  uint32 gs;
  uint32 fs;
  uint32 es;
  uint32 ds;
  uint32 vector0;
  uint32 error;
  uint32 eip;
  uint32 cs;
  uint32 eflags;
  uint32 esp;
  uint32 ss;
} intr_frame_t;

task_t *running_task();
void schedule();

void task_exit(int status);
void task_yield();
void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);

void task_sleep(uint32 ms);
void task_wakeup();

void task_to_user_mode(target_t target);

pid_t task_fork();

pid_t sys_getpid();
pid_t sys_getppid();
pid_t task_waitpid(pid_t pid, int *status);

fd_t task_get_fd(task_t *task);
void task_put_fd(task_t *task, fd_t fd);

#endif