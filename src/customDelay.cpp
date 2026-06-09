#include "customDelay.h"

/*!
 * @brief Generuje blokujące opóźnienie programowe wyrażone w milisekundach.
 * @param ms
 * Liczba milisekund, o jaką ma zostać opóźnione wykonanie programu.
 * @side effects:
 * - Rekonfiguruje i przejmuje kontrolę nad Timerem 1 (zeruje rejestry TCCR1A, TCCR1B, TCNT1).
 * - Nadpisuje rejestr komparatora OCR1A wartością 249 (tryb CTC dla 1 ms przy preskalerze 64 i F_CPU 16MHz).
 * - Modyfikuje flagę OCF1A w rejestrze TIFR1 w celu jej czyszczenia.
 * - Blokuje wykonanie programu w pętli busy-wait (for/while) do momentu upłynięcia zadanego czasu.
 * - Po zakończeniu odliczania zatrzymuje Timer 1, zerując rejestr TCCR1B.
 */
void customDelay(uint16_t ms) {
    TCCR1A = 0U; 
    TCCR1B = 0U;
    TCNT1 = 0U;
    OCR1A = 249U; 

    TIFR1 = (uint8_t)(1U << OCF1A);

    TCCR1B = (uint8_t)((1U << WGM12) | (1U << CS11) | (1U << CS10));

    for (uint16_t i = 0U; i < ms; i++) {
        while ((TIFR1 & (uint8_t)(1U << OCF1A)) == 0U) {
            /* Oczekiwanie na sprzętowe ustawienie flagi OCF1A (upłynięcie 1 ms) */
        }
        TIFR1 = (uint8_t)(1U << OCF1A);
    }

    TCCR1B = 0U;
}

/*!
 * @brief Generuje blokujące opóźnienie programowe wyrażone w mikrosekundach.
 * @param us  
 * Liczba mikrosekund, o jaką ma zostać opóźnione wykonanie programu.
 * @side effects:
 * - Rekonfiguruje i przejmuje kontrolę nad Timerem 1 (zeruje rejestry TCCR1A, TCCR1B, TCNT1).
 * - Uruchamia Timer 1 z preskalerem 8 (rejestr TCCR1B), co daje rozdzielczość 0.5 us na takt przy 16MHz.
 * - Blokuje wykonanie programu w pętli busy-wait, ciągle odpytując rejestr TCNT1.
 * - Ogranicza maksymalne opóźnienie do wartości 0xFFFF taktów (nasycenie zmiennej ticks32).
 * - Po zakończeniu odliczania zatrzymuje Timer 1, zerując rejestr TCCR1B.
 */
void customDelayUs(uint16_t us) {
    TCCR1A = 0U;
    TCCR1B = 0U;
    TCNT1 = 0U;

    TCCR1B = (uint8_t)(1U << CS11);

    uint32_t ticks32 = (uint32_t)us * 2UL;
    if (ticks32 > 0xFFFFU) {
        ticks32 = 0xFFFFU;
    }
    uint16_t ticks = (uint16_t)ticks32;

    while (TCNT1 < ticks) {
        /* Oczekiwanie, aż licznik TCNT1 osiągnie zadaną liczbę taktów */
    }

    TCCR1B = 0U;
}