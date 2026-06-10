#include <avr/io.h>
#include "customDelay.h"
#include "lcd.h"
#include "i2c.h"
#include <stddef.h>

// Piny backpacka PCF8574

//  Bit 0 (0x01) = RS - 0: komenda, 1: znak
//  Bit 2 (0x04) = EN - Krótki impuls (0→1→0) powoduje odczyt danych przez LCD - Enable
//  Bit 3 (0x08) = BL - podświetlenie
//  Bity 4-7 = D4-D7 - linie danych LCD (tryb 4-bitowy - nibble)

#define RS  0x01U
#define EN  0x04U
#define BL  0x08U
#define DEGREE_SLOT  0U

static uint8_t lcdAddress = 0x27U;

static const uint8_t DEGREE_BITMAP[8] = {
    0x06U, 0x09U, 0x09U, 0x06U, 
    0x00U, 0x00U, 0x00U, 0x00U
};

static const uint8_t BAR_BITMAP_1[8] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU
};
static const uint8_t BAR_BITMAP_2[8] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x1FU
};
static const uint8_t BAR_BITMAP_3[8] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x1FU, 0x1FU
};
static const uint8_t BAR_BITMAP_4[8] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x1FU, 0x1FU, 0x1FU
};
static const uint8_t BAR_BITMAP_5[8] = {
    0x00U, 0x00U, 0x00U, 0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU
};
static const uint8_t BAR_BITMAP_6[8] = {
    0x00U, 0x00U, 0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU
};
static const uint8_t BAR_BITMAP_7[8] = {
    0x00U, 0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU
};

static const uint8_t ROW_OFFSET[4] = { 
    0x00U, 0x40U, 0x14U, 0x54U 
};

/*!
 * @brief Zapisuje pojedynczy bajt danych bezpośrednio do ekspandera.
 * @param data
 * Bajt danych do wysłania na porty IO ekspandera.
 * @side effects:
 * - Inicjuje i kończy sesję transmisji na magistrali I2C.
 * - Wymusza logiczne włączenie podświetlenia ekranu poprzez operację alternatywy z maską BL.
 */
static void backpackWrite(uint8_t data) {
    i2cStart();
    i2cWrite((uint8_t)(lcdAddress << 1U));
    i2cWrite(((uint8_t)data | (uint8_t)BL));
    i2cStop();
}

/*!
 * @brief Generuje wymagany impuls strobujący na linii ENABLE (EN) sterownika HD44780.
 * @param data
 * Bajt danych, z którym ma zostać połączony impuls sterujący.
 * @side effects:
 * - Dwukrotnie wywołuje funkcję backpackWrite, zmieniając stan pinu EN ekspandera.
 * - Blokuje procesor mikrokontrolera na łączny czas około 51 mikrosekund (pętle busy-wait).
 */
static void pulseEnable(uint8_t data) {
    backpackWrite(((uint8_t)data | (uint8_t)EN));
    customDelayUs(1U);
    backpackWrite((uint8_t)((uint8_t)data & (uint8_t)(~((uint8_t)EN))));
    customDelayUs(50U);
}

/*!
 * @brief Wysyła starszą połowę bajtu (nibble) wraz z flagą wyboru rejestru RS.
 * @param data
 * Bajt danych, z którego zostanie wycięte 4 starsze bity (D7-D4).
 * @param mode
 * Tryb pracy kontrolera: wartość RS (dane) lub 0U (instrukcja).
 * @side effects:
 * - Wywołuje procedurę pulseEnable, co skutkuje wysłaniem danych przez magistralę I2C.
 */
static void sendNibble(uint8_t data, uint8_t mode) {
    pulseEnable((uint8_t)(((uint8_t)data & (uint8_t)0xF0U) | (uint8_t)mode));
}

/*!
 * @brief Dzieli bajt danych na dwie części 4-bitowe i wysyła je sekwencyjnie do wyświetlacza.
 * @param data
 * Pełny bajt danych lub komendy przeznaczony dla sterownika LCD.
 * @param mode
 * Tryb pracy kontrolera: wartość RS (dane) lub 0U (instrukcja).
 * @side effects:
 * - Generuje serię dwóch pełnych cykli strobujących na magistrali komunikacyjnej.
 */
static void sendByte(uint8_t data, uint8_t mode) {
    sendNibble(data, mode);
    sendNibble((uint8_t)(data << 4U), mode);
}

/*!
 * @brief Wysyła komendę sterującą do rejestru instrukcji wyświetlacza LCD.
 * @param cmd
 * Bajt zawierający kod rozkazu dla sterownika HD44780.
 * @side effects:
 * - Modyfikuje rejestr wewnętrzny oraz stan pracy kontrolera wyświetlacza (RS = 0U).
 */
static void lcdCommand(uint8_t cmd) { 
    sendByte(cmd, 0U);
} 

/*!
 * @brief Wysyła pojedynczy znak danych do pamięci DDRAM/CGRAM wyświetlacza LCD.
 * @param data
 * Kod ASCII znaku lub adres bitmapy do wyświetlenia.
 * @side effects:
 * - Zapisuje bajt danych do aktualnej komórki pamięci RAM sterownika LCD (RS = RS).
 */
static void lcdData(uint8_t data) { 
    sendByte(data, (uint8_t)RS);
} 

/*!
 * @brief Tworzy i zapisuje niestandardowy znak użytkownika w pamięci generowania znaków CGRAM.
 * @param slot
 * Numer indeksu komórki pamięci CGRAM (zakres 0 do 7).
 * @param bitmap
 * Tablica 8 bajtów definiująca wygląd kolejnych wierszy znaku graficznego.
 * @side effects:
 * - Przestawia tymczasowo licznik adresowy wyświetlacza na obszar pamięci CGRAM.
 * - Nadpisuje wybraną lokację generatora znaków.
 */
static void createChar(uint8_t slot, const uint8_t bitmap[8]) {
    uint8_t shiftedSlot = (uint8_t)((uint8_t)(slot & 7U) << 3U);
    lcdCommand((uint8_t)(0x40U | shiftedSlot));
    for (uint8_t i = 0U; i < 8U; i++) { 
        lcdData(bitmap[i]);
    }
}

static char tempBarChar(uint8_t row, uint8_t fullBlocks, uint8_t partialRows) {
    char result;
    if (row < fullBlocks) {
        result = (char)0xFFU;
    } else if ((row == fullBlocks) && (partialRows > 0U)) {
        result = (char)partialRows;
    } else {
        result = ' ';
    }
    return result;
}

/*!
 * @brief Przeprowadza pełną procedurę inicjalizacji sprzętowej wyświetlacza LCD w trybie 4-bitowym.
 * @param address
 * Adres sprzętowy I2C ekspandera (np. 0x27).
 * @side effects:
 * - Inicjalizuje peryferium i2cInit oraz zapisuje globalny adres urządzenia w 'lcdAddress'.
 * - Generuje serie wielokrotnych, blokujących opóźnień programowych o łącznej długości powyżej 60 milisekund.
 * - Konfiguruje pamięć CGRAM, wgrywając definicję znaku stopnia.
 * - Czyści zawartość ekranu i aktywuje wyświetlacz w trybie ukrytego kursora.
 */
void lcdInit(uint8_t address) {
    lcdAddress = address;

    i2cInit();
    customDelay(50U);
    backpackWrite(0U);
    customDelay(10U);

    // Sekwencja inicjalizacji HD44780:
    sendNibble(0x30U, 0U); 
    customDelay(5U);
    sendNibble(0x30U, 0U);  
    customDelayUs(150U);
    sendNibble(0x30U, 0U);  
    customDelayUs(150U);
    sendNibble(0x20U, 0U);  
    customDelayUs(150U);

    lcdCommand(0x28U);  // 2 linie, font 5x8
    lcdCommand(0x08U);  // wyłącz wyświetlacz
    lcdCommand(0x06U);  // kursor przesuwa się w prawo

    createChar(DEGREE_SLOT, DEGREE_BITMAP); // zdefiniuj symbol stopnia w CGRAM
    createChar(1U, BAR_BITMAP_1);
    createChar(2U, BAR_BITMAP_2);
    createChar(3U, BAR_BITMAP_3);
    createChar(4U, BAR_BITMAP_4);
    createChar(5U, BAR_BITMAP_5);
    createChar(6U, BAR_BITMAP_6);
    createChar(7U, BAR_BITMAP_7);

    lcdClear(); 
    lcdCommand(0x0CU);  // włącz wyświetlacz, bez kursora
}

/*!
 * @brief Czyści zawartość pamięci DDRAM wyświetlacza i przywraca kursor na pozycję początkową (0,0).
 * @side effects:
 * - Wysyła komendę czyszczenia ekranu, co usuwa wszystkie wyświetlane znaki.
 * - Blokuje działanie programu na około 2 milisekundy (wymóg czasowy sterownika HD44780).
 */
void lcdClear(void) {
    lcdCommand(0x01U); // komenda czyszczenia ekranu
    customDelay(2U);
}

/*!
 * @brief Ustawia kursor tekstowy wyświetlacza na zadanych współrzędnych kolumny i wiersza.
 * @param col
 * Numer kolumny (indeksowany od 0U).
 * @param row
 * Numer wiersza (indeksowany od 0U do 3U).
 * @side effects:
 * - Wyznacza adres sprzętowy DDRAM na podstawie tablicy ROW_OFFSET z zabezpieczeniem modulo przed przekroczeniem indeksu.
 */
void lcdSetCursor(uint8_t col, uint8_t row) {
    uint8_t offset = (uint8_t)(ROW_OFFSET[row % 4U] + col);
    lcdCommand((uint8_t)(0x80U | offset));
}

/*!
 * @brief Wyświetla pojedynczy znak tekstowy typu char na aktualnej pozycji kursora.
 * @param c
 * Znak w standardzie ASCII przeznaczony do wyświetlenia.
 * @side effects:
 * - Wywołuje wewnętrzną procedurę zapisu danych z rzutowaniem typu char na uint8_t.
 */
void lcdWriteChar(char c){ 
    lcdData((uint8_t)c);   
}

/*!
 * @brief Wyświetla ciąg znaków (łańcuch tekstowy zakończony znakiem NULL) na ekranie LCD.
 * @param  text
 * Wskaźnik typu const char na pierwszy znak ciągu tekstowego.
 * @side effects:
 * - Iteruje po pamięci wskaźnika, inkrementując jego adres aż do napotkania wartości zero.
 * - Blokuje program na czas transmisji wszystkich znaków składowych łańcucha.
 */
void lcdPrint(const char *text) { 
    if (text != NULL) {
        const char *ptr = text;
        while (*ptr != '\0') {
            lcdWriteChar(*ptr);
            ptr++;
        }
    }
}

/*!
 * @brief Wyświetla dedykowany symbol stopnia w miejscu aktualnego położenia kursora.
 * @side effects:
 * - Wywołuje lcdWriteChar przekazując zrzutowany na typ char indeks komórki pamięci CGRAM.
 */
void lcdWriteDegree(void) { 
    lcdWriteChar((char)DEGREE_SLOT);
}

void lcdDrawTempBar(uint8_t temperature) {
    uint8_t total = temperature * 32U / 40U;
    if (total > 32U) { 
        total = 32U; 
    }

    uint8_t fullBlocks  = (uint8_t)(total / 8U);
    uint8_t partialRows = (uint8_t)(total % 8U);

    lcdSetCursor(19U, 3U);
    lcdWriteChar(tempBarChar(0U, fullBlocks, partialRows));

    lcdSetCursor(19U, 2U);
    lcdWriteChar(tempBarChar(1U, fullBlocks, partialRows));

    lcdSetCursor(19U, 1U);
    lcdWriteChar(tempBarChar(2U, fullBlocks, partialRows));

    lcdSetCursor(19U, 0U);
    lcdWriteChar(tempBarChar(3U, fullBlocks, partialRows));
}