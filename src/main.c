#include <avr/io.h>
#include "customDelay.h"

#if defined(__AVR_ATmega2560__)
    #define LED_PIN PB7  // Dioda na Mega2560
#else
    #define LED_PIN PB5  // Dioda na Uno
#endif

int main(void) {
    DDRB |= (1 << LED_PIN); 

    while (1) {
        PORTB |= (1 << LED_PIN);
        customDelay(1000);
        
        PORTB &= ~(1 << LED_PIN);
       customDelay(1000);
    }
}