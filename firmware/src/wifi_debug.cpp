#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "esp_wifi.h"
#if WIFI_USE_ENTERPRISE
  #include "esp_eap_client.h"
#endif

// ── 試したい組み合わせを切り替える ───────────────────────────────────────────
// VARIANT | identity                      | username                      | method
//   0     | sd23095                       | sd23095                       | PEAP
//   1     | sd23095@toyota-ti.ac.jp       | sd23095                       | PEAP
//   2     | anonymous@toyota-ti.ac.jp     | sd23095                       | PEAP
//   3     | sd23095@toyota-ti.ac.jp       | sd23095@toyota-ti.ac.jp       | PEAP  ← NPS で最多
//   4     | sd23095@toyota-ti.ac.jp       | sd23095                       | TTLS+MSCHAPv2
//   5     | sd23095@toyota-ti.ac.jp       | sd23095                       | 自動交渉
#define IDENTITY_VARIANT 3

// ── identity ─────────────────────────────────────────────────────────────────
#if   IDENTITY_VARIANT == 0
  #define TEST_IDENTITY EAP_USERNAME
#elif IDENTITY_VARIANT == 1
  #define TEST_IDENTITY EAP_USERNAME "@" EAP_DOMAIN
#elif IDENTITY_VARIANT == 2
  #define TEST_IDENTITY "anonymous@" EAP_DOMAIN
#elif IDENTITY_VARIANT == 3
  #define TEST_IDENTITY EAP_USERNAME "@" EAP_DOMAIN
#elif IDENTITY_VARIANT == 4
  #define TEST_IDENTITY EAP_USERNAME "@" EAP_DOMAIN
#elif IDENTITY_VARIANT == 5
  #define TEST_IDENTITY EAP_USERNAME "@" EAP_DOMAIN
#endif

// ── inner username ────────────────────────────────────────────────────────────
#if IDENTITY_VARIANT == 3
  #define TEST_USERNAME EAP_USERNAME "@" EAP_DOMAIN
#else
  #define TEST_USERNAME EAP_USERNAME
#endif

static void print_status(wl_status_t s) {
    const char* names[] = {
        "IDLE","NO_SSID","SCAN_DONE","CONNECTED",
        "FAILED","CONN_LOST","?","DISCONNECTED"
    };
    int idx = (int)s;
    if (idx >= 0 && idx < 8) Serial.printf("  %s(%d)\n", names[idx], idx);
    else                      Serial.printf("  status=%d\n", idx);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== WiFi Debug ===");

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

#ifdef WIFI_FIXED_MAC
    {
        uint8_t m[] = WIFI_FIXED_MAC;
        esp_wifi_set_mac(WIFI_IF_STA, m);
    }
#endif

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    Serial.printf("MAC     : %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    Serial.printf("SSID    : %s\n", WIFI_SSID);

#if WIFI_USE_ENTERPRISE
    const char* method_str =
        (IDENTITY_VARIANT == 4) ? "TTLS+MSCHAPv2" :
        (IDENTITY_VARIANT == 5) ? "AUTO" : "PEAP";
    Serial.printf("VARIANT : %d  method=%s\n", IDENTITY_VARIANT, method_str);
    Serial.printf("identity: %s\n", TEST_IDENTITY);
    Serial.printf("username: %s\n", TEST_USERNAME);

    esp_eap_client_set_disable_time_check(true);
    esp_eap_client_use_default_cert_bundle(false);

#if IDENTITY_VARIANT == 4
    esp_eap_client_set_eap_methods(ESP_EAP_TYPE_TTLS);
    esp_eap_client_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_MSCHAPV2);
#elif IDENTITY_VARIANT == 5
    // メソッド制限なし — サーバーに交渉させる
#else
    esp_eap_client_set_eap_methods(ESP_EAP_TYPE_PEAP);
#endif

    esp_eap_client_set_identity((uint8_t*)TEST_IDENTITY, strlen(TEST_IDENTITY));
    esp_eap_client_set_username((uint8_t*)TEST_USERNAME, strlen(TEST_USERNAME));
    esp_eap_client_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_enterprise_enable();
    WiFi.begin(WIFI_SSID);
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    unsigned long start = millis();
    wl_status_t prev = (wl_status_t)-1;
    while (millis() - start < 20000) {
        wl_status_t cur = WiFi.status();
        if (cur != prev) {
            Serial.printf("[%5lus]", (millis() - start) / 1000);
            print_status(cur);
            prev = cur;
        }
        if (cur == WL_CONNECTED) break;
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("OK — IP: %s  RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.println("FAILED");
    }
}

void loop() {
    delay(5000);
    Serial.printf("[keep] ");
    print_status(WiFi.status());
}
