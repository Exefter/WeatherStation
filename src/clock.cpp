#include "clock.h"
#include "i2c.h"
#include <avr/io.h>
#include <stddef.h>

#define DS3231_ADDRESS_WRITE 0xD0U // Adres 0x68 przesunięty w lewo o 1 bit (bit zapisu = 0)
#define DS3231_ADDRESS_READ  0xD1U // Adres 0x68 przesunięty w lewo o 1 bit (bit odczytu = 1)

/*! 
 * @brief    Konwertuje liczbę zapisaną w formacie BCD (Binary-Coded Decimal) na format dziesiętny.
 * @param    val
 * Wartość wejściowa w formacie BCD (uint8_t).
 * @returns  Przekonwertowana wartość w formacie dziesiętnym (uint8_t).
 * @side effects:
 * -Brak.
 */
static uint8_t bcdToDec(uint8_t val) {
    return ((uint8_t)((val / 16U * 10U) + (val % 16U)));
}

/*!
 * @brief    Odczytuje aktualny czas (godziny, minuty, sekundy) z zewnętrznego układu RTC.
 * @param    time
 * Wskaźnik do struktury RTC_Time_Date, do której zostaną zapisane odczytane dane.
 * @returns  Kod błędu lub sukcesu (np. RTC_OK, RTC_ERROR).
 * @side effects:
 * - Generuje sygnały START i STOP na magistrali I2C (modyfikacja rejestrów sprzętowych TWI).
 * - Blokuje wykonanie programu (pętle busy-wait wewnątrz funkcji niskopoziomowych I2C).
 * - W przypadku braku wykrycia układu (błąd zapisu adresu), wysyła sygnał STOP w celu odblokowania szyny dla innych urządzeń (np. LCD).
 * - Nadpisuje pola struktury wskazywanej przez parametr 'time' wyłącznie w przypadku sukcesu transmisji.
 */
uint8_t rtcReadTime(RTC_Time_Date *time) {
    uint8_t status = RTC_ERROR;
    if (time != NULL) {
        i2cStart();
        status = i2cWrite(DS3231_ADDRESS_WRITE);
        if (status == RTC_OK) {
            i2cWrite(0x00U); 
            
            i2cStart();
            i2cWrite(DS3231_ADDRESS_READ);
            
            uint8_t rawSeconds = i2cRead((uint8_t)1U);
            uint8_t rawMinutes = i2cRead((uint8_t)1U);
            uint8_t rawHours   = i2cRead((uint8_t)1U);
            i2cRead((uint8_t)1U);
            uint8_t rawDay = i2cRead((uint8_t)1U);
            uint8_t rawMonth = i2cRead((uint8_t)1U);
            uint8_t rawYear = i2cRead((uint8_t)0U);
            
            i2cStop();
            
            time->seconds = bcdToDec(rawSeconds);
            time->minutes = bcdToDec(rawMinutes);
            time->hours   = bcdToDec((uint8_t)(rawHours & 0x3FU));
            time->day     = bcdToDec(rawDay); 
            time->month   = bcdToDec((uint8_t)(rawMonth & 0x1FU));
            time->year   = bcdToDec(rawYear);
            status = RTC_OK;
        } else {
            i2cStop();
            status = RTC_ERROR;
        }
    }
    return status;
}

