/*
 * E-ink Smart Display - Layout C v2 + WiFi(WPA2 Enterprise) + NTP
 * Waveshare 5.79"(B) 792x272 3色 / ESP32-C3
 *
 * 必要な依存ライブラリ (platformio.ini):
 *   lib_deps =
 *       adafruit/Adafruit GFX Library
 *       olikraus/U8g2_for_Adafruit_GFX
 *
 * 注意: WiFi 認証情報を main.cpp に直書きしているので
 *       公開リポジトリへのコミット前に必ず secrets.h に分離すること
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WiFi.h>
#include <time.h>
#include "epd5in79b.h"

// ---- WPA2 Enterprise API (Arduino-ESP32 のバージョンで切り替え) ----
#if __has_include("esp_eap_client.h")
  // Arduino-ESP32 3.x 系 (新API)
  #include "esp_eap_client.h"
  #include "esp_wifi.h"
  #define EAP_NEW_API 1
#else
  // Arduino-ESP32 2.x 系 (旧API)
  #include "esp_wpa2.h"
  #define EAP_NEW_API 0
#endif

// ========== WiFi (WPA2 Enterprise) ==========
static const char* WIFI_SSID    = "TeaClass";
static const char* EAP_IDENTITY = "sd23095";   // Anonymous identity (= username で大抵OK)
static const char* EAP_USERNAME = "sd23095";
static const char* EAP_PASSWORD = "Posaka1204";

static const uint32_t WIFI_TIMEOUT_MS = 20000;  // Enterprise は遅いので余裕
static const uint32_t NTP_TIMEOUT_MS  = 10000;

// ========== NTP / TZ ==========
static const char* NTP_SERVER = "ntp.nict.jp";
static const char* NTP_TZ     = "JST-9";        // POSIX TZ string for Japan

// ========== バッファ ==========
#define BUF_SIZE (EPD_WIDTH / 8 * EPD_HEIGHT)   // 26928 bytes
static uint8_t blackBuf[BUF_SIZE];
static uint8_t redBuf[BUF_SIZE];

// ========== 色 ==========
enum EpdColor : uint16_t { COL_WHITE = 0, COL_BLACK = 1, COL_RED = 2 };

// ========== Adafruit_GFX 適合バッファ ==========
class EpdGfx : public Adafruit_GFX {
public:
    EpdGfx(uint8_t* black, uint8_t* red)
        : Adafruit_GFX(EPD_WIDTH, EPD_HEIGHT), _black(black), _red(red) {}

    void clearBuffer() {
        memset(_black, 0xFF, BUF_SIZE);
        memset(_red,   0xFF, BUF_SIZE);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;
        const uint32_t idx = (uint32_t)y * (EPD_WIDTH / 8) + (x >> 3);
        const uint8_t  bit = 0x80 >> (x & 7);
        _black[idx] |= bit;
        _red[idx]   |= bit;
        switch (color) {
            case COL_BLACK: _black[idx] &= ~bit; break;
            case COL_RED:   _red[idx]   &= ~bit; break;
            case COL_WHITE: break;
        }
    }
private:
    uint8_t* _black;
    uint8_t* _red;
};

// ========== グローバル ==========
Epd epd;
EpdGfx gfx(blackBuf, redBuf);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// ========== 表示データ ==========
struct CalendarEvent {
    const char* time;
    const char* title;
    const char* location;
    bool urgent;
};

struct DisplayData {
    int year, month, day;
    const char* weekdayJp;
    const char* timeStr;
    const char* lastUpdate;
    bool wifiOk;
    bool ntpOk;

    float tempC;
    int   humidity;
    int   pressureHPa;
    int   pressureTrend3H;

    CalendarEvent events[3];
    int eventCount;
};

// ========================================================
// WiFi (WPA2 Enterprise) 接続
// ========================================================
bool connectWifiEnterprise() {
    Serial.printf("[WiFi] Connecting to %s (WPA2-Enterprise)...\n", WIFI_SSID);

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_STA);

#if EAP_NEW_API
    // Arduino-ESP32 3.x 新API
    esp_eap_client_set_identity((const uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_eap_client_set_username((const uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_eap_client_set_password((const uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_enterprise_enable();
#else
    // Arduino-ESP32 2.x 旧API
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_wpa2_ent_enable();
#endif

    WiFi.begin(WIFI_SSID);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.printf("\n[WiFi] timeout (%d s)\n", WIFI_TIMEOUT_MS / 1000);
            return false;
        }
        Serial.print(".");
        delay(250);
    }
    Serial.printf("\n[WiFi] connected. IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
}

// ========================================================
// NTP 時刻同期 (JST)
// ========================================================
bool syncNtpTime() {
    Serial.printf("[NTP] sync %s (TZ=%s)...\n", NTP_SERVER, NTP_TZ);
    configTzTime(NTP_TZ, NTP_SERVER);

    struct tm tinfo;
    uint32_t start = millis();
    while (!getLocalTime(&tinfo, 100)) {
        if (millis() - start > NTP_TIMEOUT_MS) {
            Serial.println("[NTP] timeout");
            return false;
        }
    }
    Serial.printf("[NTP] OK %04d-%02d-%02d %02d:%02d:%02d (wday=%d)\n",
                  tinfo.tm_year + 1900, tinfo.tm_mon + 1, tinfo.tm_mday,
                  tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec, tinfo.tm_wday);
    return true;
}

// ========================================================
// 描画ヘルパ
// ========================================================
static void u8gText(int16_t x, int16_t y_top,
                    const uint8_t* font, const char* utf8, EpdColor c) {
    u8g2.setFont(font);
    u8g2.setForegroundColor(c);
    u8g2.setFontMode(1);
    u8g2.setFontDirection(0);
    int16_t baseline = y_top + u8g2.getFontAscent();
    u8g2.setCursor(x, baseline);
    u8g2.print(utf8);
}

static void dottedVLine(int16_t x, int16_t y1, int16_t y2,
                        EpdColor c = COL_BLACK, int step = 4) {
    for (int y = y1; y < y2; y += step) gfx.drawPixel(x, y, c);
}

// ========================================================
// Layout C v2
// ========================================================
void drawLayoutC(const DisplayData& d) {
    gfx.clearBuffer();

    const int16_t CENTER = EPD_WIDTH / 2;  // 396
    dottedVLine(CENTER, 8, EPD_HEIGHT - 8, COL_BLACK, 4);

    // ===== 左半分 =====
    const int16_t LP = 18;

    char dateBuf[24];
    snprintf(dateBuf, sizeof(dateBuf), "%s %d月%d日", d.weekdayJp, d.month, d.day);
    u8gText(LP, 6, u8g2_font_b16_b_t_japanese1, dateBuf, COL_RED);

    u8gText(LP, 32, u8g2_font_logisoso92_tn, d.timeStr, COL_BLACK);

    // 気温
    char tempBuf[8];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", d.tempC);
    u8gText(LP, 148, u8g2_font_logisoso50_tn, tempBuf, COL_BLACK);
    u8gText(LP + 158, 168, u8g2_font_b16_b_t_japanese1, "℃", COL_BLACK);

    // 湿度・気圧
    const int16_t SX = 215;
    char humBuf[6], presBuf[6];
    snprintf(humBuf,  sizeof(humBuf),  "%d", d.humidity);
    snprintf(presBuf, sizeof(presBuf), "%d", d.pressureHPa);
    u8gText(SX,       150, u8g2_font_b16_b_t_japanese1, "湿度", COL_BLACK);
    u8gText(SX + 50,  144, u8g2_font_logisoso26_tn, humBuf, COL_BLACK);
    u8gText(SX + 100, 152, u8g2_font_helvB14_tf,    "%",    COL_BLACK);
    u8gText(SX,       188, u8g2_font_b16_b_t_japanese1, "気圧", COL_BLACK);
    u8gText(SX + 50,  184, u8g2_font_logisoso24_tn, presBuf, COL_BLACK);
    u8gText(SX + 122, 192, u8g2_font_helvB10_tf,    "hPa",   COL_BLACK);

    // フッター
    char lastUpd[24];
    if (d.wifiOk && d.ntpOk) {
        snprintf(lastUpd, sizeof(lastUpd), "更新 %s", d.lastUpdate);
        u8gText(LP, 240, u8g2_font_b16_b_t_japanese1, lastUpd, COL_BLACK);
    } else {
        // 状態表示
        const char* msg = !d.wifiOk ? "WiFi未接続" : "NTP未同期";
        u8gText(LP, 240, u8g2_font_b16_b_t_japanese1, msg, COL_RED);
    }
    if (d.pressureTrend3H <= -2) {
        u8gText(LP + 135, 240, u8g2_font_b16_b_t_japanese1,
                "↓気圧 低下中", COL_RED);
    }

    // ===== 右半分 =====
    const int16_t R = CENTER + 12;
    u8gText(R, 6, u8g2_font_b16_b_t_japanese1, "今日の予定", COL_RED);

    if (d.eventCount == 0) {
        u8gText(R, 50, u8g2_font_b16_b_t_japanese1, "予定なし", COL_BLACK);
        return;
    }

    // 最初のイベント
    {
        const auto& e = d.events[0];
        EpdColor c = e.urgent ? COL_RED : COL_BLACK;
        u8gText(R,        38, u8g2_font_logisoso30_tn,    e.time,  c);
        u8gText(R + 110,  44, u8g2_font_b16_b_t_japanese1, e.title, c);
        if (e.location && e.location[0]) {
            char locBuf[40];
            snprintf(locBuf, sizeof(locBuf), "@ %s", e.location);
            u8gText(R + 110, 78, u8g2_font_b16_b_t_japanese1, locBuf, COL_BLACK);
        }
    }

    int16_t y = 120;
    for (int i = 1; i < d.eventCount && i < 3; i++) {
        const auto& e = d.events[i];
        u8gText(R,       y,     u8g2_font_logisoso24_tn,    e.time,  COL_BLACK);
        u8gText(R + 90,  y + 4, u8g2_font_b16_b_t_japanese1, e.title, COL_BLACK);
        if (e.location && e.location[0]) {
            u8gText(R + 90, y + 30, u8g2_font_helvB08_tf, e.location, COL_BLACK);
        }
        y += 60;
    }
}

// ========================================================
// データ充填
// ========================================================
static const char* WEEKDAY_JP[] = { "日","月","火","水","木","金","土" };

void fillTimeData(DisplayData& d) {
    struct tm t;
    if (getLocalTime(&t, 100)) {
        static char timeBuf[8];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
        d.timeStr   = timeBuf;
        d.year      = t.tm_year + 1900;
        d.month     = t.tm_mon + 1;
        d.day       = t.tm_mday;
        d.weekdayJp = WEEKDAY_JP[t.tm_wday];
        d.lastUpdate = timeBuf;
        d.ntpOk = true;
    } else {
        d.timeStr    = "--:--";
        d.year       = 2026;
        d.month      = 1;
        d.day        = 1;
        d.weekdayJp  = "?";
        d.lastUpdate = "--:--";
        d.ntpOk = false;
    }
}

void fillStubSensorAndCalendar(DisplayData& d) {
    // TODO: BME280 実測値に置き換え
    d.tempC          = 22.8f;
    d.humidity       = 48;
    d.pressureHPa    = 1013;
    d.pressureTrend3H = 0;

    // TODO: Google Calendar 実データに置き換え
    d.eventCount = 0;
}

// ========================================================
// Arduino entry
// ========================================================
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) delay(10);
    Serial.println("\n=== E-ink Smart Display (WiFi+NTP+Layout C v2) ===");

    DisplayData data = {};

    // 1) WiFi 接続
    data.wifiOk = connectWifiEnterprise();

    // 2) NTP 同期
    if (data.wifiOk) {
        data.ntpOk = syncNtpTime();
    } else {
        data.ntpOk = false;
    }

    // 3) WiFi 切断 (省電力)
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);

    // 4) e-Paper 初期化
    if (epd.Init() != 0) {
        Serial.println("[EPD] init failed");
        return;
    }
    u8g2.begin(gfx);
    u8g2.setFontMode(1);
    u8g2.setFontDirection(0);

    // 5) データ詰め替え
    fillTimeData(data);
    fillStubSensorAndCalendar(data);

    // 6) 描画 → 表示 → スリープ
    Serial.println("[EPD] drawing...");
    drawLayoutC(data);

    Serial.println("[EPD] updating (~24s)...");
    epd.Display(blackBuf, redBuf);

    Serial.println("[EPD] sleep");
    epd.Sleep();

    // 7) Deep-sleep (15分後 wake up)
    // esp_sleep_enable_timer_wakeup(15ULL * 60 * 1000000);
    // esp_deep_sleep_start();
}

void loop() {}
