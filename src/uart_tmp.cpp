#include <avr/io.h>
#include "uart_tmp.h"

void uartInit(void) {
    uint16_t ubrr = 103; // 16 MHz, 9600 bps

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr & 0xFF);

    UCSR0A = 0;
    UCSR0B = (1 << TXEN0);                      // tylko nadajnik
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);    // 8 bitow danych, 1 stop, bez parzystosci
}

void uartSendChar(char c) {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = c;
}

void uartSendString(const char *str) {
    while (*str) {
        uartSendChar(*str);
        str++;
    }
}

void uartSendNumber(uint8_t value) {
    char buffer[4];
    uint8_t i = 0;

    if (value == 0) {
        uartSendChar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        uartSendChar(buffer[--i]);
    }
}

void uartNewLine(void) {
    uartSendChar('\r');
    uartSendChar('\n');
}
