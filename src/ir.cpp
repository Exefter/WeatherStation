#include "ir.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// UNO: D2 = PD2 = INT0

volatile uint16_t irOverflowCount = 0;
volatile uint16_t irLastTime = 0;

volatile uint8_t irState = 0;
volatile uint8_t irBitIndex = 0;
volatile uint32_t irData = 0;

volatile uint32_t irReceivedCode = 0;
volatile uint8_t irCodeReady = 0;

// -------------------------------------
// Timer2: preskaler 64
// 16 MHz / 64 = 250 kHz
// 1 tick = 4 us
// overflow co 256 * 4 us = 1024 us
// -------------------------------------
static void irTimerInit(void) {
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2 = 0;

    TIMSK2 = (1 << TOIE2);
    TCCR2B = (1 << CS22);   // preskaler 64
}

// -------------------------------------
// INT0 na dowolne zbocze
// -------------------------------------
static void irInterruptInit(void) {
    DDRD &= ~(1 << PD2);    // D2 jako wejscie
    PORTD |= (1 << PD2);    // pull-up

    EICRA |= (1 << ISC00);  // any logical change on INT0
    EICRA &= ~(1 << ISC01);

    EIFR |= (1 << INTF0);   // wyczysc flage
    EIMSK |= (1 << INT0);   // wlacz INT0
}

static uint16_t ticksToUs(uint16_t ticks) {
    return ticks * 4;
}

void irInit(void) {
    irOverflowCount = 0;
    irLastTime = 0;
    irState = 0;
    irBitIndex = 0;
    irData = 0;
    irReceivedCode = 0;
    irCodeReady = 0;

    irTimerInit();
    irInterruptInit();
}

uint8_t irHasCode(void) {
    return irCodeReady;
}

uint32_t irGetCode(void) {
    uint32_t code;

    cli();
    code = irReceivedCode;
    irCodeReady = 0;
    sei();

    return code;
}

ISR(TIMER2_OVF_vect) {
    irOverflowCount++;
}

ISR(INT0_vect) {
    uint16_t now = ((uint16_t)irOverflowCount << 8) | TCNT2;
    uint16_t deltaTicks = now - irLastTime;
    irLastTime = now;

    uint16_t deltaUs = ticksToUs(deltaTicks);

    uint8_t pinState = (PIND & (1 << PD2)) ? 1 : 0;

    // Automat NEC:
    // 0 - czekamy na leader LOW ~9 ms
    // 1 - czekamy na leader HIGH ~4.5 ms
    // 2 - czekamy na LOW bitu ~560 us
    // 3 - czekamy na HIGH bitu -> 0/1

    switch (irState) {
        case 0:
            // zakonczyl sie leader LOW, linia przeszla na HIGH
            if (pinState == 1 && deltaUs > 8000 && deltaUs < 10000) {
                irState = 1;
                irBitIndex = 0;
                irData = 0;
            } else {
                irState = 0;
            }
            break;

        case 1:
            // zakonczyl sie leader HIGH, linia przeszla na LOW
            if (pinState == 0 && deltaUs > 3500 && deltaUs < 5500) {
                irState = 2;
            } else {
                irState = 0;
            }
            break;

        case 2:
            // zakonczyl sie LOW pojedynczego bitu (~560 us), linia idzie na HIGH
            if (pinState == 1 && deltaUs > 300 && deltaUs < 900) {
                irState = 3;
            } else {
                irState = 0;
            }
            break;

        case 3:
            // zakonczyl sie HIGH bitu, linia idzie na LOW
            if (pinState == 0) {
                irData <<= 1;

                if (deltaUs > 1000 && deltaUs < 2000) {
                    // bit 1
                    irData |= 1;
                } else if (deltaUs > 300 && deltaUs < 900) {
                    // bit 0
                } else {
                    irState = 0;
                    break;
                }

                irBitIndex++;

                if (irBitIndex >= 32) {
                    irReceivedCode = irData;
                    irCodeReady = 1;
                    irState = 0;
                } else {
                    irState = 2;
                }
            } else {
                irState = 0;
            }
            break;

        default:
            irState = 0;
            break;
    }
}