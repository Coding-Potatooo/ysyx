#include <am.h>
#include <nemu.h>
#include <stdio.h>

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // wrong implementation?!!!
  // uint32_t l = inl(RTC_ADDR);
  // uint32_t h = inl(RTC_ADDR + 4);
  // uptime->us = (uint64_t)l + ((uint64_t)h << 32);

  /* why wrong implementation?
    read the source code of rtc_io_handler:h

    static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
      assert(offset == 0 || offset == 4);
      if (!is_write && offset == 4) {
        uint64_t us = get_time();
        rtc_port_base[0] = (uint32_t)us;
        rtc_port_base[1] = us >> 32;
      }
    }

    only when offset == 4, the value of rtc_port_base[0] and rtc_port_base[1] will be updated
    if you read lower 32 bits first, then the value of rtc_port_base[0] and rtc_port_base[1] will not be updated!
    lower 32 bits will be read from the old value of rtc_port_base[0] and rtc_port_base[1]
    so the value of uptime->us will be wrong
    the correct implementation is to read higher 32 bits first, then lower 32 bits
    when reading higher 32 bits, the value of rtc_port_base[0] and rtc_port_base[1] will be updated
    so the value of uptime->us will be correct 
  */

  // good implementation!!!
  uint32_t h = inl(RTC_ADDR + 4);
  uint32_t l = inl(RTC_ADDR);
  uptime->us = (uint64_t)l + ((uint64_t)h << 32);

  // printf("uptime->us = %ld\n", uptime->us);
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
