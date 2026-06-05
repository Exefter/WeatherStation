#include "debug.h"
#include "dht11.h"
#include "lcd.h"
#include "clock.h"
#include <stddef.h>

static const char ASCII_DIGITS[10] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

/*!
 * @brief    Konwertuje liczbę na znaki ASCII i wyświetla ją na ekranie LCD.
 * @param    value  
 * Liczba z zakresu 0-255 do wyświetlenia (uint8_t).
 * @side effects:
 * - Wywołuje funkcję lcdWriteChar, modyfikując zawartość pamięci DDRAM wyświetlacza.
 */
static void lcdPrintNumber(uint8_t value) {
    char buffer[4];
    uint8_t i = 0U;
    uint8_t tempValue = value;

    if (tempValue == 0U) {
        lcdWriteChar('0');
    } else {
        while (tempValue > 0U) {
            uint8_t remainder = (uint8_t)(tempValue % 10U);
            buffer[i] = ASCII_DIGITS[remainder];
            i++;
            tempValue /= 10U;
        }

        while (i > 0U) {
            i--;
            lcdWriteChar(buffer[i]); 
        }
    }
}

/*!
 * @brief    Zwraca tekstowy opis błędu na podstawie kodu statusu czujnika DHT11.
 * @param    status  
 * Kod statusu (int).
 * @returns  Wskaźnik na ciąg znaków char zawierający opis błędu.
 * @side effects:
 * -Brak.
 */
static const char *statusText(int status) {
    const char *resultText;
    if (status == DHT_TIMEOUT_ERROR) {
        resultText = "Timeout";
    } else if (status == DHT_CHECKSUM_ERROR) {
        resultText = "Checksum error";
    } else if (status == DHT_INVALID_ARGUMENT) {
       resultText = "Bad argument";
    } else {
        resultText = "Unknown error";
    }

    return resultText;
}

/*!
 * @brief    Prezentuje odczytane dane meteorologiczne (temperatura i wilgotność) na ekranie LCD.
 * @param    status              
 * Status ostatniego odczytu z DHT11.
 * @param    humidity            
 * Wartość wilgotności powietrza.
 * @param    temperature         
 * Całkowita wartość temperatury.
 * @param    temperatureDecimal  
 * Dziesiętna wartość temperatury.
 * @side effects:
 * - Czyści ekran LCD (lcdClear) i wielokrotnie przestawia kursor oraz wysyła ciągi tekstowe.
 */
void printWeatherDataToLcd(
    uint8_t status,
    uint8_t humidity,
    uint8_t temperature,
    uint8_t temperatureDecimal) {
    lcdClear();

    if (status == DHT_OK) {
        lcdSetCursor(0U, 0U);
        lcdPrint("Temp: ");
        lcdPrintNumber(temperature);
        lcdPrint(".");
        lcdPrintNumber(temperatureDecimal);
        lcdPrint(" ");
        lcdWriteDegree();
        lcdPrint("C");

        lcdSetCursor(0U, 1U);
        lcdPrint("Hum: ");
        lcdPrintNumber(humidity);
        lcdPrint(" %");
    } else {
        lcdSetCursor(0U, 0U);
        lcdPrint("DHT11 error");

        lcdSetCursor(0U, 1U);
        lcdPrint(statusText(status));
    }
}

/*!
 * @brief    Wyświetla liczbę dwucyfrową na LCD, uzupełniając ją w razie potrzeby o wiodące zero.
 * @param    value  
 * Wartość do wyświetlenia (uint8_t).
 * @side effects:
 * - Wysyła dwa znaki ASCII bezpośrednio do pamięci DDRAM sterownika LCD.
 */
static void lcdPrint2Digits(uint8_t value) {
    uint8_t tens = (uint8_t)(value / 10U);
    uint8_t units = (uint8_t)(value % 10U);
    
    lcdWriteChar(ASCII_DIGITS[tens]);
    lcdWriteChar(ASCII_DIGITS[units]);
}

/*!
 * @brief    Wizualizuje aktualny czas z modułu RTC na wyświetlaczu LCD w formacie HH:MM:SS.
 * @param    time  
 * Wskaźnik na stałą strukturę RTC_Time zawierającą czas.
 * @side effects:
 * - Wywołuje czyszczenie ekranu oraz modyfikuje pozycję kursora na wyświetlaczu.
 */
void printTimeToLcd(const RTC_Time *time) {
    if (time != NULL) {
        lcdClear();
        lcdSetCursor(0U, 0U);
        
        lcdPrint2Digits(time->hours);
        lcdWriteChar(':');
        
        lcdPrint2Digits(time->minutes);
        lcdWriteChar(':');
        
        lcdPrint2Digits(time->seconds);
    }
}

