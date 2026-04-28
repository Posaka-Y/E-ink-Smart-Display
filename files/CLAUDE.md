# E-ink Smart Display — Project Context for Claude Code

## ハード構成

| コンポーネント | 詳細 |
|---|---|
| MCU | ESP32-C3 SuperMini |
| ディスプレイ | Waveshare 5.79inch e-Paper (B) — 792×272px, 3色 (白/黒/赤) |
| 温湿度・気圧センサ | BME280互換 (AH-20系)、I2C接続 |
| 人感センサ | PIR 3ピン、しきい値可変抵抗つき、デジタル出力 |
| 通信 | Wi-Fi (HTTP/HTTPS) |

## ディスプレイ仕様

- **型番**: Waveshare 5.79inch e-Paper (B) Module
- **解像度**: 792 × 272 px
- **色数**: 3色（黒・白・赤）
- **インターフェース**: SPI
- **部分更新**: 非対応（フル更新のみ）
- **推奨ライブラリ**: GxEPD2

## ピン配置（未確定 — 要記入）

| 機能 | ESP32-C3 GPIO |
|---|---|
| MOSI (DIN) | GP6 |
| CLK | GP4 |
| CS | GP7 |
| DC | GP5 |
| RST | GP3 |
| BUSY | GP2 |
| BME280 SDA | GP21 |
| BME280 SCL | GP20 |
| PIR OUT | GP1 |

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
; Google Calendar API v3はHTTPS GETで直接叩く（専用ライブラリ不要）
; カレンダーは「一般公開」設定 + APIキー認証
; エンドポイント: GET https://www.googleapis.com/calendar/v3/calendars/{calendarId}/events?key={APIkey}
; APIキー・カレンダーIDはconfig.hで管理、Gitにコミットしない
```

## 表示コンテンツ

- **時計** — NTPで同期、HH:MM表示
- **気温・湿度・気圧** — BME280から取得、定期更新
- **カレンダー** — Google Calendar API (v3)、HTTPSでイベント取得

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

## 開発メモ・制約

- E-inkはフル更新のみ → **更新頻度は最低限に**（数分〜数十分サイクル）
- 赤の描画は遅い（約15秒）→ 赤は強調用途に限定する
- PIR人感センサ検知時のみ画面更新するロジックも検討中
- Wi-Fi接続後はDeep Sleepで省電力化を目指す
- 認証情報（Wi-FiパスワードなどAPIキー）は `config.h` に書くが **Gitにはコミットしない**（`.gitignore`に追加）

## よくある作業とファイルの対応

| やりたいこと | 見るファイル |
|---|---|
| レイアウト変更 | `src/display.cpp` |
| センサ値の取得・補正 | `src/sensors.cpp` |
| Wi-Fi・API処理 | `src/network.cpp` |
| 時刻フォーマット | `src/clock.cpp` |
| ピン変更・定数変更 | `src/config.h` |
