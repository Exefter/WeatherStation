#include <avr/interrupt.h>
#include <stdint.h>
#include "customDelay.h"
#include "clock.h"
#include "debug.h"
#include "dht11.h"
#include "lcd.h"
#include "ir.h"

#define SCREEN_WEATHER 0U
#define SCREEN_CLOCK   1U

/*!
 * @brief    Główny punkt wejściowy programu stacji pogodowej (funkcja main).
 * @returns  Kod zakończenia programu (int), domyślnie 0.
 * @side effects:
 * - Konfiguruje globalne peryferia sprzętowe: LCD oraz dekoder podczerwieni IR.
 * - Odblokowuje globalną flagę obsługi przerwań mikrokontrolera (instrukcja sei).
 * - Zarządza czasem odpytywania czujników (DHT11, RTC) oraz odświeżaniem ekranu.
 * - Blokuje procesor w nieskończonej pętli przetwarzania zadań.
 */
int main(void) {
    uint8_t humidity = 0U;
    uint8_t temperature = 0U;
    uint8_t temperatureDecimal = 0U;
    RTC_Time time;
    uint8_t currentScreen = SCREEN_WEATHER;

    uint32_t irCode = 0UL;

    uint16_t weatherCounter = 0U;
    uint16_t clockCounter = 0U;
    uint16_t screenRefreshCounter = 0U;
    bool isRunning = true;

    lcdInit(0x27U);
    irInit();
    sei();

    lcdSetCursor(0U, 0U);
    lcdPrint("Weather station");
    lcdSetCursor(0U, 1U);
    lcdPrint("DHT11 + RTC + IR");
    customDelay(1500U);

    gStatus = readDHT11Raw(&humidity, &temperature, &temperatureDecimal);
    if (gStatus == DHT_OK) {
        gHumidity = humidity;
        gTemperature = temperature;
        gTemperatureDecimal = temperatureDecimal;
    }

    rtcReadTime(&time);

    while (isRunning) {
        if (irHasCode() != 0U) {
            irCode = irGetCode();

            if (irCode == IR_CODE_1) {
                currentScreen = SCREEN_WEATHER;
            } else if (irCode == IR_CODE_2) {
                currentScreen = SCREEN_CLOCK;
            } else if (irCode == IR_CODE_OK) {
                if (currentScreen == SCREEN_WEATHER) {
                    currentScreen = SCREEN_CLOCK;
                } else {
                    currentScreen = SCREEN_WEATHER;
                }
            }
            else {

            }
        }

        if (weatherCounter >= 100U) {
            weatherCounter = 0U;

            gStatus = readDHT11Raw(&humidity, &temperature, &temperatureDecimal);
            if (gStatus == DHT_OK) {
                gHumidity = humidity;
                gTemperature = temperature;
                gTemperatureDecimal = temperatureDecimal;
            }
        }

        if (clockCounter >= 20U) {
            clockCounter = 0U;
            rtcReadTime(&time);
        }

        if (screenRefreshCounter >= 20U) {
            screenRefreshCounter = 0U;

            if (currentScreen == SCREEN_WEATHER) {
                printWeatherDataToLcd(gStatus, humidity, temperature, temperatureDecimal);
            } else {
                printTimeToLcd(&time);
            }
        }

        customDelay(10U);
        weatherCounter++;
        clockCounter++;
        screenRefreshCounter++;
    }

    return 0;
}