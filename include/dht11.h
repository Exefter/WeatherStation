#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

#define DHT_OK               (0U)
#define DHT_TIMEOUT_ERROR    (1U)
#define DHT_CHECKSUM_ERROR   (2U)
#define DHT_INVALID_ARGUMENT (3U)

extern volatile uint8_t gHumidity;
extern volatile uint8_t gTemperature;
extern volatile uint8_t gTemperatureDecimal;
extern volatile uint8_t gStatus;

uint8_t readDHT11Raw(uint8_t *humidity, uint8_t *temperature, uint8_t *temperatureDecimal);

#endif
