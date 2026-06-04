#include <avr/interrupt.h>
#include <stdint.h>
#include "customDelay.h"
#include "clock.h"
#include "debug.h"
#include "dht11.h"
#include "lcd.h"
#include "ir.h"

#define SCREEN_WEATHER 0
#define SCREEN_CLOCK   1

int main(void) {
    uint8_t humidity = 0;
    uint8_t temperature = 0;
    uint8_t temperatureDecimal = 0;
    RTC_Time time;
    uint8_t currentScreen = SCREEN_WEATHER;

    uint32_t irCode = 0;

    uint16_t weatherCounter = 0;
    uint16_t clockCounter = 0;
    uint16_t screenRefreshCounter = 0;

    lcdInit(0x27);
    irInit();
    sei();

    lcdSetCursor(0, 0);
    lcdPrint("Weather station");
    lcdSetCursor(0, 1);
    lcdPrint("DHT11 + RTC + IR");
    customDelay(1500);

    // pierwszy odczyt
    gStatus = readDHT11Raw(&humidity, &temperature, &temperatureDecimal);
    if (gStatus == DHT_OK) {
        gHumidity = humidity;
        gTemperature = temperature;
        gTemperatureDecimal = temperatureDecimal;
    }

    rtcReadTime(&time);

    while (1) {
        if (irHasCode()) {
            irCode = irGetCode();

            if (irCode == IR_CODE_1) {
                currentScreen = SCREEN_WEATHER;
            } else if (irCode == IR_CODE_2) {
                currentScreen = SCREEN_CLOCK;
            } else if (irCode == IR_CODE_OK) {
                currentScreen = (currentScreen == SCREEN_WEATHER) ? SCREEN_CLOCK : SCREEN_WEATHER;
            }
        }

        // DHT co 1000 ms
        if (weatherCounter >= 100) {
            weatherCounter = 0;

            gStatus = readDHT11Raw(&humidity, &temperature, &temperatureDecimal);
            if (gStatus == DHT_OK) {
                gHumidity = humidity;
                gTemperature = temperature;
                gTemperatureDecimal = temperatureDecimal;
            }
        }

        // RTC co 200 ms
        if (clockCounter >= 20) {
            clockCounter = 0;
            rtcReadTime(&time);
        }

        // ekran co 200 ms
        if (screenRefreshCounter >= 20) {
            screenRefreshCounter = 0;

            if (currentScreen == SCREEN_WEATHER) {
                printWeatherDataToLcd(gStatus, humidity, temperature, temperatureDecimal);
            } else {
                printTimeToLcd(&time);
            }
        }

        customDelay(10);
        weatherCounter++;
        clockCounter++;
        screenRefreshCounter++;
    }

    return 0;
}