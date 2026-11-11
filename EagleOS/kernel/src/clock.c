#include <stdint.h>
#include <clock.h>
#include <pit.h>
#include "rtc.h"

#define CLOCK_MILLISECONDS_PER_SECOND 1000ULL
#define CLOCK_SECONDS_PER_MINUTE 60ULL
#define CLOCK_MINUTES_PER_HOUR 60ULL
#define CLOCK_HOURS_PER_DAY 24ULL
#define CLOCK_SECONDS_PER_HOUR 3600ULL
#define CLOCK_SECONDS_PER_DAY 86400ULL

#define CLOCK_SYNC_TIMEOUT_SECONDS 3ULL
#define CLOCK_SYNC_MAX_ATTEMPTS 1000000ULL

static int clock_initialized = 0;
static int clock_synchronized = 0;

static uint64_t clock_base_seconds = 0;
static uint64_t clock_base_ticks = 0;
static uint64_t clock_uptime_base_ticks = 0;

static int clock_is_leap_year(uint32_t year) {
    if ((year % 400) == 0) {
        return 1;
    }

    if ((year % 100) == 0) {
        return 0;
    }

    return (year % 4) == 0;
}

static uint8_t clock_days_in_month(uint32_t year, uint8_t month) {
    static const uint8_t month_days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month < 1 || month > 12) {
        return 0;
    }

    if (month == 2 && clock_is_leap_year(year)) {
        return 29;
    }

    return month_days[month - 1];
}

static int clock_validate_datetime(const struct rtc_time *time) {
    if (time == 0) {
        return 0;
    }

    if (time->year < 1970) {
        return 0;
    }

    if (time->month < 1 || time->month > 12) {
        return 0;
    }

    if (time->day < 1 ||
        time->day > clock_days_in_month(time->year, time->month)) {
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

static int clock_rtc_times_equal(
    const struct rtc_time *first,
    const struct rtc_time *second
) {
    return
        first->year == second->year &&
        first->month == second->month &&
        first->day == second->day &&
        first->hour == second->hour &&
        first->minute == second->minute &&
        first->second == second->second;
}

static uint64_t clock_days_from_date(
    uint32_t year,
    uint32_t month,
    uint32_t day
) {
    year -= month <= 2;

    uint32_t era = year / 400;
    uint32_t year_of_era = year - era * 400;

    uint32_t adjusted_month =
        month > 2 ? month - 3 : month + 9;

    uint32_t day_of_year =
        (153 * adjusted_month + 2) / 5 + day - 1;

    uint32_t day_of_era =
        year_of_era * 365 +
        year_of_era / 4 -
        year_of_era / 100 +
        day_of_year;

    return (uint64_t)era * 146097 + day_of_era - 719468;
}

static uint64_t clock_datetime_to_seconds(
    const struct rtc_time *time
) {
    uint64_t days = clock_days_from_date(
        time->year,
        time->month,
        time->day
    );

    return
        days * CLOCK_SECONDS_PER_DAY +
        (uint64_t)time->hour * CLOCK_SECONDS_PER_HOUR +
        (uint64_t)time->minute * CLOCK_SECONDS_PER_MINUTE +
        time->second;
}

static int clock_seconds_to_datetime(
    uint64_t seconds,
    uint16_t milliseconds,
    struct clock_datetime *datetime
) {
    if (datetime == 0) {
        return 0;
    }

    uint64_t days = seconds / CLOCK_SECONDS_PER_DAY;
    uint64_t remaining_seconds = seconds % CLOCK_SECONDS_PER_DAY;

    int64_t shifted_days = (int64_t)days + 719468;

    int64_t era = shifted_days / 146097;

    uint32_t day_of_era =
        (uint32_t)(shifted_days - era * 146097);

    uint32_t year_of_era =
        (day_of_era -
        day_of_era / 1460 +
        day_of_era / 36524 -
        day_of_era / 146096) / 365;

    int64_t year = (int64_t)year_of_era + era * 400;

    uint32_t day_of_year =
        day_of_era -
        (365 * year_of_era +
        year_of_era / 4 -
        year_of_era / 100);

    uint32_t adjusted_month =
        (5 * day_of_year + 2) / 153;

    uint32_t day =
        day_of_year -
        (153 * adjusted_month + 2) / 5 + 1;

    uint32_t month =
        adjusted_month < 10
        ? adjusted_month + 3
        : adjusted_month - 9;

    year += month <= 2;

    if (year < 1970 || year > UINT16_MAX) {
        return 0;
    }

    datetime->year = (uint16_t)year;
    datetime->month = (uint8_t)month;
    datetime->day = (uint8_t)day;

    datetime->hour =
        (uint8_t)(remaining_seconds / CLOCK_SECONDS_PER_HOUR);

    remaining_seconds %= CLOCK_SECONDS_PER_HOUR;

    datetime->minute =
        (uint8_t)(remaining_seconds / CLOCK_SECONDS_PER_MINUTE);

    datetime->second =
        (uint8_t)(remaining_seconds % CLOCK_SECONDS_PER_MINUTE);

    datetime->millisecond = milliseconds;

    return 1;
}

int clock_init(void) {
    uint64_t frequency = pit_get_frequency();

    if (frequency == 0) {
        return 0;
    }

    clock_initialized = 1;
    clock_synchronized = 0;

    clock_base_seconds = 0;

    uint64_t current_ticks = pit_get_ticks();

    clock_base_ticks = current_ticks;
    clock_uptime_base_ticks = current_ticks;

    return 1;
}

int clock_sync_from_rtc(void) {
    if (!clock_initialized) {
        return 0;
    }

    uint64_t frequency = pit_get_frequency();

    if (frequency == 0) {
        return 0;
    }

    uint64_t flags;

    __asm__ volatile (
        "pushfq\n"
        "popq %0"
        : "=r"(flags)
    );

    int interrupts_were_enabled =
        (flags & (1ULL << 9)) != 0;

    struct rtc_time previous_time;
    struct rtc_time current_time;

    if (!rtc_get_time(&previous_time)) {
        return 0;
    }

    if (!clock_validate_datetime(&previous_time)) {
        return 0;
    }

    if (!interrupts_were_enabled) {
        __asm__ volatile ("sti" ::: "memory");
    }

    uint64_t starting_ticks = pit_get_ticks();

    uint64_t timeout_ticks =
        frequency * CLOCK_SYNC_TIMEOUT_SECONDS;

    uint64_t attempts = 0;

    int synchronized = 0;

    while (attempts < CLOCK_SYNC_MAX_ATTEMPTS) {
        attempts++;

        if (!rtc_get_time(&current_time)) {
            continue;
        }

        if (!clock_validate_datetime(&current_time)) {
            continue;
        }

        if (!clock_rtc_times_equal(
            &previous_time,
            &current_time
        )) {
            uint64_t synchronization_ticks = pit_get_ticks();

            clock_base_seconds =
                clock_datetime_to_seconds(&current_time);

            clock_base_ticks = synchronization_ticks;

            clock_synchronized = 1;

            synchronized = 1;

            break;
        }

        uint64_t current_ticks = pit_get_ticks();

        if (current_ticks - starting_ticks >= timeout_ticks) {
            break;
        }
    }

    if (!interrupts_were_enabled) {
        __asm__ volatile ("cli" ::: "memory");
    }

    return synchronized;
}

uint64_t clock_get_ticks(void) {
    if (!clock_initialized) {
        return 0;
    }

    return pit_get_ticks();
}

uint64_t clock_get_uptime_ms(void) {
    if (!clock_initialized) {
        return 0;
    }

    uint64_t frequency = pit_get_frequency();

    if (frequency == 0) {
        return 0;
    }

    uint64_t current_ticks = pit_get_ticks();

    if (current_ticks < clock_uptime_base_ticks) {
        return 0;
    }

    uint64_t elapsed_ticks =
        current_ticks - clock_uptime_base_ticks;

    uint64_t whole_seconds = elapsed_ticks / frequency;
    uint64_t remaining_ticks = elapsed_ticks % frequency;

    if (whole_seconds > UINT64_MAX / CLOCK_MILLISECONDS_PER_SECOND) {
        return UINT64_MAX;
    }

    uint64_t milliseconds =
        whole_seconds * CLOCK_MILLISECONDS_PER_SECOND;

    uint64_t remaining_ms =
        (remaining_ticks * CLOCK_MILLISECONDS_PER_SECOND) / frequency;

    if (milliseconds > UINT64_MAX - remaining_ms) {
        return UINT64_MAX;
    }

    return milliseconds + remaining_ms;
}

uint64_t clock_get_uptime_seconds(void) {
    if (!clock_initialized) {
        return 0;
    }

    uint64_t frequency = pit_get_frequency();

    if (frequency == 0) {
        return 0;
    }

    uint64_t current_ticks = pit_get_ticks();

    if (current_ticks < clock_uptime_base_ticks) {
        return 0;
    }

    return (current_ticks - clock_uptime_base_ticks) / frequency;
}

int clock_get_datetime(struct clock_datetime *datetime) {
    if (!clock_initialized ||
        !clock_synchronized ||
        datetime == 0) {
        return 0;
    }

    uint64_t frequency = pit_get_frequency();

    if (frequency == 0) {
        return 0;
    }

    uint64_t current_ticks = pit_get_ticks();

    if (current_ticks < clock_base_ticks) {
        return 0;
    }

    uint64_t elapsed_ticks =
        current_ticks - clock_base_ticks;

    uint64_t elapsed_seconds =
        elapsed_ticks / frequency;

    uint64_t remaining_ticks =
        elapsed_ticks % frequency;

    if (clock_base_seconds > UINT64_MAX - elapsed_seconds) {
        return 0;
    }

    uint64_t current_seconds =
        clock_base_seconds + elapsed_seconds;

    uint16_t milliseconds =
        (uint16_t)(
            (remaining_ticks * CLOCK_MILLISECONDS_PER_SECOND) /
            frequency
        );

    return clock_seconds_to_datetime(
        current_seconds,
        milliseconds,
        datetime
    );
}

int clock_is_synchronized(void) {
    return clock_synchronized;
}

int clock_sleep_ms(uint64_t milliseconds) {
    if (!clock_initialized) {
        return 0;
    }

    if (milliseconds == 0) {
        return 1;
    }

    uint64_t frequency = pit_get_frequency();

    if (frequency == 0) {
        return 0;
    }

    uint64_t flags;

    __asm__ volatile (
        "pushfq\n"
        "popq %0"
        : "=r"(flags)
    );

    if (!(flags & (1ULL << 9))) {
        return 0;
    }

    uint64_t whole_seconds =
        milliseconds / CLOCK_MILLISECONDS_PER_SECOND;

    uint64_t remaining_ms =
        milliseconds % CLOCK_MILLISECONDS_PER_SECOND;

    if (whole_seconds > UINT64_MAX / frequency) {
        return 0;
    }

    uint64_t required_ticks =
        whole_seconds * frequency;

    uint64_t fractional_ticks =
        (remaining_ms * frequency +
        CLOCK_MILLISECONDS_PER_SECOND - 1) /
        CLOCK_MILLISECONDS_PER_SECOND;

    if (required_ticks > UINT64_MAX - fractional_ticks) {
        return 0;
    }

    required_ticks += fractional_ticks;

    uint64_t starting_ticks = pit_get_ticks();

    while ((pit_get_ticks() - starting_ticks) < required_ticks) {
        __asm__ volatile ("hlt" ::: "memory");
    }

    return 1;
}