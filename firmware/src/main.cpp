/*
 * main.cpp — E-ink Smart Display (ESP32-C3 Supermini)
 *
 * Hardware:
 *   - Waveshare 5.79" e-Paper Module (B)  792×272px  BW+Red
 *   - AHT20 (I2C 0x38)  temperature + humidity
 *   - BMP280 (I2C 0x76) atmospheric pressure
 *   - PIR sensor (GPIO4, RTC-capable for deep sleep wake)
 *
 * Layout (792×272):
 *   Left panel  (0..269):   large clock (Font64), date, temp/hum/pressure
 *   Divider:                vertical line at x=270
 *   Right panel (270..791): up to 4 Google Calendar events (auto height)
 *
 * Sleep strategy:
 *   Normal: deep sleep SLEEP_SECONDS, wake by timer
 *   PIR:    deep sleep, wake by GPIO4 rising edge → immediate refresh
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include "config.h"
#include "esp_wifi.h"
#if WIFI_USE_ENTERPRISE
  #include "esp_eap_client.h"
#endif
#include "DEV_Config.h"
#include "EPD_5in79b.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "font64.h"
#include "googlecal.h"
#include "calendar.h"

// ── Frame buffers (BW + Red, 1 bit per pixel) ───────────────────────────────
// 792×272 / 8 = 26928 bytes each
#define FRAME_SIZE  ((EPD_5in79b_WIDTH * EPD_5in79b_HEIGHT) / 8)
static UBYTE bwImage[FRAME_SIZE];
static UBYTE redImage[FRAME_SIZE];

// ── Sensor objects ───────────────────────────────────────────────────────────
static Adafruit_AHTX0  aht20;
static Adafruit_BMP280 bmp280;

// ── NTP ─────────────────────────────────────────────────────────────────────
static WiFiUDP     ntpUDP;
static NTPClient   timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, NTP_INTERVAL);

// ── Calendar events ──────────────────────────────────────────────────────────
static CalEvent calEvents[CAL_MAX_EVENTS];
static int      calCount = 0;

// ────────────────────────────────────────────────────────────────────────────

static void wifi_connect()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

#ifdef WIFI_FIXED_MAC
    {
        uint8_t fixed_mac[] = WIFI_FIXED_MAC;
        esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, fixed_mac);
        Serial.printf("[wifi] Set fixed MAC: %02X:%02X:%02X:%02X:%02X:%02X (%s)\n",
                      fixed_mac[0], fixed_mac[1], fixed_mac[2],
                      fixed_mac[3], fixed_mac[4], fixed_mac[5],
                      err == ESP_OK ? "OK" : esp_err_to_name(err));
    }
#endif

    // 実際に使われているMACを確認
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    Serial.printf("[wifi] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    Serial.printf("[wifi] Connecting to %s ", WIFI_SSID);

#if WIFI_USE_ENTERPRISE
    // WPA2-Enterprise PEAP-MSCHAPv2 — 学校ネットワーク用
    // inner username も @domain 付きが必要 (toyota-ti.ac.jp NPS の要件)
    static const char eap_user[] = EAP_USERNAME "@" EAP_DOMAIN;
    esp_eap_client_set_eap_methods(ESP_EAP_TYPE_PEAP);
    esp_eap_client_set_disable_time_check(true);
    esp_eap_client_use_default_cert_bundle(false);
    esp_eap_client_set_identity((uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_eap_client_set_username((uint8_t*)eap_user, strlen(eap_user));
    esp_eap_client_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_enterprise_enable();
    WiFi.begin(WIFI_SSID);
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[wifi] Connected, IP: %s\n",
                      WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("\n[wifi] Failed (status=%d)\n", WiFi.status());
    }
}

static void read_sensors(float &temp_c, float &humidity, float &pressure_hpa)
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // AHT20
    temp_c   = NAN;
    humidity = NAN;
    if (aht20.begin()) {
        sensors_event_t hum_ev, temp_ev;
        aht20.getEvent(&hum_ev, &temp_ev);
        temp_c   = temp_ev.temperature;
        humidity = hum_ev.relative_humidity;
        Serial.printf("[aht20] %.1f°C  %.1f%%RH\n", temp_c, humidity);
    } else {
        Serial.println("[aht20] Not found");
    }

    // BMP280
    pressure_hpa = NAN;
    if (bmp280.begin(BMP280_ADDR)) {
        bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                           Adafruit_BMP280::SAMPLING_X1,
                           Adafruit_BMP280::SAMPLING_X1,
                           Adafruit_BMP280::FILTER_OFF,
                           Adafruit_BMP280::STANDBY_MS_1);
        pressure_hpa = bmp280.readPressure() / 100.0f;
        Serial.printf("[bmp280] %.1f hPa\n", pressure_hpa);
    } else {
        Serial.println("[bmp280] Not found");
    }
}

// Draw left panel: clock + date + sensors
static void draw_left_panel(int hour, int minute, int second,
                             const char* dateStr,
                             float temp, float hum, float pres)
{
    // ── Large clock (Font64) ─────────────────────────────────────────────────
    // "HH:MM" — 5 chars × 59px wide = 295px, center in 270px panel
    char clockStr[8];
    snprintf(clockStr, sizeof(clockStr), "%02d:%02d", hour, minute);

    // Font64 width=59, height=64. 5 chars → 295 > 270, so we scale to Font24
    // for the full "HH:MM" and use Font64 only for hours/minutes separately.
    // Center x: (270 - 3*Font64.Width) / 2  (HH + : + MM ~ 3 chars wide with colon narrower)
    int cx = 4;
    Paint_SelectImage(bwImage);
    Paint_DrawString_EN(cx, 8, clockStr, &Font64, BLACK, WHITE);

    // ── Seconds (Font20) ────────────────────────────────────────────────────
    char secStr[4];
    snprintf(secStr, sizeof(secStr), "%02d", second);
    Paint_DrawString_EN(cx + Font64.Width * 4, 52, secStr, &Font20, BLACK, WHITE);

    // ── Date (Font16) ────────────────────────────────────────────────────────
    Paint_DrawString_EN(4, 82, dateStr, &Font16, BLACK, WHITE);

    // ── Horizontal divider between clock and sensors ─────────────────────────
    Paint_DrawLine(4, 106, 265, 106, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // ── Sensor readings (Font16) ─────────────────────────────────────────────
    char buf[32];

    int sy = 115;
    // Temperature
    if (!isnan(temp)) {
        snprintf(buf, sizeof(buf), "%.1f C", temp);
    } else {
        snprintf(buf, sizeof(buf), "-- C");
    }
    Paint_DrawString_EN(4, sy, buf, &Font16, BLACK, WHITE);

    sy += Font16.Height + 4;
    // Humidity
    if (!isnan(hum)) {
        snprintf(buf, sizeof(buf), "%.1f %%", hum);
    } else {
        snprintf(buf, sizeof(buf), "-- %");
    }
    Paint_DrawString_EN(4, sy, buf, &Font16, BLACK, WHITE);

    sy += Font16.Height + 4;
    // Pressure
    if (!isnan(pres)) {
        snprintf(buf, sizeof(buf), "%.0f hPa", pres);
    } else {
        snprintf(buf, sizeof(buf), "-- hPa");
    }
    Paint_DrawString_EN(4, sy, buf, &Font16, BLACK, WHITE);

    // ── Vertical divider ─────────────────────────────────────────────────────
    Paint_DrawLine(269, 0, 269, 271, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

// ────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[boot] E-ink Smart Display starting");

    // ── WiFi + NTP ───────────────────────────────────────────────────────────
    wifi_connect();
    if (WiFi.status() == WL_CONNECTED) {
        timeClient.begin();
        timeClient.update();
        // Fetch calendar events
        calCount = googlecal_fetch(GCAL_ENDPOINT, calEvents);
    }

    // ── Sensors ──────────────────────────────────────────────────────────────
    float temp = NAN, hum = NAN, pres = NAN;
    read_sensors(temp, hum, pres);

    // ── Disconnect WiFi to save power before e-ink refresh ───────────────────
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // ── Prepare image buffers ─────────────────────────────────────────────────
    // BW: 0xFF = white, 0x00 = black
    // Red: 0xFF = no red, 0x00 = red pixel
    memset(bwImage,  0xFF, FRAME_SIZE);
    memset(redImage, 0xFF, FRAME_SIZE);

    Paint_NewImage(bwImage, EPD_5in79b_WIDTH, EPD_5in79b_HEIGHT, ROTATE_0, WHITE);
    Paint_SetScale(2);

    // ── Get time values ───────────────────────────────────────────────────────
    int hour   = timeClient.getHours();
    int minute = timeClient.getMinutes();
    int second = timeClient.getSeconds();

    // Date string: "YYYY/MM/DD (ddd)"
    time_t epochTime = timeClient.getEpochTime();
    struct tm* ti = localtime(&epochTime);
    char dateStr[24];
    const char* wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d (%s)",
             ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
             wday[ti->tm_wday]);

    // ── Draw ─────────────────────────────────────────────────────────────────
    draw_left_panel(hour, minute, second, dateStr, temp, hum, pres);
    calendar_draw(bwImage, redImage, calEvents, calCount);

    // ── Init e-ink and push frame ─────────────────────────────────────────────
    DEV_Module_Init();
    if (EPD_5in79b_Init() != 0) {
        Serial.println("[epd] Init failed!");
    } else {
        Serial.println("[epd] Displaying...");
        EPD_5in79b_Display(bwImage, redImage);
        EPD_5in79b_Sleep();
        Serial.println("[epd] Done, entering deep sleep");
    }
    DEV_Module_Exit();

    // ── Deep sleep ────────────────────────────────────────────────────────────
    // Also enable PIR (GPIO4) as ext0 wake source
    esp_deep_sleep_enable_gpio_wakeup(1ULL << PIR_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
    esp_deep_sleep_start();
}

void loop()
{
    // Never reached — ESP32-C3 wakes via deep sleep → setup() runs again
}
