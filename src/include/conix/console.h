#ifndef CONIX_CONSOLE_H
#define CONIX_CONSOLE_H

#include "types.h"

void console_init();
void console_clear();
int32 console_write(void *dev, char *buf, uint32 count);

#endif