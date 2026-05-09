// DS1307 RTC I2C implementation for Firmware Contest
#include <Arduino.h>
#include <Wire.h>

#define DS1307_ADDR    0x68
#define MAX_RETRY      3
#define RETRY_DELAY_MS 300

static uint8_t bcdToDec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NACK,
    STATUS_ERR_OTHER,
    STATUS_ERR_SHORT_READ,
} I2CStatus;

static I2CStatus readDS1307(uint16_t &year, uint8_t &month, uint8_t &day,
                              uint8_t &hour, uint8_t &min,  uint8_t &sec) {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(0x00);
    uint8_t err = Wire.endTransmission();

    if (err == 2 || err == 3) return STATUS_ERR_NACK;
    if (err != 0)             return STATUS_ERR_OTHER;

    uint8_t n = Wire.requestFrom(DS1307_ADDR, 7);
    if (n == 0) return STATUS_ERR_NACK;
    if (n < 7)  return STATUS_ERR_SHORT_READ;

    sec   = bcdToDec(Wire.read() & 0x7F);  // 0x00 seconds
    min   = bcdToDec(Wire.read());          // 0x01 minutes
    hour  = bcdToDec(Wire.read() & 0x3F);  // 0x02 hours
    Wire.read();                            // 0x03 day-of-week (skip)
    day   = bcdToDec(Wire.read());          // 0x04 date
    month = bcdToDec(Wire.read());          // 0x05 month
    year  = 2000 + bcdToDec(Wire.read());  // 0x06 year

    return STATUS_OK;
}

static const char* statusStr(I2CStatus s) {
    switch (s) {
        case STATUS_ERR_NACK:       return "NACK";
        case STATUS_ERR_SHORT_READ: return "SHORT";
        case STATUS_ERR_OTHER:      return "ERR";
        default:                    return "UNKNOWN";
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();

    Serial.println("BOOT OK");

    uint16_t year  = 0;
    uint8_t  month = 0, day  = 0;
    uint8_t  hour  = 0, min  = 0, sec = 0;
    I2CStatus status = STATUS_ERR_OTHER;

    for (int attempt = 1; attempt <= MAX_RETRY; attempt++) {
        status = readDS1307(year, month, day, hour, min, sec);
        if (status == STATUS_OK) break;

        char msg[32];
        snprintf(msg, sizeof(msg), "RETRY %d/%d %s",
                 attempt, MAX_RETRY, statusStr(status));
        Serial.println(msg);

        if (attempt < MAX_RETRY) delay(RETRY_DELAY_MS);
    }

    if (status == STATUS_OK) {
        char buf[32];
        snprintf(buf, sizeof(buf),
                 "TIME %04u-%02u-%02u %02u:%02u:%02u",
                 year, month, day, hour, min, sec);
        Serial.println(buf);
    } else {
        char msg[24];
        snprintf(msg, sizeof(msg), "ERROR %s", statusStr(status));
        Serial.println(msg);
    }
}

void loop() {}
