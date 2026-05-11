#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Maximum number of calendar events to fetch and display
#define CAL_MAX_EVENTS 4

// A single calendar event
struct CalEvent {
    char title[64] = {};
    char time[12]  = {};
    bool allDay    = false;
    bool valid     = false;
};

// Fetch today's events from Google Apps Script JSON proxy.
// endpoint: full URL of deployed Apps Script web app
// returns number of events fetched (0..CAL_MAX_EVENTS)
int googlecal_fetch(const char* endpoint, CalEvent events[CAL_MAX_EVENTS]);
