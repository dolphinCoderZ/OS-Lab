#ifndef CONIX_IO_H
#define CONIX_IO_H

#include "types.h"

extern uint8 inb(uint16 port);  // ����һ���ֽ�
extern uint16 inw(uint16 port); // ����һ����

extern void outb(uint16 port, uint8 value);  // ���һ���ֽ�
extern void outw(uint16 port, uint16 value); // ���һ����

#endif