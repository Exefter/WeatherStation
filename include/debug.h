#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include "clock.h"

void printWeatherDataToLcd(
    uint8_t status,
    uint8_t humidity,
    uint8_t temperature,
    uint8_t temperatureDecimal);

void printTimeToLcd(const RTC_Time *time);

#endif
