#include "../include/conix/debug.h"
#include "../include/conix/printk.h"
#include "../include/conix/stdarg.h"
#include "../include/conix/stdio.h"

static char buf[1024];

void debugk(char *file, int line, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  printk("[%s] [%d] %s", file, line, buf);
}