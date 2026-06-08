#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define I2C_BIT_RATE  72U  // 16 MHz / (16 + 2*72*1) ≈ 100 kHz - standardowa prędkość I2C
#define I2C_OK       (0U)
#define I2C_ERROR    (1U)

void i2cInit(void);
void i2cStart(void);
void i2cStop(void);
uint8_t i2cWrite(uint8_t data);
uint8_t i2cRead(uint8_t ack);

#endif