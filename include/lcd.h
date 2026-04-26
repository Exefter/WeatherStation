#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void lcdInit(uint8_t address);
void lcdClear(void);
void lcdSetCursor(uint8_t col, uint8_t row);
void lcdWriteChar(char c);
void lcdPrint(const char *text);

#endif
