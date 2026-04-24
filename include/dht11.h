#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

#define DHT_OK               0
#define DHT_TIMEOUT_ERROR    1
#define DHT_CHECKSUM_ERROR   2
#define DHT_INVALID_ARGUMENT 3

extern volatile uint8_t gHumidity;
extern volatile uint8_t gTemperature;
extern volatile uint8_t gStatus;

int readDHT11Raw(uint8_t *humidity, uint8_t *temperature);

#endif
