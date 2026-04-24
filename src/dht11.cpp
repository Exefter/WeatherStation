#include <avr/io.h>
#include <stdbool.h>
#include "dht11.h"
#include "customDelay.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// DHT11 podlaczony do D4
// Uno  (ATmega328P):   D4 = PD4 (PORTD)
// Mega (ATmega2560):   D4 = PG5 (PORTG)
#if defined(__AVR_ATmega2560__)
    #define DHT_DDR  DDRG
    #define DHT_PORT PORTG
    #define DHT_PIN  PING
    #define DHT_BIT  PG5
#else
    #define DHT_DDR  DDRD
    #define DHT_PORT PORTD
    #define DHT_PIN  PIND
    #define DHT_BIT  PD4
#endif

volatile uint8_t gHumidity    = 0;
volatile uint8_t gTemperature = 0;
volatile uint8_t gStatus      = DHT_TIMEOUT_ERROR;

static bool dhtPinRead(void) {
    return (DHT_PIN & (1 << DHT_BIT)) != 0;
}

static void dhtPinOutput(void) { 
    DHT_DDR  |=  (1 << DHT_BIT); 
}
static void dhtPinInput(void)  {
     DHT_DDR  &= ~(1 << DHT_BIT); 
    }
static void dhtPinHigh(void)   { 
    DHT_PORT |=  (1 << DHT_BIT); 
}
static void dhtPinLow(void)    { 
    DHT_PORT &= ~(1 << DHT_BIT); 
}

// 1 tick = 0.5 us przy preskalerze 8
static bool waitWhileState(bool state, uint16_t timeoutTicks, uint16_t *ticks) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    TCCR1B |= (1 << CS11); // preskaler 8

    while (dhtPinRead() == state) {
        if (TCNT1 > timeoutTicks) {
            TCCR1B = 0;
            return false;
        }
    }

    if (ticks) {
        *ticks = TCNT1;
    }
    TCCR1B = 0;
    return true;
}

int readDHT11Raw(uint8_t *humidity, uint8_t *temperature) {
    if (humidity == 0 || temperature == 0) {
        return DHT_INVALID_ARGUMENT;
    }

    uint8_t data[5] = {0};

    dhtPinOutput();
    dhtPinLow();
    customDelay(20);

    dhtPinHigh();
    customDelayUs(40);

    dhtPinInput();

    // odpowiedz czujnika
    if (!waitWhileState(true,  200, 0)) 
        return DHT_TIMEOUT_ERROR;
    if (!waitWhileState(false, 200, 0)) 
        return DHT_TIMEOUT_ERROR;
    if (!waitWhileState(true,  200, 0)) 
        return DHT_TIMEOUT_ERROR;

    // 40 bitow
    for (uint8_t i = 0; i < 40; i++) {
        uint16_t highTicks = 0;

        if (!waitWhileState(false, 200, 0))          
            return DHT_TIMEOUT_ERROR;
        if (!waitWhileState(true,  200, &highTicks)) 
            return DHT_TIMEOUT_ERROR;

        data[i / 8] <<= 1;

        // prog ok. 50 us => 100 tickow
        if (highTicks > 100) {
            data[i / 8] |= 1;
        }
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return DHT_CHECKSUM_ERROR;
    }

    *humidity    = data[0];
    *temperature = data[2];

    return DHT_OK;
}
