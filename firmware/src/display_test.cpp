#include <Arduino.h>
#include "DEV_Config.h"
#include "EPD_5in79b.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "font64.h"

#define FRAME_SIZE ((EPD_5in79b_WIDTH * EPD_5in79b_HEIGHT) / 8)
static UBYTE bwImage[FRAME_SIZE];
static UBYTE redImage[FRAME_SIZE];

void setup() {
    Serial.begin(115200);
    // USB CDC: wait until host connects (up to 3s)
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println("[display_test] start");

    memset(bwImage,  0xFF, FRAME_SIZE);
    memset(redImage, 0xFF, FRAME_SIZE);

    // ── BW layer ──────────────────────────────────────────────────────────────
    Paint_NewImage(bwImage, EPD_5in79b_WIDTH, EPD_5in79b_HEIGHT, ROTATE_0, WHITE);
    Paint_SetScale(2);

    Paint_DrawString_EN(10,  8, "EPD TEST",                    &Font64, BLACK, WHITE);
    Paint_DrawString_EN(10, 82, "Waveshare 5.79\" 792x272 3-color", &Font16, BLACK, WHITE);
    Paint_DrawLine(10, 106, 781, 106, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(10, 116, "BW: OK",  &Font24, BLACK, WHITE);
    Paint_DrawString_EN(10, 148, "RED: OK", &Font24, BLACK, WHITE);

    // ── Red layer — filled rectangle next to the "RED: OK" label ─────────────
    Paint_SelectImage(redImage);
    Paint_DrawRectangle(140, 148, 380, 200, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // ── Init display and push ─────────────────────────────────────────────────
    DEV_Module_Init();
    if (EPD_5in79b_Init() != 0) {
        Serial.println("[epd] init FAILED");
        return;
    }
    Serial.println("[epd] displaying... (~15s for red)");
    EPD_5in79b_Display(bwImage, redImage);
    EPD_5in79b_Sleep();
    DEV_Module_Exit();
    Serial.println("[epd] done");
}

void loop() {}
