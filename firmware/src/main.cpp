/*
 * main.cpp — E-ink Smart Display
 *
 * Hardware:
 *   - Waveshare 5.79" e-Paper Module (B)  792×272px  BW+Red
 *   - AHT20 (I2C 0x38)  temperature + humidity
 *   - BMP280 (I2C 0x77) atmospheric pressure
 *   - PIR sensor (PIR_PIN)
 *
 * Layout (792×272):
 *   Left panel  (0..269):   large clock (Font64), date, temp/hum/pressure
 *   Divider:                vertical line at x=270
 *   Right panel (270..791): up to 4 Google Calendar events (auto height)
 *
 * Sleep strategy:
 *   Normal: deep sleep SLEEP_SECONDS, wake by timer
 *   PIR:    deep sleep, wake by PIR_PIN rising edge → immediate refresh
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <GxEPD2_3C.h>
#include <gdey3c/GxEPD2_579c_GDEY0579Z93.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "config.h"
#include "esp_wifi.h"
#if WIFI_USE_ENTERPRISE
  #include "esp_eap_client.h"
#endif
#include "googlecal.h"

// ── Frame buffers (BW + Red, 1 bit per pixel) ───────────────────────────────
// 792×272 / 8 = 26928 bytes each
static GxEPD2_3C<GxEPD2_579c_GDEY0579Z93, GxEPD2_579c_GDEY0579Z93::HEIGHT> display(
    GxEPD2_579c_GDEY0579Z93(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

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
    esp_eap_client_set_disable_time_check(true);
    esp_eap_client_use_default_cert_bundle(false);
    esp_eap_client_set_eap_methods(ESP_EAP_TYPE_PEAP);
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
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);

    // AHT20
    temp_c   = NAN;
    humidity = NAN;
    if (aht20.begin(&Wire)) {
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

#if 0
// Legacy GUI_Paint layout kept only as history while GxEPD2 is the active renderer.
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

static int text_width(const char* s, const sFONT* font)
{
    return strlen(s) * font->Width;
}

static void draw_string_center(UBYTE* image, int x, int y, int w,
                               const char* s, sFONT* font)
{
    int tx = x + (w - text_width(s, font)) / 2;
    if (tx < x) tx = x;
    Paint_SelectImage(image);
    Paint_DrawString_EN(tx, y, s, font, BLACK, WHITE);
}

static void draw_event_item(UBYTE* bw, UBYTE* red, int x, int y, int w,
                            const CalEvent& ev, bool accent)
{
    Paint_SelectImage(accent ? red : bw);
    Paint_DrawRectangle(x, y + 1, x + 3, y + 25,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_SelectImage(accent ? red : bw);
    Paint_DrawString_EN(x + 8, y + 2, ev.allDay ? "ALL" : ev.time,
                        &Font12, BLACK, WHITE);

    Paint_SelectImage(bw);
    char title[48];
    strncpy(title, ev.title, sizeof(title) - 1);
    title[sizeof(title) - 1] = '\0';
    const int titleX = x + 74;
    const int maxChars = max(1, (w - 82) / (int)Font12.Width);
    if ((int)strlen(title) > maxChars) {
        title[maxChars - 1] = '.';
        title[maxChars] = '\0';
    }
    Paint_DrawString_EN(titleX, y + 2, title, &Font12, BLACK, WHITE);
}

static void draw_display_layout(int hour, int minute,
                                const struct tm* ti,
                                float temp, float hum, float pres,
                                const CalEvent events[CAL_MAX_EVENTS],
                                int eventCount)
{
    const int W = EPD_5in79b_WIDTH;
    const int H = EPD_5in79b_HEIGHT;
    const int LEFT_W = 340;
    const int SENSOR_H = 82;
    const int WEEK_Y = SENSOR_H + 8;
    const int WEEK_H = 70;
    const int EVENT_Y = WEEK_Y + WEEK_H + 8;

    Paint_SelectImage(bwImage);
    Paint_DrawRectangle(0, 0, W - 1, H - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawLine(LEFT_W, 0, LEFT_W, H - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(LEFT_W, SENSOR_H, W - 1, SENSOR_H, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d/%02d", ti->tm_year + 1900, ti->tm_mon + 1);
    Paint_DrawString_EN(20, 16, buf, &Font16, BLACK, WHITE);

    const char* wday[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    Paint_SelectImage(redImage);
    Paint_DrawRectangle(264, 11, 327, 33, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_SelectImage(bwImage);
    Paint_DrawString_EN(279, 14, wday[ti->tm_wday], &Font12, WHITE, BLACK);

    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    draw_string_center(redImage, 0, 90, LEFT_W, buf, &Font64);

    snprintf(buf, sizeof(buf), "%02d", ti->tm_mday);
    draw_string_center(bwImage, 80, 180, 92, buf, &Font20);
    snprintf(buf, sizeof(buf), "/ %02d", ti->tm_mon + 1);
    Paint_SelectImage(bwImage);
    Paint_DrawString_EN(168, 184, buf, &Font16, BLACK, WHITE);

    const int rightX = LEFT_W + 1;
    const int rightW = W - rightX - 1;
    const int sensorW = rightW / 3;
    const char* labels[3] = {"TEMP", "HUM", "PRES"};
    char values[3][16];
    snprintf(values[0], sizeof(values[0]), isnan(temp) ? "--.- C" : "%.1f C", temp);
    snprintf(values[1], sizeof(values[1]), isnan(hum) ? "-- %" : "%.0f %%", hum);
    snprintf(values[2], sizeof(values[2]), isnan(pres) ? "---- hPa" : "%.0f hPa", pres);

    for (int i = 0; i < 3; i++) {
        const int x = rightX + sensorW * i;
        if (i > 0) {
            Paint_SelectImage(bwImage);
            Paint_DrawLine(x, 0, x, SENSOR_H, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
        draw_string_center(bwImage, x, 12, sensorW, labels[i], &Font12);
        draw_string_center(bwImage, x, 44, sensorW, values[i], &Font20);
    }

    const int weekX = rightX + 10;
    const int cellGap = 4;
    const int cellW = (rightW - 20 - cellGap * 6) / 7;
    const int dowMondayFirst[7] = {1, 2, 3, 4, 5, 6, 0};
    const int mondayOffset = (ti->tm_wday == 0) ? -6 : 1 - ti->tm_wday;
    for (int i = 0; i < 7; i++) {
        const int x = weekX + i * (cellW + cellGap);
        const bool today = (mondayOffset + i) == 0;
        Paint_SelectImage(today ? redImage : bwImage);
        Paint_DrawRectangle(x, WEEK_Y, x + cellW - 1, WEEK_Y + WEEK_H - 1,
                            BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

        draw_string_center(bwImage, x, WEEK_Y + 8, cellW, wday[dowMondayFirst[i]], &Font12);

        struct tm day = *ti;
        day.tm_mday += mondayOffset + i;
        mktime(&day);
        snprintf(buf, sizeof(buf), "%02d", day.tm_mday);
        draw_string_center(today ? redImage : bwImage, x, WEEK_Y + 31, cellW, buf, &Font16);

        if (today && eventCount > 0) {
            Paint_SelectImage(redImage);
            Paint_DrawRectangle(x + cellW / 2 - 2, WEEK_Y + 58,
                                x + cellW / 2 + 2, WEEK_Y + 62,
                                BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    }

    if (eventCount <= 0) {
        draw_string_center(bwImage, rightX, EVENT_Y + 28, rightW, "No events today", &Font16);
    } else {
        const int itemH = 29;
        const int maxItems = min(eventCount, CAL_MAX_EVENTS);
        for (int i = 0; i < maxItems; i++) {
            if (!events[i].valid) continue;
            draw_event_item(bwImage, redImage, rightX + 12, EVENT_Y + i * itemH,
                            rightW - 24, events[i], i == 0);
        }
    }

    Paint_SelectImage(bwImage);
}

#endif

static void draw_centered(const char* s, int16_t x, int16_t w, int16_t y)
{
    int16_t bx, by;
    uint16_t bw, bh;
    display.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
    display.setCursor(x + (w - (int16_t)bw) / 2 - bx, y);
    display.print(s);
}

static void print_or_dash(float value, const char* fmt, char* out, size_t outLen)
{
    if (isnan(value)) {
        snprintf(out, outLen, "--");
    } else {
        snprintf(out, outLen, fmt, value);
    }
}

static void draw_event_item(int x, int y, int w, const CalEvent& ev, bool accent)
{
    display.fillRect(x, y + 4, 3, 32, accent ? GxEPD_RED : GxEPD_BLACK);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(accent ? GxEPD_RED : GxEPD_BLACK);
    display.setCursor(x + 8, y + 22);
    display.print(ev.allDay ? "ALL" : ev.time);

    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x + 76, y + 22);

    char title[48];
    strncpy(title, ev.title, sizeof(title) - 1);
    title[sizeof(title) - 1] = '\0';
    const int maxChars = max(1, (w - 86) / 13);
    if ((int)strlen(title) > maxChars) {
        title[maxChars - 1] = '.';
        title[maxChars] = '\0';
    }
    display.print(title);
}

static void draw_display_layout_gx(int hour, int minute,
                                   const struct tm* ti,
                                   float temp, float hum, float pres,
                                   const CalEvent events[CAL_MAX_EVENTS],
                                   int eventCount)
{
    const int LEFT_W = 340;
    const int SENSOR_H = 82;
    const int RIGHT_X = LEFT_W + 1;
    const int RIGHT_W = 792 - RIGHT_X - 1;
    const int WEEK_Y = SENSOR_H + 8;
    const int WEEK_H = 70;
    const int EVENT_Y = WEEK_Y + WEEK_H + 8;

    display.fillScreen(GxEPD_WHITE);
    display.drawRect(0, 0, 792, 272, GxEPD_BLACK);
    display.drawLine(LEFT_W, 0, LEFT_W, 271, GxEPD_BLACK);
    display.drawLine(RIGHT_X, SENSOR_H, 791, SENSOR_H, GxEPD_BLACK);

    char buf[32];
    static const char* const monthName[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    static const char* const wday[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, 32);
    snprintf(buf, sizeof(buf), "%04d %s", ti->tm_year + 1900, monthName[ti->tm_mon]);
    display.print(buf);

    display.fillRect(264, 6, 63, 28, GxEPD_RED);
    display.setTextColor(GxEPD_WHITE);
    draw_centered(wday[ti->tm_wday], 264, 63, 30);

    display.setFont(&FreeSansBold24pt7b);
    display.setTextSize(2);
    display.setTextColor(GxEPD_RED);
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    draw_centered(buf, 0, LEFT_W, 165);
    display.setTextSize(1);

    const int sensorW = RIGHT_W / 3;
    const char* labels[3] = {"TEMP", "HUM", "PRES"};
    char values[3][16];
    print_or_dash(temp, "%.1f", values[0], sizeof(values[0]));
    print_or_dash(hum, "%.0f", values[1], sizeof(values[1]));
    print_or_dash(pres, "%.0f", values[2], sizeof(values[2]));
    const char* units[3] = {"C", "%", "hPa"};

    for (int i = 0; i < 3; i++) {
        const int x = RIGHT_X + sensorW * i;
        if (i > 0) display.drawLine(x, 0, x, SENSOR_H - 1, GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        draw_centered(labels[i], x, sensorW, 26);
        display.setFont(&FreeSansBold12pt7b);
        draw_centered(values[i], x, sensorW, 60);
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(x + sensorW - 44, 78);
        display.print(units[i]);
    }

    const int cellGap = 4;
    const int cellW = (RIGHT_W - 20 - cellGap * 6) / 7;
    const int weekX = RIGHT_X + 10;
    const int dowMondayFirst[7] = {1, 2, 3, 4, 5, 6, 0};
    const int mondayOffset = (ti->tm_wday == 0) ? -6 : 1 - ti->tm_wday;
    for (int i = 0; i < 7; i++) {
        const int x = weekX + i * (cellW + cellGap);
        const bool today = (mondayOffset + i) == 0;
        display.drawRect(x, WEEK_Y, cellW, WEEK_H, today ? GxEPD_RED : GxEPD_BLACK);
        display.setTextColor(today ? GxEPD_RED : GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        draw_centered(wday[dowMondayFirst[i]], x, cellW, WEEK_Y + 24);

        struct tm day = *ti;
        day.tm_mday += mondayOffset + i;
        mktime(&day);
        snprintf(buf, sizeof(buf), "%02d", day.tm_mday);
        display.setFont(&FreeSansBold12pt7b);
        draw_centered(buf, x, cellW, WEEK_Y + 50);

        if (today && eventCount > 0) {
            display.fillRect(x + cellW / 2 - 2, WEEK_Y + 60, 4, 4, GxEPD_RED);
        }
    }

    if (eventCount <= 0) {
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        draw_centered("No events today", RIGHT_X, RIGHT_W, EVENT_Y + 44);
    } else {
        const int maxItems = min(eventCount, CAL_MAX_EVENTS);
        for (int i = 0; i < maxItems; i++) {
            if (!events[i].valid) continue;
            draw_event_item(RIGHT_X + 12, EVENT_Y + i * 40, RIGHT_W - 24, events[i], i == 0);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    uint32_t serialStart = millis();
    while (!Serial && millis() - serialStart < 3000) delay(10);
    delay(200);

    // ── PIR check: only update display when person is detected ──────────────
    // Wake sources: EXT1 (PIR rising) or timer (periodic 60s check).
    // On timer wake, re-read PIR GPIO to confirm presence before updating.
    // EXT1 is armed only when PIR is LOW before sleep to avoid level re-trigger.
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    // PIR未接続のため常時HIGH扱い（接続後は下2行を有効化し、この行を削除）
    bool pir_high  = true;
    // pinMode(PIR_PIN, INPUT);
    // bool pir_high   = (digitalRead(PIR_PIN) == HIGH);
    bool do_update  = (wakeup == ESP_SLEEP_WAKEUP_EXT1) || pir_high;

    Serial.printf("\n[boot] wakeup=%d pir=%s -> %s\n",
                  wakeup, pir_high ? "HIGH" : "LOW",
                  do_update ? "update" : "skip");

    // スリープ時間: 更新時は次の5分境界まで、スキップ時はPIR再チェック用
    uint64_t sleep_secs = SLEEP_SECONDS;

    if (do_update) {
        // ── WiFi + NTP ───────────────────────────────────────────────────────
        wifi_connect();
        if (WiFi.status() == WL_CONNECTED) {
            timeClient.begin();
            timeClient.update();
            calCount = googlecal_fetch(GCAL_ENDPOINT, calEvents);
        }

        // ── Sensors ──────────────────────────────────────────────────────────
        float temp = NAN, hum = NAN, pres = NAN;
        read_sensors(temp, hum, pres);

        // ── Disconnect WiFi ───────────────────────────────────────────────────
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        // ── Get time values ───────────────────────────────────────────────────
        int hour     = timeClient.getHours();
        int minute   = timeClient.getMinutes();
        time_t epoch = timeClient.getEpochTime();
        struct tm* ti = localtime(&epoch);

        // ── 次の5分境界までのスリープ時間を計算 ──────────────────────────────
        // UTC epochの5分余り=0が5分境界。JSTはUTC+9hでありUTC+32400s、32400%300==0なので補正不要
        uint64_t secs_past = (uint64_t)epoch % 300;
        sleep_secs = 300 - secs_past;
        Serial.printf("[sleep] %llu s to next 5-min mark\n", sleep_secs);

        // ── Draw ─────────────────────────────────────────────────────────────
        pinMode(EPD_PWR_PIN, OUTPUT);
        digitalWrite(EPD_PWR_PIN, HIGH);
        delay(100);

        SPI.begin(EPD_SCK_PIN, /*MISO*/ -1, EPD_MOSI_PIN, EPD_CS_PIN);
        display.init(115200);
        display.setRotation(0);
        display.setFullWindow();

        Serial.println("[epd] Displaying with GxEPD2...");
        display.firstPage();
        do {
            draw_display_layout_gx(hour, minute, ti, temp, hum, pres, calEvents, calCount);
        } while (display.nextPage());
        display.hibernate();
        digitalWrite(EPD_PWR_PIN, LOW);
        Serial.println("[epd] Done");
    }

    // ── Deep sleep ────────────────────────────────────────────────────────────
    // Arm PIR ext1 only when PIR is currently LOW to avoid immediate re-trigger
    pir_high = (digitalRead(PIR_PIN) == HIGH);
    if (!pir_high) {
#if defined(BOARD_ESP32S3_DEVKITC1) || defined(BOARD_ESP32_DEVKIT)
        esp_sleep_enable_ext1_wakeup(1ULL << PIR_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
#else
        esp_deep_sleep_enable_gpio_wakeup(1ULL << PIR_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
#endif
    }
    esp_sleep_enable_timer_wakeup(sleep_secs * 1000000ULL);
    esp_deep_sleep_start();
}

void loop()
{
    // Never reached — ESP32 wakes via deep sleep → setup() runs again
}
