#include "customDelay.h"
#include "dht11.h"
#include "debug.h"
#include "lcd.h"

int main(void) {
    uint8_t humidity = 0;
    uint8_t temperature = 0;
    uint8_t temperatureDecimal = 0;

    lcdInit(0x27);

    lcdSetCursor(0, 0);
    lcdPrint("Weather station");
    lcdSetCursor(0, 1);
    lcdPrint("DHT11 + LCD");
    customDelay(1500);

    while (1) {
        gStatus = readDHT11Raw(&humidity, &temperature, &temperatureDecimal);

        if (gStatus == DHT_OK) {
            gHumidity = humidity;
            gTemperature = temperature;
            gTemperatureDecimal = temperatureDecimal;
        }

        printWeatherDataToLcd(gStatus, humidity, temperature, temperatureDecimal);

        customDelay(2000);
    }

    return 0;
}
