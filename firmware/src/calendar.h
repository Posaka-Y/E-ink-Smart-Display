#pragma once
#include <Arduino.h>
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "googlecal.h"

/*
 * calendar.h — Dynamic event layout for the right panel of the display
 *
 * Display area (right panel): x=270..791, y=0..271  (522 x 272 px)
 * Panels auto-scale: 0 events → placeholder, 1-4 events → equal height slots
 *
 * Colors: title in BLACK, time in RED (on the red image buffer)
 */

// Right panel geometry
#define CAL_X0      270   // left edge of calendar panel
#define CAL_Y0        0   // top edge
#define CAL_W       522   // panel width  (792 - 270)
#define CAL_H       272   // panel height

// Padding inside each event row
#define CAL_PAD_X    10
#define CAL_PAD_Y     6

// Draw all events into the BW and Red image buffers.
// Call after Paint_SelectImage() is NOT needed — this function
// switches buffers internally.
void calendar_draw(UBYTE *bwImage, UBYTE *redImage,
                   const CalEvent events[CAL_MAX_EVENTS], int eventCount);
