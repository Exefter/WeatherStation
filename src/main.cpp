#include <Arduino.h>

#define DHT_OK 0
#define DHT_TIMEOUT_ERROR 1
#define DHT_CHECKSUM_ERROR 2

// DHT11 podlaczony do D4 = PD4
#define DHT_BIT PD4

void customDelay(uint16_t ms) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    OCR1A = 249;

    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);

    for (uint16_t i = 0; i < ms; i++) {
        while (!(TIFR1 & (1 << OCF1A)));
        TIFR1 |= (1 << OCF1A);
    }

    TCCR1B = 0;
}

bool dhtPinRead() {
    return (PIND & (1 << DHT_BIT)) != 0;
}

void dhtPinOutput() {
    DDRD |= (1 << DHT_BIT);
}

void dhtPinInput() {
    DDRD &= ~(1 << DHT_BIT);
}

void dhtPinHigh() {
    PORTD |= (1 << DHT_BIT);
}

void dhtPinLow() {
    PORTD &= ~(1 << DHT_BIT);
}

bool waitWhileState(bool state, unsigned long timeoutUs) {
    unsigned long start = micros();

    while (dhtPinRead() == state) {
        if (micros() - start > timeoutUs) {
            return false;
        }
    }
    return true;
}

int readDHT11(uint8_t *humidity, uint8_t *temperature) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // 1. Sygnał startu od MCU
    dhtPinOutput();
    dhtPinLow();
    customDelay(20);           

    dhtPinHigh();
    delayMicroseconds(40);       

    dhtPinInput();

    // 2. Odpowiedz czujnika
    unsigned long start = micros();
    while (dhtPinRead()) {
        if (micros() - start > 100) {
            return DHT_TIMEOUT_ERROR;
        }
    }

    if (!waitWhileState(false, 100)) {
        return DHT_TIMEOUT_ERROR;
    }

    if (!waitWhileState(true, 100)) {
        return DHT_TIMEOUT_ERROR;
    }

    // 3. Odczyt 40 bitow
    for (int i = 0; i < 40; i++) {
        if (!waitWhileState(false, 100)) {
            return DHT_TIMEOUT_ERROR;
        }

        unsigned long highStart = micros();
        if (!waitWhileState(true, 100)) {
            return DHT_TIMEOUT_ERROR;
        }
        unsigned long highTime = micros() - highStart;

        uint8_t bit = (highTime > 50) ? 1 : 0;

        data[i / 8] <<= 1;
        data[i / 8] |= bit;
    }

    // 4. Checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return DHT_CHECKSUM_ERROR;
    }

    *humidity = data[0];
    *temperature = data[2];

    return DHT_OK;
}

void setup() {
    Serial.begin(9600);
}

void loop() {
    uint8_t humidity = 0;
    uint8_t temperature = 0;

    int status = readDHT11(&humidity, &temperature);

    Serial.print("Status: ");
    if (status == DHT_OK) {
        Serial.println("OK");

        Serial.print("Wilgotnosc: ");
        Serial.print(humidity);
        Serial.println(" %");

        Serial.print("Temperatura: ");
        Serial.print(temperature);
        Serial.println(" C");
    } else if (status == DHT_TIMEOUT_ERROR) {
        Serial.println("TIMEOUT");
    } else if (status == DHT_CHECKSUM_ERROR) {
        Serial.println("CHECKSUM ERROR");
    } else {
        Serial.println("UNKNOWN ERROR");
    }

    Serial.println("------------------------");
    customDelay(2000);           
}