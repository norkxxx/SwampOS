#pragma once
#include <stdint.h>
struct rtc_time {unsigned year2,month,day,hour,minute,second;};
typedef uint8_t (*rtc_read_reg)(void*,uint8_t);
/* No century inference; RTC timezone is unknown. Bounded stable read. */
int rtc_read_time(rtc_read_reg,void*,struct rtc_time*);
#ifndef RTC_HOST
int rtc_hardware_time(struct rtc_time*);
#endif
