#include <avr/io.h>
#include "customDelay.h"
#include "lcd.h"

//I2C 
#define I2C_BIT_RATE  72  // 16 MHz / (16 + 2*72*1) ≈ 100 kHz - standardowa prędkość I2C

static uint8_t lcdAddress = 0x27;

// Inicjalizacja I2C, wysyłanie start/stop, zapisywanie bajtów
static void i2cInit(void) {
    TWSR = 0;
    TWBR = I2C_BIT_RATE; // ustawienie szybkości I2C
    TWCR = (1 << TWEN); // włącz I2C
}

static void i2cStart(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // wysłanie sygnału start
    while (!(TWCR & (1 << TWINT))); // czekaj na zakończenie
}

static void i2cStop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // wysłanie sygnału stop
    while (TWCR & (1 << TWSTO)); // czekaj aż stop zostanie wysłany
}

static void i2cWrite(uint8_t data) {
    TWDR = data; // załaduj bajt do rejestru danych
    TWCR = (1 << TWINT) | (1 << TWEN); // rozpocznij transmisję
    while (!(TWCR & (1 << TWINT))); // czekaj na zakończenie
}

// Piny backpacka PCF8574

//  Bit 0 (0x01) = RS - 0: komenda, 1: znak
//  Bit 2 (0x04) = EN - Krótki impuls (0→1→0) powoduje odczyt danych przez LCD - Enable
//  Bit 3 (0x08) = BL - podświetlenie
//  Bity 4-7 = D4-D7 - linie danych LCD (tryb 4-bitowy - nibble)

#define RS  0x01
#define EN  0x04
#define BL  0x08


static void backpackWrite(uint8_t data) {
    i2cStart(); // rozpocznij transmisję
    i2cWrite((lcdAddress << 1)); // rozpocznij transmisję do backpacka - wyślij adres z bitem zapisu (LSB=0)
    i2cWrite(data | BL); // wyślij dane z włączonym podświetleniem
    i2cStop(); // zakończ transmisję
}

// Puls EN — LCD zapamiętuje dane gdy sygnał ENABLE przechodzi z 0 do 1, a następnie z powrotem do 0.
static void pulseEnable(uint8_t data) {
    backpackWrite(data | EN); // EN=1
    customDelayUs(1); // krótki impuls
    backpackWrite(data & ~EN); // EN=0
    customDelayUs(50); // czekaj na przetworzenie danych przez LCD (HD44780 potrzebuje do 37 us)
}

// Wysyła nibble (4 bity) do LCD. Mode=0 dla komendy, RS dla danych.
static void sendNibble(uint8_t data, uint8_t mode) {
    pulseEnable((data & 0xF0) | mode); // wysyłamy górny nibble (D7-D4) + informacja o trybie (komenda/dane)
}

// Wysyła bajt jako dwa nibblee (górny, potem dolny)
static void sendByte(uint8_t data, uint8_t mode) {
    sendNibble(data, mode); // najpierw górny nibble
    sendNibble(data << 4, mode); // potem dolny nibble (przesunięty na pozycję górnego)
}

static void lcdCommand(uint8_t cmd) { 
    sendByte(cmd, 0); // RS=0: komenda 
} 

static void lcdData(uint8_t data) { 
    sendByte(data, RS); // RS=1: znak
} 

// HD44780 obsługuje 8 znaków 5×8 w CGRAM (sloty 0–7).

#define DEGREE_SLOT  0 // slot w CGRAM dla symbolu stopnia

static const uint8_t DEGREE_BITMAP[8] = {
    0x06, 0x09, 0x09, 0x06, 
    0x00, 0x00, 0x00, 0x00
};

static void createChar(uint8_t slot, const uint8_t bitmap[8]) {
    lcdCommand(0x40 | ((slot & 7) << 3)); // ustaw adres CGRAM na początek slotu
    for (uint8_t i = 0; i < 8; i++) { 
        lcdData(bitmap[i]); // wczytaj bitmapę do CGRAM
    }
}

// Układ DDRAM wyświetlacza 20×4:
//   Wiersz 0: 0x00,  
//   Wiersz 1: 0x40,  
//   Wiersz 2: 0x14,  
//   Wiersz 3: 0x54

static const uint8_t ROW_OFFSET[4] = { 
    0x00, 0x40, 0x14, 0x54 
};

//API
void lcdInit(uint8_t address) {
    lcdAddress = address; // zapamiętaj adres I2C LCD

    i2cInit();
    customDelay(50); // czekaj na zasilanie LCD
    backpackWrite(0); // wszystkie piny backpacka w stan niski
    customDelay(10); // czekaj na stabilizację backpacka 

    // Sekwencja inicjalizacji HD44780 - zgodnie z dokumentacją, aby przejść do trybu 4-bitowego, należy wysłać 0x30 trzy razy, a następnie 0x20.:
    sendNibble(0x30, 0); 
    customDelay(5);
    sendNibble(0x30, 0);  
    customDelayUs(150);
    sendNibble(0x30, 0);  
    customDelayUs(150);
    sendNibble(0x20, 0);  
    customDelayUs(150);  // tryb 4-bitowy

    lcdCommand(0x28);  // 2 linie, font 5x8
    lcdCommand(0x08);  // wyłącz wyświetlacz
    lcdCommand(0x06);  // kursor przesuwa się w prawo

    createChar(DEGREE_SLOT, DEGREE_BITMAP); // zdefiniuj symbol stopnia w CGRAM

    lcdClear(); 
    lcdCommand(0x0C);  // włącz wyświetlacz, bez kursora
}

void lcdClear(void) {
    lcdCommand(0x01); // komenda czyszczenia ekranu
    customDelay(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
    lcdCommand(0x80 | (ROW_OFFSET[row % 4] + col)); // ustaw adres DDRAM na podstawie kolumny i wiersza
}

void lcdWriteChar(char c){ 
    lcdData((uint8_t)c); // wyślij znak do LCD      
}

void lcdPrint(const char *text) { 
    while (*text) // wyślij kolejne znaki z łańcucha do LCD - zakończ przy napotkaniu znaku null
        lcdWriteChar(*text++); 
}

void lcdWriteDegree(void) { 
    lcdWriteChar((char)DEGREE_SLOT); // wyświetl symbol stopnia z CGRAM
}