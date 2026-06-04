#include "clock.h"
#include "i2c.h"
#include <avr/io.h>

#define DS3231_ADDRESS_WRITE 0xD0 // Adres 0x68 przesunięty w lewo o 1 bit (bit zapisu = 0)
#define DS3231_ADDRESS_READ  0xD1 // Adres 0x68 przesunięty w lewo o 1 bit (bit odczytu = 1)

static uint8_t bcdToDec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

void rtcReadTime(RTC_Time *time) {
    i2cStart();
    i2cWrite(DS3231_ADDRESS_WRITE);
    i2cWrite(0x00); 
    
    i2cStart();
    i2cWrite(DS3231_ADDRESS_READ);
    
    uint8_t rawSeconds = i2cRead(1);
    uint8_t rawMinutes = i2cRead(1);
    uint8_t rawHours   = i2cRead(0);
    
    i2cStop();
    
    (*time).seconds = bcdToDec(rawSeconds);
    (*time).minutes = bcdToDec(rawMinutes);
    (*time).hours   = bcdToDec(rawHours & 0x3F);
}

