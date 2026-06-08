#include <avr/io.h>
#include "dht11.h"
#include "customDelay.h"
#include <stddef.h>

#define DHT_DDR  DDRG
#define DHT_PORT PORTG
#define DHT_PIN  PING
#define DHT_BIT  PG5


volatile uint8_t gHumidity    = 0U;
volatile uint8_t gTemperature = 0U;
volatile uint8_t gTemperatureDecimal = 0U;
volatile uint8_t gStatus = (uint8_t)DHT_TIMEOUT_ERROR;

/*!
 * @brief    Odczytuje aktualny stan logiczny pinu podłączonego do czujnika DHT11.
 * @returns  Wartość logiczna: true dla stanu wysokiego, false dla stanu niskiego.
 * @side effects:
 * - Odczytuje stan rejestru wejściowego PING (DHT_PIN).
 */
static bool dhtPinRead(void) {
    bool pinState = false;
    if ((DHT_PIN & (uint8_t)(1U << DHT_BIT)) != 0U) {
        pinState = true;
    }
    return pinState;
}

/*!
 * @brief    Konfiguruje pin dedykowany dla DHT11 jako wyjście.
 * @side effects:
 * - Modyfikuje rejestr kierunku DDRG, ustawiając bit PG5.
 */
static void dhtPinOutput(void) { 
    DHT_DDR  |=  (uint8_t)(1U << DHT_BIT); 
}

/*!
 * @brief    Konfiguruje pin dedykowany dla DHT11 jako wejście (stan wysokiej impedancji).
 * @side effects:
 * - Modyfikuje rejestr kierunku DDRG, czyszcząc bit PG5.
 */
static void dhtPinInput(void)  {
    DHT_DDR &= (uint8_t)(~((uint8_t)(1U << DHT_BIT)));
}

/*!
 * @brief    Wysterowuje stan wysoki na pinie wyjściowym czujnika DHT11.
 * @side effects:
 * - Modyfikuje rejestr wyjściowy PORTG, ustawiając bit PG5.
 */
static void dhtPinHigh(void)   { 
    DHT_PORT |=  (uint8_t)(1U << DHT_BIT); 
}

/*!
 * @brief    Wysterowuje stan niski na pinie wyjściowym czujnika DHT11.
 * @side effects:
 * - Modyfikuje rejestr wyjściowy PORTG, czyszcząc bit PG5.
 */
static void dhtPinLow(void)    { 
    DHT_PORT &= (uint8_t)(~((uint8_t)(1U << DHT_BIT)));
}

/*!
 * @brief    Blokuje wykonanie programu tak długo, jak długo pin utrzymuje zadany stan, lub do przekroczenia limitu czasu.
 * @param    state         
 * Stan logiczny (true/false), dla którego pętla ma kontynuować oczekiwanie.
 * @param    timeoutTicks  
 * Maksymalna liczba taktów licznika (rozdzielczość 0.5 us), po której nastąpi przerwanie.
 * @param    ticks         
 * Wskaźnik na zmienną, do której zostanie zapisany końcowy stan licznika TCNT1 (opcjonalny).
 * @returns  true jeśli zmiana stanu nastąpiła przed limitem czasu, false w przypadku przekroczenia timeoutu.
 * @side effects:
 * - Konfiguruje i uruchamia Timer 1 z preskalerem 8 (modyfikacja rejestrów TCCR1A, TCCR1B, TCNT1).
 * - Blokuje wykonanie programu w pętli busy-wait, odpytując stan pinu GPIO oraz rejestru TCNT1.
 * - Wyłącza odliczanie Timera 1 przed opuszczeniem funkcji (zerowanie TCCR1B).
 */
static bool waitWhileState(bool state, uint16_t timeoutTicks, uint16_t *ticks) {
    bool result = true;

    TCCR1A = 0U;
    TCCR1B = 0U;
    TCNT1  = 0U;

    TCCR1B |= (uint8_t)(1U << CS11);

    while (dhtPinRead() == state) {
        if (TCNT1 > timeoutTicks) {
            TCCR1B = 0U;
            result = false;
            break;
        }
    }

    if (result == true) {
        if (ticks != NULL) {
            *ticks = TCNT1;
        }
        TCCR1B = 0U;
    }

    return result;
}

/*!
 * @brief    Realizuje niskopoziomowy protokół transmisji One-Wire w celu odczytania 40 bitów surowych danych z czujnika DHT11.
 * @param    humidity            
 * Wskaźnik na zmienną, gdzie zostanie zapisana całkowita wartość wilgotności.
 * @param    temperature         
 * Wskaźnik na zmienną, gdzie zostanie zapisana całkowita wartość temperatury.
 * @param    temperatureDecimal  
 * Wskaźnik na zmienną, gdzie zostanie zapisana dziesiętna wartość temperatury.
 * @returns  Kod błędu lub sukcesu typu int (np. DHT_OK, DHT_TIMEOUT_ERROR, DHT_CHECKSUM_ERROR, DHT_INVALID_ARGUMENT).
 * @side effects:
 * - Aktywnie przejmuje kontrolę nad pinem GPIO, wymuszając sekwencję Start (stan niski przez 20 ms).
 * - Wykorzystuje Timer 1 do dokładnego pomiaru szerokości impulsów logicznych (funkcja waitWhileState).
 * - Blokuje procesor na czas trwania pełnej ramki danych czujnika (ok. 4-6 milisekund).
 */
uint8_t readDHT11Raw(uint8_t *humidity, uint8_t *temperature, uint8_t *temperatureDecimal) {
    uint8_t statusResult = DHT_OK;

    if ((humidity == NULL) || (temperature == NULL) || (temperatureDecimal == NULL)) {
        statusResult = DHT_INVALID_ARGUMENT;
    } else {
        uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};

        dhtPinOutput();
        dhtPinLow();
        customDelay(20U);

        dhtPinHigh();
        customDelayUs(40U);

        dhtPinInput();

        if (!waitWhileState(true, 200U, NULL)) {
                statusResult = DHT_TIMEOUT_ERROR;
            } else if (!waitWhileState(false, 200U, NULL)) {
                statusResult = DHT_TIMEOUT_ERROR;
            } else if (!waitWhileState(true, 200U, NULL)) {
                statusResult = DHT_TIMEOUT_ERROR;
            } else {
                for (uint8_t i = 0U; i < 40U; i++) {
                uint16_t highTicks = 0U;

                if (!waitWhileState(false, 200U, NULL)) {
                    statusResult = DHT_TIMEOUT_ERROR;
                    break;
                }
                if (!waitWhileState(true, 200U, &highTicks)) {
                    statusResult = DHT_TIMEOUT_ERROR;
                    break;
                }          
                
                uint8_t idx = (uint8_t)(i / 8U);
                data[idx] = (uint8_t)(data[idx] << 1U);

                if (highTicks > 100U) {
                    data[idx] |= 1U;
                }
            }

            if (statusResult == DHT_OK) {
                uint8_t sum1 = (uint8_t)(data[0] + data[1]);
                uint8_t sum2 = (uint8_t)(sum1 + data[2]);
                uint8_t checksum = (uint8_t)(sum2 + data[3]);
                if (checksum != data[4]) {
                    statusResult = DHT_CHECKSUM_ERROR;
                } else {
                    *humidity           = data[0];
                    *temperature        = data[2];
                    *temperatureDecimal = data[3];
                }
            }
        }
    }
    return statusResult;
}
