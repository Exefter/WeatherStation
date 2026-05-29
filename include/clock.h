#ifndef CLOCK_H
#define CLOCK_H

#include <avr/io.h>

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} RTC_Time;

void rtcReadTime(RTC_Time *time);

#endif