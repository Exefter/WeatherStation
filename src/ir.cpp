#include "ir.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint16_t irOverflowCount = 0U;
volatile uint16_t irLastTime = 0U;

volatile uint8_t irState = 0U;
volatile uint8_t irBitIndex = 0U;
volatile uint32_t irData = 0U;

volatile uint32_t irReceivedCode = 0U;
volatile uint8_t irCodeReady = 0U;

/*!
 * @brief Inicjalizuje moduł sprzętowy Timer 2 do zliczania czasu impulsów pilota IR.
 * @side effects:
 * - Zeruje rejestry konfiguracyjne TCCR2A, TCCR2B oraz licznik TCNT2.
 * - Aktywuje przerwanie od przepełnienia Timera 2 poprzez ustawienie bitu TOIE2 w TIMSK2.
 * - Uruchamia licznik z preskalerem 64, co ustawia taktowanie modułu na 250 kHz (1 takt = 4 us).
 */
static void irTimerInit(void) {
    TCCR2A = 0U;
    TCCR2B = 0U;
    TCNT2 = 0U;

    TIMSK2 = (uint8_t)(1U << TOIE2);
    TCCR2B = (uint8_t)(1U << CS22);
}

/*!
 * @brief Konfiguruje zewnętrzne przerwanie INT4 (pin PE4) do detekcji zboczy sygnału IR.
 * @side effects:
 * - Konfiguruje pin PE4 jako wejście i aktywuje jego wewnętrzny rezystor podciągający (pull-up).
 * - Modyfikuje rejestr EICRB, ustawiając wyzwalanie INT4 na dowolną zmianę stanu logicznego.
 * - Czyści flagę zaległego przerwania INTF4 i aktywuje maskę przerwania INT4 w rejestrze EIMSK.
 */
static void irInterruptInit(void) {
    DDRE &= (uint8_t)(~((uint8_t)(1U << PE4)));
    PORTE |= (uint8_t)(1U << PE4);    

    EICRB |= (uint8_t)(1U << ISC40);
    EICRB &= (uint8_t)(~((uint8_t)(1U << ISC41)));

    EIFR |= (uint8_t)(1U << INTF4); 
    EIMSK |= (uint8_t)(1U << INT4);
}

/*!
 * @brief Przelicza liczbę taktów Timera 2 (rozdzielczość 4 us) na mikrosekundy.
 * @param ticks
 * Liczba zaliczonych taktów licznika (uint16_t).
 * @returns  Czas wyrażony w mikrosekundach (uint16_t).
 * @side effects:
 * -Brak.
 */
static uint16_t ticksToUs(uint16_t ticks) {
    return (uint16_t)(ticks * (uint16_t)4U);
}

/*!
 * @brief Dokonuje pełnej inicjalizacji programowych zmiennych stanu oraz sprzętu do obsługi IR.
 * @side effects:
 * - Zeruje globalne zmienne i flagi automatu stanów odbiornika NEC.
 * - Wywołuje irTimerInit() oraz irInterruptInit(), modyfikując rejestry peryferiów AVR.
 */
void irInit(void) {
    irOverflowCount = 0U;
    irLastTime = 0U;
    irState = 0U;
    irBitIndex = 0U;
    irData = 0U;
    irReceivedCode = 0U;
    irCodeReady = 0U;

    irTimerInit();
    irInterruptInit();
}

/*!
 * @brief Sprawdza, czy w buforze znajduje się nowo odebrany i nieprzeczytany kod pilota.
 * @returns  Wartość 1U jeśli kod jest gotowy do pobrania, 0U w przeciwnym wypadku.
 * @side effects:
 * - Odczytuje stan globalnej zmiennej irCodeReady.
 */
uint8_t irHasCode(void) {
    return irCodeReady;
}

/*!
 * @brief Pobiera odebrany kod pilota i resetuje flagę gotowości.
 * @returns 32-bitowy kod odebranej ramki protokołu NEC (uint32_t).
 * @side effects:
 * - Blokuje globalne przerwania (instrukcja cli) na czas odczytu zmiennej w celu zachowania atomowości.
 * - Przywraca globalne przerwania (instrukcja sei) przed wyjściem z procedury.
 * - Zeruje flagę gotowości nowej ramki irCodeReady.
 */
uint32_t irGetCode(void) {
    uint32_t code;

    cli();
    code = irReceivedCode;
    irCodeReady = 0U;
    sei();

    return code;
}

/*!
 * @brief Procedura obsługi przerwania (ISR) od przepełnienia modułu Timer 2.
 * @side effects:
 * - Inkrementuje globalny licznik programowy irOverflowCount, rozszerzając zakres pomiarowy timera.
 */
ISR(TIMER2_OVF_vect) {
    irOverflowCount++;
}

/*!
 * @brief Procedura obsługi przerwania (ISR) zewnętrznego INT4 reagująca na zmiany stanu pinu IR.
 * @side effects:
 * - Odczytuje rejestr PINE oraz rejestr licznika TCNT2.
 * - Realizuje niskopoziomową maszynę stanów dekodera protokołu NEC w oparciu o pomiary czasu deltaUs.
 * - Modyfikuje zmienne stanu automatu, indeksy bitów oraz tymczasowy rejestr przesuwany danych.
 * - Po odebraniu pełnych 32 bitów aktualizuje rejestr irReceivedCode i podnosi flagę irCodeReady.
 */
ISR(INT4_vect) {
    uint16_t now = (uint16_t)(((uint16_t)irOverflowCount << 8U) | (uint16_t)TCNT2);
    uint16_t deltaTicks = (uint16_t)(now - irLastTime);
    irLastTime = now;

    uint16_t deltaUs = ticksToUs(deltaTicks);

    uint8_t pinState;
    if ((PINE & (uint8_t)(1U << PE4)) != 0U) {
        pinState = 1U;
    } else {
        pinState = 0U;
    }

    // Automat NEC:
    // 0 - czekamy na leader LOW ~9 ms
    // 1 - czekamy na leader HIGH ~4.5 ms
    // 2 - czekamy na LOW bitu ~560 us
    // 3 - czekamy na HIGH bitu -> 0/1

    switch (irState) {
        case 0U:
            if ((pinState == 1U) && (deltaUs > 8000U) && (deltaUs < 10000U)) {
                irState = 1U;
                irBitIndex = 0U;
                irData = 0U;
            } else {
                irState = 0U;
            }
            break;

        case 1U:
            if ((pinState == 0U) && (deltaUs > 3500U) && (deltaUs < 5500U)) {
                irState = 2U;
            } else {
                irState = 0U;
            }
            break;

        case 2U:
            if ((pinState == 1U) && (deltaUs > 300U) && (deltaUs < 900U)) {
                irState = 3U;
            } else {
                irState = 0U;
            }
            break;

        case 3U:
            if (pinState == 0U) {
                irData <<= 1U;

                if ((deltaUs > 1000U) && (deltaUs < 2000U)) {
                    irData |= 1UL;
                } else if ((deltaUs > 300U) && (deltaUs < 900U)) {
                    
                } else {
                    irState = 0U;
                    break;
                }

                irBitIndex++;

                if (irBitIndex >= 32U) {
                    irReceivedCode = irData;
                    irCodeReady = 1U;
                    irState = 0U;
                } else {
                    irState = 2U;
                }
            } else {
                irState = 0U;
            }
            break;

        default:
            irState = 0U;
            break;
    }
}