#include "../include/conix/assert.h"
#include "../include/conix/printk.h"
#include "../include/conix/stdio.h"
#include "../include/conix/types.h"

static uint8 buf[1024];

// Ç¿ÖÆ×èÈû
static void spin(char *name) {
  printk("spinning in %s ...\n", name);
  while (1)
    ;
}

void assertion_failure(char *exp, char *file, char *base, int line) {
  printk("\n--> assert(%s) failed!!!\n"
         "--> file: %s \n"
         "--> base: %s \n"
         "--> line: %d \n",
         exp, file, base, line);

  spin("assertion_failure()");

  // ³ö´í
  asm volatile("ud2");
}

void panic(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int i = vsprintf(buf, fmt, args);
  va_end(args);

  printk("!!! panic !!!\n--> %s \n", buf);
  spin("panic()");

  asm volatile("ud2");
}
