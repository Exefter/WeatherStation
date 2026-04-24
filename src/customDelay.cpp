#include "customDelay.h"

void customDelay(uint16_t ms) {
    TCCR1A = 0; 
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 249; 

    TIFR1 |= (1 << OCF1A);

    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);

    for (uint16_t i = 0; i < ms; i++) {
        while (!(TIFR1 & (1 << OCF1A)));
        TIFR1 |= (1 << OCF1A);
    }

    TCCR1B = 0;
}

void customDelayUs(uint16_t us) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1B |= (1 << CS11);

    uint32_t ticks32 = (uint32_t)us * 2U;
    if (ticks32 > 0xFFFFU) {
        ticks32 = 0xFFFFU;
    }
    uint16_t ticks = (uint16_t)ticks32;

    while (TCNT1 < ticks) {
    }

    TCCR1B = 0;
}