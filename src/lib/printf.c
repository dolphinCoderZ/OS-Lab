#include "../include/conix/stdarg.h"
#include "../include/conix/stdio.h"
#include "../include/conix/syscall.h"

static char buf[1024];

int printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int i = vsprintf(buf, fmt, args);
  va_end(args);
  write(stdout, buf, i);
  return i;
}
