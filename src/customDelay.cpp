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