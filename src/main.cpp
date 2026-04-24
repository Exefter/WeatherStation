#include "customDelay.h"
#include "dht11.h"
#include "uart_tmp.h"
#include "debug_tmp.h"

int main(void) {
    uint8_t humidity = 0;
    uint8_t temperature = 0;

    uartInit();
    uartSendString("Weather station - DHT11 test");
    uartNewLine();

    while (1) {
        gStatus = readDHT11Raw(&humidity, &temperature);

        if (gStatus == DHT_OK) {
            gHumidity = humidity;
            gTemperature = temperature;
        }

        printWeatherDataToUart(gStatus, humidity, temperature);

        customDelay(2000);
    }

    return 0;
}