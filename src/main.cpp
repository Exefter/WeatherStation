#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

#define F_CPU 16000000UL

#define DHT_OK 0
#define DHT_TIMEOUT_ERROR 1
#define DHT_CHECKSUM_ERROR 2
#define DHT_INVALID_ARGUMENT 3

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

volatile uint8_t gHumidity = 0;
volatile uint8_t gTemperature = 0;
volatile uint8_t gStatus = DHT_TIMEOUT_ERROR;

// =====================================
// UART - 9600 bps, 8N1
// =====================================
void uartInit(void) {
    uint16_t ubrr = 103; // 16 MHz, 9600 bps

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr & 0xFF);

    UCSR0A = 0;
    UCSR0B = (1 << TXEN0);                      // tylko nadajnik
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);    // 8 bitow danych, 1 stop, bez parzystosci
}

void uartSendChar(char c) {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = c;
}

void uartSendString(const char *str) {
    while (*str) {
        uartSendChar(*str);
        str++;
    }
}

void uartSendNumber(uint8_t value) {
    char buffer[4];
    uint8_t i = 0;

    if (value == 0) {
        uartSendChar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        uartSendChar(buffer[--i]);
    }
}

void uartNewLine(void) {
    uartSendChar('\r');
    uartSendChar('\n');
}

// =====================================
// Wlasny Delay 
// =====================================
void customDelay(uint16_t ms) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    OCR1A = 249;

    // CTC, preskaler 64
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);

    for (uint16_t i = 0; i < ms; i++) {
        while (!(TIFR1 & (1 << OCF1A))) {
        }
        TIFR1 |= (1 << OCF1A);
    }

    TCCR1B = 0;
}

// =====================================
// Krotkie opoznienie w us na Timer1
// 16 MHz, preskaler 8 => 0.5 us / tick
// =====================================
void customDelayUs(uint16_t us) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1B |= (1 << CS11); // preskaler 8

    uint16_t ticks = us * 2;

    while (TCNT1 < ticks) {
    }

    TCCR1B = 0;
}

// =====================================
// GPIO DHT11
// =====================================
bool dhtPinRead(void) {
    return (DHT_PIN & (1 << DHT_BIT)) != 0;
}

void dhtPinOutput(void) {
    DHT_DDR |= (1 << DHT_BIT);
}

void dhtPinInput(void) {
    DHT_DDR &= ~(1 << DHT_BIT);
}

void dhtPinHigh(void) {
    DHT_PORT |= (1 << DHT_BIT);
}

void dhtPinLow(void) {
    DHT_PORT &= ~(1 << DHT_BIT);
}

// =====================================
// Timeout na Timer1
// 1 tick = 0.5 us przy preskalerze 8
// =====================================
bool waitWhileState(bool state, uint16_t timeoutTicks) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1B |= (1 << CS11); // preskaler 8

    while (dhtPinRead() == state) {
        if (TCNT1 > timeoutTicks) {
            TCCR1B = 0;
            return false;
        }
    }

    TCCR1B = 0;
    return true;
}

bool measureHighPulse(uint16_t *pulseTicks, uint16_t timeoutTicks) {
    if (pulseTicks == 0) {
        return false;
    }

    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1B |= (1 << CS11); // preskaler 8

    while (dhtPinRead()) {
        if (TCNT1 > timeoutTicks) {
            TCCR1B = 0;
            return false;
        }
    }

    *pulseTicks = TCNT1;
    TCCR1B = 0;
    return true;
}

// =====================================
// Odczyt surowy DHT11
// =====================================
int readDHT11Raw(uint8_t *humidity, uint8_t *temperature) {
    if (humidity == 0 || temperature == 0) {
        return DHT_INVALID_ARGUMENT;
    }

    uint8_t data[5] = {0, 0, 0, 0, 0};

    dhtPinOutput();
    dhtPinLow();
    customDelay(20);

    dhtPinHigh();
    customDelayUs(40);

    dhtPinInput();

    // odpowiedz czujnika
    if (!waitWhileState(true, 200)) {
        return DHT_TIMEOUT_ERROR;
    }

    if (!waitWhileState(false, 200)) {
        return DHT_TIMEOUT_ERROR;
    }

    if (!waitWhileState(true, 200)) {
        return DHT_TIMEOUT_ERROR;
    }

    // 40 bitow
    for (uint8_t i = 0; i < 40; i++) {
        uint16_t highTicks = 0;

        if (!waitWhileState(false, 200)) {
            return DHT_TIMEOUT_ERROR;
        }

        if (!measureHighPulse(&highTicks, 200)) {
            return DHT_TIMEOUT_ERROR;
        }

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

    *humidity = data[0];
    *temperature = data[2];

    return DHT_OK;
}

int getWeatherData(uint8_t *humidity, uint8_t *temperature) {
    return readDHT11Raw(humidity, temperature);
}

// =====================================
// Debug UART
// =====================================
void printWeatherDataToUart(int status, uint8_t humidity, uint8_t temperature) {
    uartSendString("Status: ");

    if (status == DHT_OK) {
        uartSendString("OK");
        uartNewLine();

        uartSendString("Wilgotnosc: ");
        uartSendNumber(humidity);
        uartSendString(" %");
        uartNewLine();

        uartSendString("Temperatura: ");
        uartSendNumber(temperature);
        uartSendString(" C");
        uartNewLine();
    } else if (status == DHT_TIMEOUT_ERROR) {
        uartSendString("TIMEOUT");
        uartNewLine();
    } else if (status == DHT_CHECKSUM_ERROR) {
        uartSendString("CHECKSUM ERROR");
        uartNewLine();
    } else if (status == DHT_INVALID_ARGUMENT) {
        uartSendString("INVALID ARGUMENT");
        uartNewLine();
    } else {
        uartSendString("UNKNOWN ERROR");
        uartNewLine();
    }

    uartSendString("------------------------");
    uartNewLine();
}

int main(void) {
    uint8_t humidity = 0;
    uint8_t temperature = 0;

    uartInit();
    uartSendString("Weather station - DHT11 test");
    uartNewLine();

    while (1) {
        gStatus = getWeatherData(&humidity, &temperature);

        if (gStatus == DHT_OK) {
            gHumidity = humidity;
            gTemperature = temperature;
        }

        printWeatherDataToUart(gStatus, humidity, temperature);

        customDelay(2000);
    }

    return 0;
}