#include "debug_tmp.h"
#include "dht11.h"
#include "uart_tmp.h"

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
