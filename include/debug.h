#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

void printWeatherDataToLcd(
    int status,
    uint8_t humidity,
    uint8_t temperature,
    uint8_t temperatureDecimal);

#endif
