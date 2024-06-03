#ifndef CONIX_RTC_H
#define CONIX_RTC_H

#include "types.h"

uint8 cmos_read(uint8 addr);
void cmos_write(uint8 addr, uint8 value);
void set_alarm(uint32 seconds);

#endif