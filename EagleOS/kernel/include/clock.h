#ifndef EAGLEOS_CLOCK_H
#define EAGLEOS_CLOCK_H

#include <stdint.h>

struct clock_datetime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
};

int clock_init(void);
int clock_sync_from_rtc(void);
int clock_is_synchronized(void);

uint64_t clock_get_ticks(void);
uint64_t clock_get_uptime_ms(void);
uint64_t clock_get_uptime_seconds(void);

int clock_get_datetime(struct clock_datetime *datetime);

int clock_sleep_ms(uint64_t milliseconds);

#endif