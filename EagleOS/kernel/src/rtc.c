#include <stdint.h>
#include "rtc.h"

#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT  0x71

#define RTC_SECONDS       0x00
#define RTC_MINUTES       0x02
#define RTC_HOURS         0x04
#define RTC_DAY           0x07
#define RTC_MONTH         0x08
#define RTC_YEAR          0x09
#define RTC_STATUS_A      0x0A
#define RTC_STATUS_B      0x0B
#define RTC_CENTURY       0x32

#define RTC_UIP           0x80
#define RTC_24_HOUR       0x02
#define RTC_BINARY_MODE   0x04
#define RTC_PM            0x80

#define RTC_MAX_ATTEMPTS  10
#define RTC_CENTURY_PIVOT 70

static int rtc_initialized = 0;

static inline void rtc_outb(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t rtc_inb(uint16_t port) {
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static uint8_t rtc_read_register(uint8_t reg) {
    rtc_outb(RTC_INDEX_PORT, reg);
    return rtc_inb(RTC_DATA_PORT);
}

static uint8_t rtc_bcd_to_binary(uint8_t value) {
    return (uint8_t)(((value >> 4) * 10) + (value & 0x0F));
}

static int rtc_wait_for_update(void) {
    for (uint32_t i = 0; i < 1000000; i++) {
        if (!(rtc_read_register(RTC_STATUS_A) & RTC_UIP)) {
            return 1;
        }
    }

    return 0;
}

static int rtc_read_snapshot(struct rtc_time *time, uint8_t *status_b) {
    if (!time || !status_b) {
        return 0;
    }

    if (!rtc_wait_for_update()) {
        return 0;
    }

    *status_b = rtc_read_register(RTC_STATUS_B);

    uint8_t seconds = rtc_read_register(RTC_SECONDS);
    uint8_t minutes = rtc_read_register(RTC_MINUTES);
    uint8_t hours = rtc_read_register(RTC_HOURS);
    uint8_t day = rtc_read_register(RTC_DAY);
    uint8_t month = rtc_read_register(RTC_MONTH);
    uint8_t year = rtc_read_register(RTC_YEAR);
    uint8_t century = rtc_read_register(RTC_CENTURY);

    if (!(*status_b & RTC_BINARY_MODE)) {
        seconds = rtc_bcd_to_binary(seconds);
        minutes = rtc_bcd_to_binary(minutes);
        day = rtc_bcd_to_binary(day);
        month = rtc_bcd_to_binary(month);
        year = rtc_bcd_to_binary(year);
        century = rtc_bcd_to_binary(century);
    }

    int is_pm = hours & RTC_PM;
    hours &= 0x7F;

    if (!(*status_b & RTC_BINARY_MODE)) {
        hours = rtc_bcd_to_binary(hours);
    }

    if (!(*status_b & RTC_24_HOUR)) {
        if (is_pm) {
            hours = (uint8_t)((hours % 12) + 12);
        } else {
            hours %= 12;
        }
    }

    time->second = seconds;
    time->minute = minutes;
    time->hour = hours;
    time->day = day;
    time->month = month;

    if (century >= 19 && century <= 99) {
        time->year = (uint16_t)(century * 100 + year);
    } else {
        time->year = (uint16_t)(
            year >= RTC_CENTURY_PIVOT
                ? 1900 + year
                : 2000 + year
        );
    }

    return 1;
}

static int rtc_times_equal(
    const struct rtc_time *a,
    const struct rtc_time *b
) {
    return
        a->year == b->year &&
        a->month == b->month &&
        a->day == b->day &&
        a->hour == b->hour &&
        a->minute == b->minute &&
        a->second == b->second;
}

static int rtc_validate_time(const struct rtc_time *time) {
    if (time->year < 1970 || time->year > 9999) {
        return 0;
    }

    if (time->month < 1 || time->month > 12) {
        return 0;
    }

    if (time->day < 1 || time->day > 31) {
        return 0;
    }

    if (time->hour > 23) {
        return 0;
    }

    if (time->minute > 59) {
        return 0;
    }

    if (time->second > 59) {
        return 0;
    }

    return 1;
}

int rtc_init(void) {
    struct rtc_time first;
    struct rtc_time second;

    uint8_t first_status;
    uint8_t second_status;

    for (uint32_t attempt = 0; attempt < RTC_MAX_ATTEMPTS; attempt++) {
        if (!rtc_read_snapshot(&first, &first_status)) {
            continue;
        }

        if (!rtc_read_snapshot(&second, &second_status)) {
            continue;
        }

        if (first_status != second_status) {
            continue;
        }

        if (!rtc_times_equal(&first, &second)) {
            continue;
        }

        if (!rtc_validate_time(&second)) {
            continue;
        }

        rtc_initialized = 1;
        return 1;
    }

    rtc_initialized = 0;
    return 0;
}

int rtc_get_time(struct rtc_time *time) {
    if (!rtc_initialized || !time) {
        return 0;
    }

    struct rtc_time first;
    struct rtc_time second;

    uint8_t first_status;
    uint8_t second_status;

    for (uint32_t attempt = 0; attempt < RTC_MAX_ATTEMPTS; attempt++) {
        if (!rtc_read_snapshot(&first, &first_status)) {
            continue;
        }

        if (!rtc_read_snapshot(&second, &second_status)) {
            continue;
        }

        if (first_status != second_status) {
            continue;
        }

        if (!rtc_times_equal(&first, &second)) {
            continue;
        }

        if (!rtc_validate_time(&second)) {
            continue;
        }

        *time = second;
        return 1;
    }

    return 0;
}