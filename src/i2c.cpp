#include "i2c.h"
#include <avr/io.h>

#define I2C_BIT_RATE  72U

/*!
 * @brief    Inicjalizuje moduł sprzętowy TWI (I2C) mikrokontrolera do pracy w trybie Master.
 * @side effects:
 * - Zeruje rejestr statusowy TWSR (ustawia preskaler TWI na wartość 1).
 * - Modyfikuje rejestr TWBR, ustawiając prędkość transmisji na około 100 kHz (przy F_CPU = 16 MHz).
 * - Włącza interfejs I2C poprzez ustawienie bitu TWEN w rejestrze TWCR.
 */
void i2cInit(void) {
    TWSR = 0U;
    TWBR = I2C_BIT_RATE;
    TWCR = (uint8_t)(1U << TWEN);
}

/*!
 * @brief    Generuje sygnał START na magistrali I2C i oczekuje na zakończenie operacji.
 * @side effects:
 * - Modyfikuje rejestr TWCR, wymuszając nadanie sekwencji START (bit TWSTA) oraz czyszcząc flagę TWINT.
 * - Blokuje wykonanie programu w pętli busy-wait do momentu sprzętowego ustawienia flagi TWINT przez moduł TWI.
 * - Przejmuje kontrolę nad liniami SDA i SCL, blokując magistralę dla innych urządzeń.
 */
void i2cStart(void) {
    TWCR = (uint8_t)((1U << TWINT) | (1U << TWSTA) | (1U << TWEN));
    while ((TWCR & (uint8_t)(1U << TWINT)) == 0U) {
        /* Oczekiwanie na sprzętowe ustawienie flagi TWINT */
    }
}

/*!
 * @brief    Generuje sygnał STOP na magistrali I2C i oczekuje na zwolnienie linii.
 * @side effects:
 * - Modyfikuje rejestr TWCR, wymuszając nadanie sekwencji STOP (bit TWSTO) oraz czyszcząc flagę TWINT.
 * - Blokuje wykonanie programu w pętli busy-wait do momentu, gdy sprzęt automatycznie wyczyści bit TWSTO (oznacza to fizyczne wysłanie stanu STOP).
 * - Zwalnia linie magistrali I2C (SDA i SCL) do stanu wysokiego.
 */
void i2cStop(void) {
    TWCR =  (uint8_t)((1U << TWINT) | (1U << TWSTO) | (1U << TWEN));
    while ((TWCR & (uint8_t)(1U << TWSTO)) != 0U) {
        /* Oczekiwanie na sprzętowe wyczyszczenie bitu TWSTO */
    }
}

/*!
 * @brief    Wysyła jeden bajt danych na magistralę I2C i oczekuje na zakończenie transmisji.
 * @param    data  
 * Bajt danych (np. adres urządzenia lub rejestru, albo dane użytkownika), który ma zostać nadany.
 * @side effects:
 * - Zapisuje przesyłany bajt do rejestru danych TWDR, nadpisując jego poprzednią zawartość.
 * - Modyfikuje rejestr TWCR w celu rozpoczęcia transmisji (czyszczenie flagi TWINT).
 * - Blokuje wykonanie programu w pętli busy-wait do momentu sprzętowego ustawienia flagi TWINT (koniec nadawania i odebranie bitu ACK/NACK).
 */
void i2cWrite(uint8_t data) {
    TWDR = data;
    TWCR =  (uint8_t)((1U << TWINT) | (1U << TWEN));

    while ((TWCR & (uint8_t)(1U << TWINT)) == 0U) {
        /* Oczekiwanie na sprzętowe ustawienie flagi TWINT */
    }
}

/*!
 * @brief    Odczytuje jeden bajt danych z magistrali I2C z wysłaniem sygnału ACK lub NACK.
 * @param    ack
 * Decyzja o kontynuacji transmisji: 
 * - wartość niezerowa (true) wysyła ACK, informując, że będą czytane kolejne dane.
 * - wartość zero (false) wysyła NACK, informując, że jest to ostatni odczytywany bajt (koniec transmisji).
 * @returns  Odczytany z magistrali bajt danych (uint8_t).
 * @side effects:
 * - Modyfikuje rejestr TWCR, ustawiając bit TWEA (jeśli param. ack jest prawdziwy) oraz czyści flagę TWINT, co wyzwala procedurę odbioru.
 * - Blokuje wykonanie programu w pętli busy-wait do momentu sprzętowego ustawienia flagi TWINT (koniec odbioru bajtu).
 * - Odczytuje i zwraca zawartość rejestru danych TWDR.
 */
uint8_t i2cRead(uint8_t ack) {
    if (ack != 0U) {
        TWCR = (uint8_t)((1U << TWINT) | (1U << TWEN) | (1U << TWEA));
    } else {
        TWCR = (uint8_t)((1U << TWINT) | (1U << TWEN));
    }
    while ((TWCR & (uint8_t)(1U << TWINT)) == 0U) {
        /* Oczekiwanie na sprzętowe ustawienie flagi TWINT */
    }
    return TWDR;
}

