#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "config.h"
#include "esp_wifi.h"
#if WIFI_USE_ENTERPRISE
  #include "esp_eap_client.h"
#endif
#include "googlecal.h"

static void wifi_connect()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

#ifdef WIFI_FIXED_MAC
    {
        uint8_t fixed_mac[] = WIFI_FIXED_MAC;
        esp_wifi_set_mac(WIFI_IF_STA, fixed_mac);
    }
#endif

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    Serial.printf("[wifi] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    Serial.printf("[wifi] Connecting to %s", WIFI_SSID);

#if WIFI_USE_ENTERPRISE
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
        delay(500); Serial.print("."); retries++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[wifi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.printf("[wifi] Failed (status=%d)\n", WiFi.status());
}

void setup()
{
    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println("\n[cal_debug] start");

    wifi_connect();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[cal_debug] WiFi failed, abort");
        return;
    }

    // ── Step 1: Fetch calendar events ──────────────────────────────────────
    CalEvent events[CAL_MAX_EVENTS];
    int count = googlecal_fetch(GCAL_ENDPOINT, events);

    Serial.printf("\n── gcal result (%d events) ──\n", count);
    for (int i = 0; i < count; i++)
        Serial.printf("  [%d] %s  |  %s  allDay=%d\n",
                      i, events[i].title, events[i].time, events[i].allDay);

    Serial.println("\n[cal_debug] done — reset to run again");
}

void loop() {}
