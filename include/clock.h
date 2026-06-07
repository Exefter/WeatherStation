#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

#define RTC_OK       (0U)
#define RTC_ERROR    (1U)

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t year;
    uint8_t month;
    uint8_t day;
} RTC_Time_Date;

uint8_t rtcReadTime(RTC_Time_Date *time);

#endif