/*
 * googlecal.cpp — Google Calendar via Apps Script JSON proxy
 *
 * ═══════════════════════════════════════════════════════════════════
 *  Apps Script の作り方（script.google.com で新規プロジェクト）
 * ═══════════════════════════════════════════════════════════════════
 *
 *  以下のコードを貼り付けて「デプロイ → 新しいデプロイ」:
 *    種類          : ウェブアプリ
 *    次のユーザーとして実行 : 自分
 *    アクセスできるユーザー : 全員
 *
 * -------------------------------------------------------------------
 *  function doGet(e) {
 *    try {
 *      var tz   = "Asia/Tokyo";
 *      var now  = new Date();
 *      var end  = new Date(now.getFullYear(), now.getMonth(), now.getDate(), 23, 59, 59);
 *      var cal  = CalendarApp.getDefaultCalendar();
 *      var evts = cal.getEvents(now, end);
 *
 *      var result = evts.slice(0, 4).map(function(ev) {
 *        var allDay = ev.isAllDayEvent();
 *        var rawTitle = ev.getTitle();
 *        var engTitle = LanguageApp.translate(rawTitle, "ja", "en");
 *        if (engTitle.length > 15) engTitle = engTitle.substring(0, 15);
 *        return {
 *          title:  engTitle,
 *          time:   allDay ? "ALL"
 *                         : Utilities.formatDate(ev.getStartTime(), tz, "HH:mm"),
 *          allDay: allDay
 *        };
 *      });
 *
 *      var out = ContentService.createTextOutput(JSON.stringify(result));
 *      out.setMimeType(ContentService.MimeType.JSON);
 *      return out;
 *    } catch(err) {
 *      var out = ContentService.createTextOutput(JSON.stringify({error: err.message}));
 *      out.setMimeType(ContentService.MimeType.JSON);
 *      return out;
 *    }
 *  }
 * -------------------------------------------------------------------
 *
 *  デプロイ後に表示される URL（/exec で終わるもの）を
 *  config.h の GCAL_ENDPOINT に設定する。
 *
 *  ※ /dev URL は認証が必要なので NG。必ず /exec URL を使う。
 *  ※ 初回アクセス時に「カレンダーへのアクセス許可」が求められる。
 *     ブラウザでその URL を一度開いて許可しておくこと。
 * ═══════════════════════════════════════════════════════════════════
 */

#include "googlecal.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

int googlecal_fetch(const char* endpoint, CalEvent events[CAL_MAX_EVENTS])
{
    for (int i = 0; i < CAL_MAX_EVENTS; i++) {
        events[i] = CalEvent{};   // zero-init
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[gcal] WiFi not connected");
        return 0;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, endpoint);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[gcal] HTTP %d\n", code);
        http.end();
        return 0;
    }

    String body = http.getString();
    http.end();
    Serial.printf("[gcal] body: %s\n", body.c_str());

    // エラーオブジェクト検知 {"error": "..."}
    if (body.startsWith("{")) {
        Serial.println("[gcal] Script returned an error object");
        return 0;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[gcal] JSON parse error: %s\n", err.c_str());
        return 0;
    }

    if (!doc.is<JsonArray>()) {
        Serial.println("[gcal] Response is not a JSON array");
        return 0;
    }

    int count = 0;
    for (JsonObject obj : doc.as<JsonArray>()) {
        if (count >= CAL_MAX_EVENTS) break;
        const char* title    = obj["title"] | "";
        const char* startStr = obj["start"] | (obj["time"] | "");

        // allDay: "start" has no 'T' (date-only) or legacy "ALL"
        bool allDay = (strchr(startStr, 'T') == nullptr);

        char timeStr[12] = "ALL";
        if (!allDay && strlen(startStr) >= 16) {
            // Extract HH:mm from "YYYY-MM-DDTHH:MM:SS+09:00"
            strncpy(timeStr, startStr + 11, 5);
            timeStr[5] = '\0';
        }

        strncpy(events[count].title, title,   sizeof(events[count].title) - 1);
        strncpy(events[count].time,  timeStr, sizeof(events[count].time)  - 1);
        events[count].allDay = allDay;
        events[count].valid  = true;
        count++;
    }

    Serial.printf("[gcal] Fetched %d events\n", count);
    return count;
}
