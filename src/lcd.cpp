#include <avr/io.h>
#include "customDelay.h"
#include "lcd.h"

static const uint8_t I2C_WRITE = 0;
static const uint8_t I2C_BIT_RATE = 72;

static const uint8_t LCD_COMMAND = 0;
static const uint8_t LCD_CHARACTER = (1 << 0);
static const uint8_t LCD_ENABLE = (1 << 2);
static const uint8_t LCD_BACKLIGHT = (1 << 3);

static const uint8_t LCD_RESET_TO_8_BIT_MODE = 0x30;
static const uint8_t LCD_CLEAR = 0x01;
static const uint8_t LCD_HOME_LINE = 0x80;
static const uint8_t LCD_4_BIT_MODE = 0x20;
static const uint8_t LCD_2_LINES_5X8_FONT = 0x28;
static const uint8_t LCD_DISPLAY_OFF = 0x08;
static const uint8_t LCD_DISPLAY_ON = 0x0C;
static const uint8_t LCD_ENTRY_LEFT = 0x06;
static const uint8_t LCD_CUSTOM_CHAR_START = 0x40;

static const uint8_t ROW_START[4] = {
    0x00, 0x40, 0x14, 0x54
};

static const uint8_t LCD_DEGREE_CHARACTER = 0;

static const uint8_t DEGREE_SYMBOL[8] = {
    0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00
};

static uint8_t lcdAddress = 0x27;

static void i2cInit(void) {
    TWSR = 0;
    TWBR = I2C_BIT_RATE;
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

static void writeToBackpack(uint8_t data) {
    i2cStart();
    i2cWrite((lcdAddress << 1) | I2C_WRITE);
    i2cWrite(data | LCD_BACKLIGHT);
    i2cStop();
}

static void pulseEnable(uint8_t data) {
    writeToBackpack(data | LCD_ENABLE);
    customDelayUs(1);
    writeToBackpack(data & ~LCD_ENABLE);
    customDelayUs(50);
}

static void sendNibble(uint8_t nibble, uint8_t mode) {
    pulseEnable((nibble & 0xF0) | mode);
}

static void sendByte(uint8_t data, uint8_t mode) {
    sendNibble(data, mode);
    sendNibble(data << 4, mode);
}

static void lcdCommand(uint8_t command) {
    sendByte(command, LCD_COMMAND);
}

static void lcdData(uint8_t data) {
    sendByte(data, LCD_CHARACTER);
}

static void lcdCreateChar(uint8_t character, const uint8_t rows[8]) {
    lcdCommand(LCD_CUSTOM_CHAR_START | ((character & 7) << 3));

    for (uint8_t i = 0; i < 8; i++) {
        lcdData(rows[i]);
    }
}

void lcdInit(uint8_t address) {
    lcdAddress = address;

    i2cInit();
    customDelay(50);
    writeToBackpack(0);
    customDelay(10);

    sendNibble(LCD_RESET_TO_8_BIT_MODE, LCD_COMMAND);
    customDelay(5);
    sendNibble(LCD_RESET_TO_8_BIT_MODE, LCD_COMMAND);
    customDelayUs(150);
    sendNibble(LCD_RESET_TO_8_BIT_MODE, LCD_COMMAND);
    customDelayUs(150);
    sendNibble(LCD_4_BIT_MODE, LCD_COMMAND);
    customDelayUs(150);

    lcdCommand(LCD_2_LINES_5X8_FONT);
    lcdCommand(LCD_DISPLAY_OFF);
    lcdCommand(LCD_ENTRY_LEFT);
    lcdCreateChar(LCD_DEGREE_CHARACTER, DEGREE_SYMBOL);
    lcdClear();
    lcdCommand(LCD_DISPLAY_ON);
}

void lcdClear(void) {
    lcdCommand(LCD_CLEAR);
    customDelay(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
    lcdCommand(LCD_HOME_LINE | (ROW_START[row % 4] + col));
}

void lcdWriteChar(char c) {
    lcdData((uint8_t)c);
}

void lcdWriteDegree(void) {
    lcdWriteChar((char)LCD_DEGREE_CHARACTER);
}

void lcdPrint(const char *text) {
    while (*text) {
        lcdWriteChar(*text);
        text++;
    }
}
