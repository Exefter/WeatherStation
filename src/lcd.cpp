#include <avr/io.h>
#include "customDelay.h"
#include "lcd.h"

static const uint8_t LCD_RS = (1 << 0);
static const uint8_t LCD_EN = (1 << 2);
static const uint8_t LCD_BL = (1 << 3);
static const uint8_t ROW_OFFSETS[4] = {0x00, 0x40, 0x14, 0x54};

static uint8_t lcdAddress = 0x27;

static void i2cInit(void) {
    TWSR = 0;
    TWBR = 72;
    TWCR = (1 << TWEN);
}

static void i2cStart(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))) {
    }
}

static void i2cStop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    while (TWCR & (1 << TWSTO)) {
    }
}

static void i2cWrite(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))) {
    }
}

static void pcfWrite(uint8_t data) {
    i2cStart();
    i2cWrite((lcdAddress << 1) | 0);
    i2cWrite(data | LCD_BL);
    i2cStop();
}

static void lcdPulse(uint8_t data) {
    pcfWrite(data | LCD_EN);
    customDelayUs(1);
    pcfWrite(data & ~LCD_EN);
    customDelayUs(50);
}

static void lcdWrite4(uint8_t data, uint8_t mode) {
    lcdPulse((data & 0xF0) | mode);
}

static void lcdSend(uint8_t data, uint8_t mode) {
    lcdWrite4(data, mode);
    lcdWrite4(data << 4, mode);
}

static void lcdCommand(uint8_t command) {
    lcdSend(command, 0);
}

void lcdInit(uint8_t address) {
    lcdAddress = address;

    i2cInit();
    customDelay(50);
    pcfWrite(0);
    customDelay(10);

    lcdWrite4(0x30, 0);
    customDelay(5);
    lcdWrite4(0x30, 0);
    customDelayUs(150);
    lcdWrite4(0x30, 0);
    customDelayUs(150);
    lcdWrite4(0x20, 0);
    customDelayUs(150);

    lcdCommand(0x28);
    lcdCommand(0x08);
    lcdClear();
    lcdCommand(0x06);
    lcdCommand(0x0C);
}

void lcdClear(void) {
    lcdCommand(0x01);
    customDelay(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
    lcdCommand(0x80 | (col + ROW_OFFSETS[row % 4]));
}

void lcdWriteChar(char c) {
    lcdSend((uint8_t)c, LCD_RS);
}

void lcdPrint(const char *text) {
    while (*text) {
        lcdWriteChar(*text);
        text++;
    }
}
