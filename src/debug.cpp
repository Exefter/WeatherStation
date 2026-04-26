#include "debug.h"
#include "dht11.h"
#include "lcd.h"

static const uint8_t LCD_COLUMNS = 20;

static uint8_t lcdPrintText(const char *text, uint8_t written) {
    while (*text && written < LCD_COLUMNS) {
        lcdWriteChar(*text);
        text++;
        written++;
    }

    return written;
}

static uint8_t lcdPrintNumber(uint8_t value, uint8_t written) {
    char buffer[4];
    uint8_t i = 0;

    if (value == 0) {
        if (written < LCD_COLUMNS) {
            lcdWriteChar('0');
            written++;
        }
        return written;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0 && written < LCD_COLUMNS) {
        lcdWriteChar(buffer[--i]);
        written++;
    }

    return written;
}

static void lcdPadRow(uint8_t written) {
    while (written < LCD_COLUMNS) {
        lcdWriteChar(' ');
        written++;
    }
}

static void lcdPrintStatusRow(uint8_t row, const char *text) {
    uint8_t written = 0;

    lcdSetCursor(0, row);
    written = lcdPrintText(text, written);
    lcdPadRow(written);
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
    if (status == DHT_OK) {
        uint8_t written = 0;

        lcdSetCursor(0, 0);
        written = lcdPrintText("Temp: ", written);
        written = lcdPrintNumber(temperature, written);
        written = lcdPrintText(".", written);
        written = lcdPrintNumber(temperatureDecimal, written);
        written = lcdPrintText(" C", written);
        lcdPadRow(written);

        written = 0;
        lcdSetCursor(0, 1);
        written = lcdPrintText("Hum: ", written);
        written = lcdPrintNumber(humidity, written);
        written = lcdPrintText(" %", written);
        lcdPadRow(written);
    } else {
        lcdPrintStatusRow(0, "DHT11 error");
        lcdPrintStatusRow(1, statusText(status));
    }
}
