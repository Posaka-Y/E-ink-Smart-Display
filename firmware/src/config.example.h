#pragma once

// Copy this file to src/config.h and fill in local credentials.
// Keep src/config.h out of Git because it contains Wi-Fi/API secrets.

// Pin assignments
// Default target: ESP32-S3 DevKitC-1
// Alternative targets:
//   - ESP32 DevKit when BOARD_ESP32_DEVKIT is defined
//   - ESP32-C3 SuperMini when BOARD_ESP32C3_SUPERMINI is defined
#if defined(BOARD_ESP32S3_DEVKITC1)
  // ESP32-S3 DevKitC-1
  // Avoid boot strapping pins: GPIO0, GPIO3, GPIO45, GPIO46
  // Avoid native USB pins on many S3 boards: GPIO19, GPIO20
  #define EPD_SCK_PIN     12   // CLK
  #define EPD_MOSI_PIN    11   // DIN
  #define EPD_CS_PIN      10   // CS
  #define EPD_DC_PIN       9   // DC
  #define EPD_RST_PIN     14   // RST
  #define EPD_BUSY_PIN    13   // BUSY
  #define EPD_PWR_PIN     21   // PWR, HIGH=display on, LOW=display off

  #define I2C_SDA_PIN      8   // SDA
  #define I2C_SCL_PIN     18   // SCL

  #define PIR_PIN          4   // PIR OUT
#elif defined(BOARD_ESP32_DEVKIT)
  // ESP32 DevKit / ESP32-WROOM
  // Avoid boot strapping pins: GPIO0, GPIO2, GPIO4, GPIO5, GPIO12, GPIO15
  // Avoid flash pins: GPIO6-GPIO11
  // GPIO34-GPIO39 are input-only; PIR can use GPIO34 if the module drives it.
  #define EPD_SCK_PIN     18   // CLK
  #define EPD_MOSI_PIN    23   // DIN
  #define EPD_CS_PIN      27   // CS
  #define EPD_DC_PIN      26   // DC
  #define EPD_RST_PIN     25   // RST
  #define EPD_BUSY_PIN    33   // BUSY
  #define EPD_PWR_PIN     32   // PWR, HIGH=display on, LOW=display off

  #define I2C_SDA_PIN     21   // SDA
  #define I2C_SCL_PIN     22   // SCL

  #define PIR_PIN         34   // PIR OUT, input-only, ext1 wake capable
#else
  // ESP32-C3 SuperMini
  // E-ink display (software SPI / bit-bang)
  #define EPD_SCK_PIN      6   // CLK
  #define EPD_MOSI_PIN     7   // DIN
  #define EPD_CS_PIN      10   // CS
  #define EPD_DC_PIN       5   // DC
  #define EPD_RST_PIN      3   // RST
  #define EPD_BUSY_PIN     2   // BUSY
  #define EPD_PWR_PIN      8   // PWR, HIGH=display on, LOW=display off

  #define I2C_SDA_PIN     21   // SDA
  #define I2C_SCL_PIN     20   // SCL

  #define PIR_PIN          1   // PIR OUT
#endif

// Wi-Fi
// 0 = WPA2 personal / 1 = WPA2-Enterprise (EAP / PEAP-MSCHAPv2)
#define WIFI_USE_ENTERPRISE  1

#define WIFI_SSID     "your_ssid_here"
#define WIFI_PASSWORD "your_wifi_password_here"  // unused when WIFI_USE_ENTERPRISE=1

// WPA2-Enterprise credentials, used only when WIFI_USE_ENTERPRISE=1
#define EAP_IDENTITY  "anonymous@example.com"
#define EAP_USERNAME  "your_enterprise_username"
#define EAP_PASSWORD  "your_enterprise_password"
#define EAP_DOMAIN    "example.com"

// Optional fixed MAC address.
// Locally administered unicast MACs should have first byte bit0=0 and bit1=1.
#define WIFI_FIXED_MAC  {0x02, 0xAB, 0xCD, 0xEF, 0x12, 0x34}

// Gemini Flash API
#define GEMINI_API_KEY "your_gemini_api_key_here"

// Google Calendar (Apps Script JSON proxy)
// Deploy the Apps Script web app and paste the /exec URL here.
#define GCAL_ENDPOINT "https://script.google.com/macros/s/your_deployment_id/exec"

// NTP
#define NTP_SERVER    "ntp.nict.jp"
#define NTP_OFFSET    32400   // JST = UTC+9 (seconds)
#define NTP_INTERVAL  3600000 // sync every 1 hour (ms)

// Display update interval
#define SLEEP_SECONDS  60

// Sensor I2C
#define BMP280_ADDR  0x77    // SDO->GND=0x76, SDO->VCC=0x77
