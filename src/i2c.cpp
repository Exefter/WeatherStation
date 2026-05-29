#include "i2c.h"
#include <avr/io.h>

#define I2C_BIT_RATE  72  // 16 MHz / (16 + 2*72*1) ≈ 100 kHz - standardowa prędkość I2C

void i2cInit(void) {
    TWSR = 0;
    TWBR = I2C_BIT_RATE; // ustawienie szybkości I2C
    TWCR = (1 << TWEN); // włącz I2C
}

void i2cStart(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // wysłanie sygnału start
    while (!(TWCR & (1 << TWINT))); // czekaj na zakończenie
}

void i2cStop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // wysłanie sygnału stop
    while (TWCR & (1 << TWSTO)); // czekaj aż stop zostanie wysłany
}

void i2cWrite(uint8_t data) {
    TWDR = data; // załaduj bajt do rejestru danych
    TWCR = (1 << TWINT) | (1 << TWEN); // rozpocznij transmisję
    while (!(TWCR & (1 << TWINT))); // czekaj na zakończenie
}

uint8_t i2cRead(uint8_t ack) {
    // Jeśli ack == 1, wysyłamy ACK (chcemy więcej bajtów)
    // Jeśli ack == 0, wysyłamy NACK (to ostatni bajt, kończymy)
    TWCR = (1 << TWINT) | (1 << TWEN) | (ack ? (1 << TWEA) : 0);
    while (!(TWCR & (1 << TWINT))); // czekaj na zakończenie odbioru
    return TWDR; // zwróć odebrany bajt
}

