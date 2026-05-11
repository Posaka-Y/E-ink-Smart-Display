#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include <gdey3c/GxEPD2_579c_GDEY0579Z93.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "config.h"

GxEPD2_3C<GxEPD2_579c_GDEY0579Z93, GxEPD2_579c_GDEY0579Z93::HEIGHT> display(
    GxEPD2_579c_GDEY0579Z93(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

static void drawCentered(const char* s, int16_t x, int16_t w, int16_t y) {
    int16_t bx, by; uint16_t bw, bh;
    display.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
    display.setCursor(x + (w - (int16_t)bw) / 2 - bx, y);
    display.print(s);
}

void setup() {
    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println("[layout_a] start");

    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(100);

    SPI.begin(EPD_SCK_PIN, /*MISO*/-1, EPD_MOSI_PIN, EPD_CS_PIN);
    display.init(115200);
    display.setRotation(0);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // ── Structure lines ──────────────────────────────────────────────────
        display.drawLine(340,   0, 340, 271, GxEPD_BLACK);
        display.drawLine(  0,  28, 339,  28, GxEPD_BLACK);
        display.drawLine(342,  80, 791,  80, GxEPD_BLACK);
        display.drawLine(342, 164, 791, 164, GxEPD_BLACK);
        display.drawLine(492,   0, 492,  79, GxEPD_BLACK);
        display.drawLine(642,   0, 642,  79, GxEPD_BLACK);

        // ── Left header ──────────────────────────────────────────────────────
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(12, 22);
        display.print("2026 APR");

        display.fillRect(264, 4, 71, 21, GxEPD_RED);
        display.setTextColor(GxEPD_WHITE);
        drawCentered("TUE", 264, 71, 22);

        // ── Clock ────────────────────────────────────────────────────────────
        display.setFont(&FreeSansBold24pt7b);
        display.setTextColor(GxEPD_RED);
        drawCentered("14:32", 0, 340, 140);

        // ── Date ─────────────────────────────────────────────────────────────
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(90, 210);
        display.print("28");
        display.setFont(&FreeSans9pt7b);
        display.setCursor(140, 210);
        display.print("/ 04");

        // ── Sensors ──────────────────────────────────────────────────────────
        display.setTextColor(GxEPD_BLACK);

        display.setFont(&FreeSans9pt7b);
        drawCentered("TEMP", 342, 150, 22);
        display.setFont(&FreeSansBold12pt7b);
        drawCentered("22.3", 342, 150, 58);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(455, 70);
        display.print("\xB0""C");

        display.setFont(&FreeSans9pt7b);
        drawCentered("HUM", 492, 150, 22);
        display.setFont(&FreeSansBold12pt7b);
        drawCentered("58", 492, 150, 58);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(580, 70);
        display.print("%");

        display.setFont(&FreeSans9pt7b);
        drawCentered("PRES", 642, 150, 22);
        display.setFont(&FreeSansBold12pt7b);
        drawCentered("1013", 642, 150, 58);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(735, 70);
        display.print("hPa");

        // ── Week strip ───────────────────────────────────────────────────────
        static const char* const DAY3[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
        static const int  DNUM[]        = {  27,   28,   29,   30,    1,    2,    3 };
        static const bool DEVT[]        = {false, true, false, false, true, false, false};

        for (int i = 0; i < 7; i++) {
            int x0 = 342 + i * 64;
            bool today = (i == 1);
            if (i > 0) display.drawLine(x0, 84, x0, 163, GxEPD_BLACK);
            if (today) display.drawRect(x0 + 1, 85, 62, 78, GxEPD_BLACK);

            display.setFont(&FreeSans9pt7b);
            display.setTextColor(GxEPD_BLACK);
            drawCentered(DAY3[i], x0, 64, 100);

            char ds[4]; snprintf(ds, sizeof(ds), "%d", DNUM[i]);
            display.setFont(&FreeSansBold12pt7b);
            drawCentered(ds, x0, 64, 128);

            if (DEVT[i])
                display.fillRect(x0 + 30, 148, 4, 4, GxEPD_RED);
        }

        // ── Event list ───────────────────────────────────────────────────────
        struct Ev { const char* dt; const char* title; bool today; };
        static const Ev EVTS[] = {
            {"4/28 10:00", "Team Meeting",   true },
            {"4/29 14:30", "Dentist",        false},
            {"5/1  09:00", "Monthly Review", false},
        };
        for (int i = 0; i < 3; i++) {
            int y0 = 168 + i * 34;
            uint16_t col = EVTS[i].today ? GxEPD_RED : GxEPD_BLACK;
            display.fillRect(344, y0 + 3, 3, 28, col);
            display.setFont(&FreeSans9pt7b);
            display.setTextColor(GxEPD_BLACK);
            display.setCursor(350, y0 + 16);
            display.print(EVTS[i].dt);
            display.setCursor(350, y0 + 30);
            display.print(EVTS[i].title);
        }

    } while (display.nextPage());

    display.hibernate();
    digitalWrite(EPD_PWR_PIN, LOW);
    Serial.println("[epd] done");
}

void loop() {}
