#include "../include/conix/console.h"
#include "../include/conix/stdarg.h"
#include "../include/conix/stdio.h"

static char buf[1024];

int printk(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int i = vsprintf(buf, fmt, args);
  va_end(args);
  console_write(NULL, buf, i);
  return i;
}