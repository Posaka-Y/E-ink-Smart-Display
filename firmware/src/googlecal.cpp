/*
 * googlecal.cpp — Google Calendar via Apps Script JSON proxy
 *
 * Setup:
 *   1. Create a Google Apps Script project at script.google.com
 *   2. Paste the following code and deploy as web app (Anyone can access):
 *
 *   function doGet(e) {
 *     var cal = CalendarApp.getDefaultCalendar();
 *     var now = new Date();
 *     var end = new Date(now); end.setHours(23,59,59,999);
 *     var events = cal.getEvents(now, end);
 *     var result = events.slice(0,4).map(function(ev) {
 *       var allDay = ev.isAllDayEvent();
 *       return {
 *         title: ev.getTitle(),
 *         time:  allDay ? "終日" :
 *                Utilities.formatDate(ev.getStartTime(),"Asia/Tokyo","HH:mm"),
 *         allDay: allDay
 *       };
 *     });
 *     return ContentService.createTextOutput(JSON.stringify(result))
 *       .setMimeType(ContentService.MimeType.JSON);
 *   }
 *
 *   3. Copy the deployment URL into config.h as GCAL_ENDPOINT
 */

#include "googlecal.h"
#include <ArduinoJson.h>

int googlecal_fetch(const char* endpoint, CalEvent events[CAL_MAX_EVENTS])
{
    // Clear all slots
    for (int i = 0; i < CAL_MAX_EVENTS; i++) {
        events[i].valid = false;
        events[i].title[0] = '\0';
        events[i].time[0]  = '\0';
        events[i].allDay   = false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[gcal] WiFi not connected");
        return 0;
    }

    HTTPClient http;
    http.begin(endpoint);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[gcal] HTTP error %d\n", code);
        http.end();
        return 0;
    }

    String body = http.getString();
    http.end();

    // Parse JSON array: [{title,time,allDay}, ...]
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[gcal] JSON parse error: %s\n", err.c_str());
        return 0;
    }

    JsonArray arr = doc.as<JsonArray>();
    int count = 0;
    for (JsonObject obj : arr) {
        if (count >= CAL_MAX_EVENTS) break;
        const char* title  = obj["title"] | "";
        const char* time   = obj["time"]  | "";
        bool allDay        = obj["allDay"] | false;

        strncpy(events[count].title, title, sizeof(events[count].title) - 1);
        events[count].title[sizeof(events[count].title)-1] = '\0';
        strncpy(events[count].time, time, sizeof(events[count].time) - 1);
        events[count].time[sizeof(events[count].time)-1] = '\0';
        events[count].allDay = allDay;
        events[count].valid  = true;
        count++;
    }

    Serial.printf("[gcal] Fetched %d events\n", count);
    return count;
}
