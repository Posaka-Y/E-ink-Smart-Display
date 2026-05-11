# E-ink Smart Display — Project Context for Claude Code

## ハード構成

| コンポーネント | 詳細 |
|---|---|
| MCU | ESP32 DevKit / ESP32-WROOM（ESP32-S3 DevKitC-1 環境も代替として保持。ESP32-C3 SuperMini は Wi-Fi 不安定のため非推奨） |
| ディスプレイ | Waveshare 5.79inch e-Paper (B) — 792×272px, 3色 (白/黒/赤) |
| 温湿度・気圧センサ | AHT20 (0x38) + BMP280 (0x77)、I2C接続 |
| 人感センサ | PIR 3ピン、しきい値可変抵抗つき、デジタル出力 |
| 通信 | Wi-Fi (HTTP/HTTPS) |

## ディスプレイ仕様

- **型番**: Waveshare 5.79inch e-Paper (B) Module
- **解像度**: 792 × 272 px
- **色数**: 3色（黒・白・赤）
- **インターフェース**: SPI
- **部分更新**: 非対応（フル更新のみ）
- **推奨ライブラリ**: GxEPD2

## ピン配置（`firmware/src/config.h` で管理）

### 現行: ESP32-S3 DevKitC-1

| 機能 | ESP32-S3 GPIO | 備考 |
|---|---|---|
| EPD DIN (MOSI) | GPIO11 | ソフトウェアSPI |
| EPD CLK | GPIO12 | ソフトウェアSPI |
| EPD CS | GPIO10 | |
| EPD DC | GPIO9 | |
| EPD RST | GPIO14 | |
| EPD BUSY | GPIO13 | 起動ストラップを避けた入力 |
| EPD PWR | GPIO21 | HIGH=電源ON、LOW=電源断 |
| I2C SDA | GPIO8 | AHT20 + BMP280 共有 |
| I2C SCL | GPIO18 | AHT20 + BMP280 共有 |
| PIR OUT | GPIO4 | デジタル入力、Deep Sleep ext1 wake |

### 代替: ESP32 DevKit / ESP32-WROOM

| 機能 | ESP32 GPIO | 備考 |
|---|---|---|
| EPD DIN (MOSI) | GPIO23 | ソフトウェアSPI |
| EPD CLK | GPIO18 | ソフトウェアSPI |
| EPD CS | GPIO27 | 起動ストラップを避ける |
| EPD DC | GPIO26 | |
| EPD RST | GPIO25 | |
| EPD BUSY | GPIO33 | 入力 |
| EPD PWR | GPIO32 | HIGH=電源ON、LOW=電源断 |
| I2C SDA | GPIO21 | AHT20 + BMP280 共有 |
| I2C SCL | GPIO22 | AHT20 + BMP280 共有 |
| PIR OUT | GPIO34 | 入力専用、Deep Sleep ext1 wake |

### 代替: ESP32-C3 SuperMini

| 機能 | ESP32-C3 GPIO | 備考 |
|---|---|---|
| EPD DIN (MOSI) | GPIO7 | ソフトウェアSPI |
| EPD CLK | GPIO6 | ソフトウェアSPI |
| EPD CS | GPIO10 | |
| EPD DC | GPIO5 | |
| EPD RST | GPIO3 | |
| EPD BUSY | GPIO2 | |
| EPD PWR | GPIO8 | HIGH=電源ON、LOW=電源断 |
| I2C SDA | GPIO21 | AHT20 + BMP280 共有 |
| I2C SCL | GPIO20 | AHT20 + BMP280 共有 |
| PIR OUT | GPIO1 | デジタル入力、Deep Sleep GPIO wake |

### ボード方針

- 通常開発ターゲットは ESP32 DevKit (ESP32-WROOM)。
- ESP32-S3 DevKitC-1 用のPlatformIO環境とピンアサインは代替・検証用として残す。
- ESP32-C3 SuperMini は Wi-Fi 不安定が確認されたため非推奨（環境は参照用として残す）。

## ライブラリ構成

```ini
; platformio.ini に追記する想定
lib_deps =
    zinggjm/GxEPD2                  ; e-ink制御
    adafruit/Adafruit BME280 Library ; 温湿度気圧センサ
    adafruit/Adafruit Unified Sensor
    bblanchon/ArduinoJson            ; API・データパース
    arduino-libraries/NTPClient      ; 時刻同期
    knolleary/PubSubClient           ; (将来的にMQTT使う場合)
; Google Calendar: Apps Script (GAS) をプロキシとして使用
;   - GAS 内で LanguageApp.translate() により日→英翻訳 + 15文字切り捨て済み
;   - ESP32 は翻訳済みの英語タイトルを受け取るだけ（Gemini API 不要）
;   - エンドポイント URL は config.h の GCAL_ENDPOINT で管理、Gitにコミットしない
```

## 表示コンテンツ

- **時計** — NTPで同期、HH:MM表示
- **気温・湿度・気圧** — BME280から取得、定期更新
- **カレンダー** — Google Apps Script プロキシ経由でイベント取得。GAS側で日→英翻訳（LanguageApp.translate）・15文字切り捨て済み

## フォルダ構成

```
E-ink-Smart-Display/
├── CLAUDE.md
├── platformio.ini
├── src/
│   ├── main.cpp          ← エントリポイント・ループ制御
│   ├── display.cpp/.h    ← e-ink描画ロジック
│   ├── sensors.cpp/.h    ← BME280・PIR読み取り
│   ├── network.cpp/.h    ← Wi-Fi接続・HTTP取得
│   ├── clock.cpp/.h      ← NTP時刻管理
│   └── config.h          ← ピン定義・定数・認証情報プレースホルダ
├── docs/
│   └── changelog.md
└── .gitignore
```

## センサ確認済み情報

- AHT20: I2C 0x38、温湿度OK
- BMP280: I2C **0x77**（SDO→VCC）、chip_id=0x58、気圧・温度OK
- Adafruit ライブラリ使用時は `Wire.begin()` を明示呼び出しせず `Wire.setPins(SDA, SCL)` → `sensor.begin(&Wire)` の順で初期化すること（二重 begin によるESP_ERR_INVALID_STATE 回避）

## 開発メモ・制約

- E-inkはフル更新のみ → **更新頻度は最低限に**（数分〜数十分サイクル）
- 赤の描画は遅い（約15秒）→ 赤は強調用途に限定する
- PIR人感センサ検知時のみ画面更新するロジックも検討中
- Wi-Fi接続後はDeep Sleepで省電力化を目指す
- 認証情報（Wi-FiパスワードなどAPIキー）は `config.h` に書くが **Gitにはコミットしない**（`.gitignore`に追加）
- ESP32-S3では起動ストラップ GPIO0 / GPIO3 / GPIO45 / GPIO46 を避ける。
- ESP32-S3 DevKitC-1ではUSBに使われることが多い GPIO19 / GPIO20 も避ける。
- ESP32 DevKitでは起動ストラップ GPIO0 / GPIO2 / GPIO4 / GPIO5 / GPIO12 / GPIO15 とフラッシュ GPIO6〜GPIO11 を避ける。
- e-ink電源OFF中でも信号線から逆給電する可能性があるため、Wi-Fi接続前はe-ink系ピンを安全状態にする方針。

## よくある作業とファイルの対応

| やりたいこと | 見るファイル |
|---|---|
| レイアウト変更 | `src/display.cpp` |
| センサ値の取得・補正 | `src/sensors.cpp` |
| Wi-Fi・API処理 | `src/network.cpp` |
| 時刻フォーマット | `src/clock.cpp` |
| ピン変更・定数変更 | `src/config.h` |
