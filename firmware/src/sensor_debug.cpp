#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include "config.h"

static Adafruit_AHTX0  aht20;
static Adafruit_BMP280 bmp280;
static Adafruit_BME280 bme280;
static bool aht_ok  = false;
static bool bmp_ok  = false;
static bool bme_ok  = false;

// BMP280 chip ID register (0xD0): BMP280=0x58, BME280=0x60, BMP388=0x50
static uint8_t read_chip_id(uint8_t i2c_addr)
{
    Wire.beginTransmission(i2c_addr);
    Wire.write(0xD0);
    Wire.endTransmission(false);
    Wire.requestFrom(i2c_addr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

static void i2c_scan()
{
    Serial.println("[i2c] Scanning...");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[i2c]   0x%02X found\n", addr);
            found++;
        }
    }
    if (found == 0) Serial.println("[i2c]   no devices found");
}

void setup()
{
    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 5000) delay(10);
    delay(500);
    Serial.println("\n[sensor_debug] start");
    Serial.printf("  SDA=GPIO%d  SCL=GPIO%d  PIR=GPIO%d\n",
                  I2C_SDA_PIN, I2C_SCL_PIN, PIR_PIN);

    // setPins() で先にピンを設定しておき、aht20.begin() に Wire.begin() を1回だけ呼ばせる
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);

    aht_ok = aht20.begin(&Wire);
    Serial.printf("[aht20] %s\n", aht_ok ? "OK" : "NOT FOUND");

    i2c_scan();

    // チップID直読みで BMP280 / BME280 を判別
    uint8_t chip_id = read_chip_id(BMP280_ADDR);
    Serial.printf("[baro]  chip_id=0x%02X at 0x%02X → ", chip_id, BMP280_ADDR);
    if (chip_id == 0x58) {
        Serial.println("BMP280");
        bmp_ok = bmp280.begin(BMP280_ADDR);
        if (bmp_ok) {
            bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL,
                               Adafruit_BMP280::SAMPLING_X1,
                               Adafruit_BMP280::SAMPLING_X1,
                               Adafruit_BMP280::FILTER_OFF,
                               Adafruit_BMP280::STANDBY_MS_500);
        }
        Serial.printf("[bmp280] %s\n", bmp_ok ? "OK" : "init failed");
    } else if (chip_id == 0x60) {
        Serial.println("BME280 (not BMP280!)");
        bme_ok = bme280.begin(BMP280_ADDR);
        Serial.printf("[bme280] %s\n", bme_ok ? "OK" : "init failed");
    } else {
        Serial.printf("unknown (0x%02X) — check wiring or SDO pin\n", chip_id);
    }

    pinMode(PIR_PIN, INPUT);
    Serial.println("[pir] pin configured\n");
}

void loop()
{
    // ── AHT20 ────────────────────────────────────────────────────────────────
    if (aht_ok) {
        sensors_event_t hum_ev, temp_ev;
        aht20.getEvent(&hum_ev, &temp_ev);
        Serial.printf("[aht20] %.2f °C  %.2f %%RH\n",
                      temp_ev.temperature, hum_ev.relative_humidity);
    } else {
        Serial.println("[aht20] ---");
    }

    // ── BMP280 / BME280 ──────────────────────────────────────────────────────
    if (bmp_ok) {
        float p = bmp280.readPressure() / 100.0f;
        float t = bmp280.readTemperature();
        Serial.printf("[bmp280] %.2f hPa  (%.2f °C)\n", p, t);
    } else if (bme_ok) {
        float p = bme280.readPressure() / 100.0f;
        float t = bme280.readTemperature();
        float h = bme280.readHumidity();
        Serial.printf("[bme280] %.2f hPa  %.2f °C  %.2f %%RH\n", p, t, h);
    } else {
        Serial.println("[baro] ---");
    }

    // ── PIR ──────────────────────────────────────────────────────────────────
    int pir = digitalRead(PIR_PIN);
    Serial.printf("[pir] GPIO%d = %d %s\n\n", PIR_PIN, pir, pir ? "MOTION" : "");

    delay(2000);
}
