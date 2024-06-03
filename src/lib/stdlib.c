#include "../include/conix/stdlib.h"

void delay(uint32 count) {
  while (count--) {
    ;
  }
}

void hang() {
  while (1) {
    ;
  }
}

uint8 bcd_to_bin(uint8 value) { return (value & 0xf) + (value >> 4) * 10; }

uint8 bin_to_bcd(uint8 value) { return (value / 10) * 0x10 + (value % 10); }

uint32 div_round_up(uint32 num, uint32 size) { return (num + size - 1) / size; }
