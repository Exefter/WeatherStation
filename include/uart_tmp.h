#ifndef UART_H
#define UART_H

#include <stdint.h>

void uartInit(void);
void uartSendChar(char c);
void uartSendString(const char *str);
void uartSendNumber(uint8_t value);
void uartNewLine(void);

#endif
