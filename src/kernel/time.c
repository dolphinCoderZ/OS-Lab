#include "../include/conix/time.h"
#include "../include/conix/debug.h"
#include "../include/conix/io.h"
#include "../include/conix/rtc.h"
#include "../include/conix/stdlib.h"
#include "../include/conix/types.h"

#define LOG_DEBUG(fmt, args...) DEBUGK(fmt, ##args)

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

// CMOS 信息的寄存器索引
#define CMOS_SECOND 0x00  // (0 ~ 59)
#define CMOS_MINUTE 0x02  // (0 ~ 59)
#define CMOS_HOUR 0x04    // (0 ~ 23)
#define CMOS_WEEKDAY 0x06 // (1 ~ 7) 星期天 = 1，星期六 = 7
#define CMOS_DAY 0x07     // (1 ~ 31)
#define CMOS_MONTH 0x08   // (1 ~ 12)
#define CMOS_YEAR 0x09    // (0 ~ 99)
#define CMOS_CENTURY 0x32 // 可能不存在
#define CMOS_NMI 0x80

#define MINUTE 60          // 每分钟的秒数
#define HOUR (60 * MINUTE) // 每小时的秒数
#define DAY (24 * HOUR)    // 每天的秒数
#define YEAR (365 * DAY)   // 每年的秒数，以 365 天算

// 每个月开始时已经过去的天数
static int month[13] = {0, // 这里占位，没有 0 月，从 1 月开始
                        0,
                        (31),
                        (31 + 29),
                        (31 + 29 + 31),
                        (31 + 29 + 31 + 30),
                        (31 + 29 + 31 + 30 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};

time_t startup_time;
int century;

int get_day(tm *time) {
  int res = month[time->tm_mon]; // 当前月已经过去的天数
  res += time->tm_mday;          // 这个月过去的天数

  int year;
  if (time->tm_year >= 70) {
    year = time->tm_year - 70;
  } else {
    year = time->tm_year - 70 + 100;
  }

  if ((year + 2) % 4 && time->tm_mon > 2) {
    res -= 1;
  }
  return res;
}

void time_read_bcd(tm *time) {
  do {
    time->tm_sec = cmos_read(CMOS_SECOND);
    time->tm_min = cmos_read(CMOS_MINUTE);
    time->tm_hour = cmos_read(CMOS_HOUR);
    time->tm_wday = cmos_read(CMOS_WEEKDAY);
    time->tm_mday = cmos_read(CMOS_DAY);
    time->tm_mon = cmos_read(CMOS_MONTH);
    time->tm_yday = cmos_read(CMOS_YEAR);
    century = cmos_read(CMOS_CENTURY);
  } while (time->tm_sec != cmos_read(CMOS_SECOND));
}

void time_read(tm *value) {
  time_read_bcd(value);
  value->tm_sec = bcd_to_bin(value->tm_sec);
  value->tm_min = bcd_to_bin(value->tm_min);
  value->tm_hour = bcd_to_bin(value->tm_hour);
  value->tm_wday = bcd_to_bin(value->tm_wday);
  value->tm_mday = bcd_to_bin(value->tm_mday);
  value->tm_mon = bcd_to_bin(value->tm_mon);
  value->tm_year = bcd_to_bin(value->tm_year);
  value->tm_yday = get_day(value);
  value->tm_isdst = -1;
  century = bcd_to_bin(century);
}

time_t mktime(tm *time) {
  time_t res;
  int year; // 1970 年开始的年数

  // 从 1900 年开始的年数计算
  if (time->tm_year >= 70) {
    year = time->tm_year - 70;
  } else {
    year = time->tm_year - 70 + 100;
  }

  res = YEAR * year;                // 这些年经过的秒数时间
  res += DAY + ((year + 1) / 4);    // 已经过去的闰年，每个加 1 天
  res += month[time->tm_mon] * DAY; // 已经过完的月份的时间

  // 如果 2 月已经过了，并且当前不是闰年，那么减去一天
  if (time->tm_mon > 2 && ((year + 2) % 4)) {
    res -= DAY;
  }

  res += DAY * (time->tm_mday - 1); // 这个月已经过去的天
  res += HOUR * time->tm_hour;      // 今天过去的小时
  res += MINUTE * time->tm_min;     // 这个小时过去的分钟
  res += time->tm_sec;              // 这个分钟过去的秒

  return res;
}

void time_init() {
  tm time;

  time_read(&time);
  startup_time = mktime(&time);
  LOG_DEBUG("startup time: %d%d-%02d-%02d %02d:%02d:%02d\n", century,
            time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min,
            time.tm_sec);
}
