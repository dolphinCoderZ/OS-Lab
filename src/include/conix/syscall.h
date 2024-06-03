#ifndef CONIX_SYSCALL_H
#define CONIX_SYSCALL_H

#include "types.h"

typedef enum syscall_t {
  SYS_NR_TEST,
  SYS_NR_EXIT = 1,
  SYS_NR_FORK = 2,
  SYS_NR_READ = 3,
  SYS_NR_WRITE = 4,
  SYS_NR_OPEN = 5,
  SYS_NR_CLOSE = 6,
  SYS_NR_WAITPID = 7,
  SYS_NR_CREAT = 8,
  SYS_NR_LINK = 9,
  SYS_NR_UNLINK = 10,
  SYS_NR_CHDIR = 12,
  SYS_NR_TIME = 13,
  SYS_NR_LSEEK = 19,
  SYS_NR_GETPID = 20,
  SYS_NR_MKDIR = 39,
  SYS_NR_RMDIR = 40,
  SYS_NR_BRK = 45,
  SYS_NR_UMASK = 60,
  SYS_NR_CHROOT = 61,
  SYS_NR_GETPPID = 64,
  SYS_NR_YIELD = 158,
  SYS_NR_SLEEP = 162,
  SYS_NR_GETCWD = 183,
} syscall_t;

uint32 test();
void yield();
void sleep(uint32 ms);

void exit(int status);

pid_t fork();
pid_t getpid();
pid_t getppid();
pid_t waitpid(pid_t pid, int32 *status);

int32 brk(void *addr);

time_t time();

int mkdir(char *pathname, int mode);
int rmdir(char *pathname);

fd_t open(char *filename, int flags, int mode);
fd_t creat(char *filename, int mode);
void close(fd_t fd);
int32 read(fd_t fd, char *buf, int len);
int32 write(fd_t fd, char *buf, int len);
int lseek(fd_t fd, off_t offset, int whence);

mode_t umask(mode_t mask);
// Ó²Á´½Ó
int link(char *oldname, char *newname);
int unlink(char *filename);

char *getcwd(char *buf, size_t size);
int chdir(char *pathname);
int chroot(char *pathname);

#endif