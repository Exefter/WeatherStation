#include "debug.h"
#include "dht11.h"
#include "lcd.h"

static void lcdPrintNumber(uint8_t value) {
    char buffer[4];
    uint8_t i = 0;

    if (value == 0) {
        lcdWriteChar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        lcdWriteChar(buffer[--i]); 
    }
}

static const char *statusText(int status) {
    if (status == DHT_TIMEOUT_ERROR) {
        return "Timeout";
    } else if (status == DHT_CHECKSUM_ERROR) {
        return "Checksum error";
    } else if (status == DHT_INVALID_ARGUMENT) {
        return "Bad argument";
    }

    return "Unknown error";
}

void printWeatherDataToLcd(
    int status,
    uint8_t humidity,
    uint8_t temperature,
    uint8_t temperatureDecimal) {
    lcdClear();

    if (status == DHT_OK) {
        lcdSetCursor(0, 0);
        lcdPrint("Temp: ");
        lcdPrintNumber(temperature);
        lcdPrint(".");
        lcdPrintNumber(temperatureDecimal);
        lcdPrint(" ");
        lcdWriteDegree();
        lcdPrint("C");

        lcdSetCursor(0, 1);
        lcdPrint("Hum: ");
        lcdPrintNumber(humidity);
        lcdPrint(" %");
    } else {
        lcdSetCursor(0, 0);
        lcdPrint("DHT11 error");

        lcdSetCursor(0, 1);
        lcdPrint(statusText(status));
    }
}
