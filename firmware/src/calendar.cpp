/*
 * calendar.cpp — Dynamic calendar event layout
 *
 * Layout rules:
 *   0 events → "予定なし" centered in panel
 *   1 event  → full panel height (272px)
 *   2 events → 136px each
 *   3 events → ~90px each
 *   4 events → 68px each
 *
 * Each event row:
 *   ┌─────────────────────────────────┐
 *   │ [time]  Title text              │
 *   │ (horizontal separator)          │
 *   └─────────────────────────────────┘
 *   - Time drawn in RED buffer (Font12)
 *   - Title drawn in BLACK buffer (Font16 or Font12 depending on row height)
 *   - Separator line in BLACK buffer between rows
 */

#include "calendar.h"
#include "fonts.h"

// Helper: pick font based on available row height
static sFONT* pick_font(int row_h)
{
    if (row_h >= 40) return &Font20;
    if (row_h >= 28) return &Font16;
    return &Font12;
}

// Draw a single event row into both BW and Red buffers
static void draw_event_row(UBYTE *bwImage, UBYTE *redImage,
                            int x0, int y0, int w, int h,
                            const CalEvent& ev)
{
    // --- Black buffer: title ---
    Paint_SelectImage(bwImage);
    sFONT* titleFont = pick_font(h);
    int ty = y0 + CAL_PAD_Y;
    // Leave room for the time label (Font12 width * 5 chars + padding)
    int title_x = x0 + CAL_PAD_X + Font12.Width * 6;
    // Truncate title to available width
    int max_chars = (w - title_x + x0 - CAL_PAD_X) / titleFont->Width;
    if (max_chars < 1) max_chars = 1;
    char title_buf[64];
    strncpy(title_buf, ev.title, sizeof(title_buf)-1);
    title_buf[sizeof(title_buf)-1] = '\0';
    if ((int)strlen(title_buf) > max_chars) {
        title_buf[max_chars - 1] = '.';
        title_buf[max_chars]     = '\0';
    }
    Paint_DrawString_EN(title_x, ty, title_buf, titleFont, BLACK, WHITE);

    // --- Red buffer: time label ---
    Paint_SelectImage(redImage);
    if (ev.allDay) {
        Paint_DrawString_EN(x0 + CAL_PAD_X, ty, "ALL", &Font12, BLACK, WHITE);
    } else {
        Paint_DrawString_EN(x0 + CAL_PAD_X, ty, ev.time, &Font12, BLACK, WHITE);
    }

    // --- Black buffer: horizontal separator at bottom of row ---
    Paint_SelectImage(bwImage);
    if (y0 + h < CAL_Y0 + CAL_H) {  // don't draw after last row
        Paint_DrawLine(x0, y0 + h - 1, x0 + w - 1, y0 + h - 1,
                       BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
}

void calendar_draw(UBYTE *bwImage, UBYTE *redImage,
                   const CalEvent events[CAL_MAX_EVENTS], int eventCount)
{
    // Draw panel left border in black
    Paint_SelectImage(bwImage);
    Paint_DrawLine(CAL_X0, CAL_Y0, CAL_X0, CAL_Y0 + CAL_H - 1,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    if (eventCount <= 0) {
        // No events — show placeholder
        Paint_SelectImage(bwImage);
        const char* msg = "No events today";
        int tx = CAL_X0 + (CAL_W - (int)strlen(msg) * Font16.Width) / 2;
        int ty = CAL_Y0 + (CAL_H - Font16.Height) / 2;
        if (tx < CAL_X0 + CAL_PAD_X) tx = CAL_X0 + CAL_PAD_X;
        Paint_DrawString_EN(tx, ty, msg, &Font16, BLACK, WHITE);
        return;
    }

    // Calculate row height
    int row_h = CAL_H / eventCount;

    for (int i = 0; i < eventCount; i++) {
        if (!events[i].valid) continue;
        int y0 = CAL_Y0 + i * row_h;
        // Last row gets any remainder pixels
        int h  = (i == eventCount - 1) ? (CAL_Y0 + CAL_H - y0) : row_h;
        draw_event_row(bwImage, redImage,
                       CAL_X0 + 1, y0, CAL_W - 1, h,
                       events[i]);
    }

    // Restore to BW image after all drawing
    Paint_SelectImage(bwImage);
}
