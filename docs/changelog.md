## v1

### 2026-04-28 — ビルドエラー修正

**問題1: リンカエラー (`ld returned 1 exit status`)**
- `extra_sources.py` の Waveshare フォントディレクトリのパスが間違っていた
- `../5in79_e-Paper_B_ESP32/ESP32` → `../reference/5in79_e-Paper_B_ESP32/ESP32` に修正
- `Font8`〜`Font24` のシンボルが未定義のままリンカが失敗していた

**問題2: GCC が日本語パスを開けない (`No such file or directory`)**
- `reference/` フォルダのパスに `趣味`（日本語）が含まれており、GCC (`cc1plus.exe`) が直接開けない
- 対策: `font8/12/16/20/24.cpp` を `firmware/src/` にコピーして日本語パスを回避
- `extra_sources.py` の `BuildSources` 呼び出しと `platformio.ini` の `extra_scripts` 行を削除

**現在の状態**
- `firmware/src/` にすべてのソース・フォントファイルが揃っている
- ビルド出力は `C:/pio-build/eink-display/`（ASCII パス）に設定済み
- 次回ビルドで通るはず

### 2026-05-01 — Wi-Fi / Google Calendar デバッグ結果統合

**ビルド構成**
- `firmware/platformio.ini` の通常環境 `esp32c3_supermini` で、デバッグ用ソースを除外するよう修正
- 除外対象: `wifi_debug.cpp`, `cal_debug.cpp`, `sensor_debug.cpp`, `display_test.cpp`
- `pio run -e wifi_debug` 成功
- `pio run -e esp32c3_supermini` 成功

**Wi-Fi デバッグ**
- `wifi_debug` 環境で WPA2-Enterprise 接続を確認
- 成功ログ:
  - `CONNECTED(3)`
  - IP取得成功: `10.27.72.88`
  - RSSI: `-67 dBm`
- 予備ボードでは同じコードで接続成功
- 元ボードでは `AUTH_EXPIRE` / `ASSOC_EXPIRE` が出ていたため、コード・認証情報よりも以下を疑う
  - 元ボード側のWi-Fi RF/アンテナ/電源品質
  - 固定MACを使った2台同時起動によるMAC衝突
  - NVS/フラッシュ内の古いWi-Fi状態
- 元ボードの切り分け手順:
  - `pio run -e wifi_debug -t erase`
  - `pio run -e wifi_debug -t upload`
  - 予備ボードは電源OFFにして元ボード単体で確認

**Google Calendar デバッグ**
- Wi-Fi接続後、Apps Script経由のカレンダーJSON取得に成功
- 成功ログ:
  - `[gcal] body: [{"title":"Calendar test","start":"2026-05-01T16:30:00+09:00","end":"2026-05-01T17:30:00+09:00"}]`
  - `[gcal] Fetched 1 events`
  - `Calendar test | 16:30 | allDay=0`
- 一度 `SSL EOF` / `HTTP -1` が発生したが、その後同じ経路で成功
- 現時点ではカレンダー処理は以下まで確認済み
  - Wi-Fi接続
  - HTTPS接続
  - Apps Script JSON取得
  - JSONパース
  - `start` から `HH:MM` 表示値への変換
  - `CalEvent` への格納

**コード修正**
- `firmware/src/main.cpp`
  - BMP280 初期化を Adafruit BMP280 ライブラリのAPIに合わせて修正
  - USB CDC のシリアルログ取りこぼし対策として起動時に `Serial` 待ちを追加
- `firmware/src/wifi_debug.cpp`
  - 既定の認証パターンを `PEAP` + `user@domain` に変更
  - MAC設定・EAP設定の戻り値ログを追加
  - USB CDC のシリアルログ取りこぼし対策を追加

### 2026-05-01 — ESP32-S3 DevKitC-1 へ開発ボード移行

**移行理由**
- ESP32-C3 SuperMini + 自作拡張ボード接続時にWi-Fi `AUTH_EXPIRE` / `ASSOC_EXPIRE` が再現
- 信号線を外すとWi-Fi接続し、ボードに触れる/揺らすと接続状態が変化
- C3の小型基板ではアンテナ干渉、ピンヘッダ接触、信号線逆給電、ストラップピン干渉の切り分けが難しい
- 開発効率と配線余裕を優先し、ESP32-S3 DevKitC-1を開発母艦に変更

**新ピンアサイン**
- EPD DIN/MOSI: GPIO11
- EPD CLK/SCK: GPIO12
- EPD CS: GPIO10
- EPD DC: GPIO9
- EPD RST: GPIO14
- EPD BUSY: GPIO13
- EPD PWR: GPIO21
- I2C SDA: GPIO8
- I2C SCL: GPIO18
- PIR OUT: GPIO4

**避けるピン**
- ESP32-S3 起動ストラップ: GPIO0, GPIO3, GPIO45, GPIO46
- USBで使われることが多いピン: GPIO19, GPIO20

**コード変更**
- `firmware/src/config.h` をESP32-S3 DevKitC-1向けピンへ更新
- `firmware/platformio.ini` のboardを `esp32-s3-devkitc-1` に変更
- メイン環境名を `esp32s3_devkitc1` に変更
- `main.cpp` のDeep Sleep PIR wakeをS3でビルドできる `esp_sleep_enable_ext1_wakeup()` に変更
- `display_test.cpp` の単体テスト用ピンもS3向けに更新
- `CLAUDE.md` に移行理由と新ピン表を保存

**ビルド確認**
- `pio run -e esp32s3_devkitc1` 成功
- `pio run -e wifi_debug` 成功
- `pio run -e cal_debug` 成功
- `pio run -e sensor_debug` 成功
- `pio run -e display_test` 成功

### 2026-05-01 — ESP32-C3 SuperMini を通常ターゲットへ戻す

**方針変更**
- ESP32-C3 SuperMini のWi-Fi接続不良は、ボード自体ではなく拡張ボード/配線側の要因として切り分けできたため、通常ターゲットをC3へ戻した
- ESP32-S3 DevKitC-1の環境とピンアサインは代替・検証用として残す

**コード変更**
- `firmware/platformio.ini`
  - 通常メイン環境を `esp32c3_supermini` に戻した
  - `esp32s3_devkitc1` 環境は残した
  - `wifi_debug`, `cal_debug`, `sensor_debug`, `display_test` はC3向けへ戻した
- `firmware/src/config.h`
  - C3ピンをデフォルトに戻した
  - `BOARD_ESP32S3_DEVKITC1` 定義時だけS3ピンへ切り替える構成にした
- `firmware/src/display_test.cpp`
  - ハードコードしていたe-inkピンをやめ、`config.h` のピン定義を使うようにした
- `firmware/src/main.cpp`
  - C3では `esp_deep_sleep_enable_gpio_wakeup()`
  - S3では `esp_sleep_enable_ext1_wakeup()`
  - ビルドターゲットに応じてDeep Sleep wake APIを切り替えるようにした
- `CLAUDE.md`
  - 現行ターゲットをESP32-C3 SuperMiniへ戻し、S3を代替環境として記録

**ビルド確認**
- `pio run -e esp32c3_supermini` 成功
- `pio run -e esp32s3_devkitc1` 成功
- `pio run -e wifi_debug` 成功
- `pio run -e cal_debug` 成功
- `pio run -e sensor_debug` 成功
- `pio run -e display_test` 成功

### 2026-05-01 — ESP32-S3 DevKitC-1 を通常ターゲットへ再設定

**方針変更**
- 通常ターゲットをESP32-S3 DevKitC-1へ再設定
- ESP32-C3 SuperMiniの環境とピンアサインは代替・検証用として保持

**コード変更**
- `firmware/platformio.ini`
  - `esp32s3_devkitc1` を通常メイン環境として先頭に配置
  - `esp32c3_supermini` を代替環境として保持
  - `wifi_debug`, `cal_debug`, `sensor_debug`, `display_test` をS3向けに変更
- `firmware/src/config.h`
  - コメントをS3デフォルト/C3代替に更新
- `CLAUDE.md`
  - 現行ターゲットをESP32-S3 DevKitC-1、C3を代替として更新

**ビルド確認**
- `pio run -e esp32s3_devkitc1` 成功
- `pio run -e esp32c3_supermini` 成功
- `pio run -e wifi_debug` 成功
- `pio run -e cal_debug` 成功
- `pio run -e sensor_debug` 成功
- `pio run -e display_test` 成功

### 2026-05-01 — ESP32 DevKit を通常ターゲットへ変更

**移行理由**
- ESP32-C3 SuperMini の Wi-Fi 不安定（AUTH_EXPIRE / ASSOC_EXPIRE）がボード起因と確定
- デバッグを重ねた結果、ボード変更が必要と判断し ESP32 DevKit (ESP32-WROOM) へ移行

**コード変更**
- `firmware/platformio.ini`
  - `default_envs = esp32dev` を追加
  - `wifi_debug`, `cal_debug`, `sensor_debug`, `display_test` を esp32dev ボード・BOARD_ESP32_DEVKIT へ変更
  - `wifi_debug_esp32dev` 環境を削除（`wifi_debug` と重複）
  - USB CDC フラグ (`ARDUINO_USB_MODE`, `ARDUINO_USB_CDC_ON_BOOT`) を削除（esp32dev 不要）
- `CLAUDE.md`
  - 現行ターゲットを ESP32 DevKit へ更新
  - C3 SuperMini を非推奨として記録

**Wi-Fi 接続確認（wifi_debug_esp32dev → wifi_debug）**
- VARIANT 3 (PEAP, identity=`user@domain`, username=`user@domain`) で接続成功
- IP: 10.27.72.88  RSSI: -66 dBm  TeaClass ch1/ch6/ch11 検出
- main.cpp の `wifi_connect()` は既に VARIANT 3 と同一設定のため追加変更不要
- 初回 `WiFi.disconnect()` の `ESP_ERR_WIFI_NOT_INIT` は未初期化時の想定内エラー（無害）

**ビルド確認**
- `pio run -e wifi_debug` 成功（Wi-Fi 接続 OK）
- `pio run -e esp32dev` は未実施（次回確認すること）

### 2026-05-01 — ESP32 DevKit / ESP32-WROOM 環境追加

**追加理由**
- 書き込み時に `This chip is ESP32, not ESP32-S3` が出たため、通常ESP32 DevKit用の環境を追加

**新環境**
- `esp32dev`: メインファームウェア用
- `wifi_debug_esp32dev`: Wi-Fi接続確認用

**ESP32 DevKit ピンアサイン**
- EPD DIN/MOSI: GPIO23
- EPD CLK/SCK: GPIO18
- EPD CS: GPIO27
- EPD DC: GPIO26
- EPD RST: GPIO25
- EPD BUSY: GPIO33
- EPD PWR: GPIO32
- I2C SDA: GPIO21
- I2C SCL: GPIO22
- PIR OUT: GPIO34

**避けるピン**
- 起動ストラップ: GPIO0, GPIO2, GPIO4, GPIO5, GPIO12, GPIO15
- フラッシュ: GPIO6-GPIO11

**ビルド確認**
- `pio run -e esp32dev` 成功
- `pio run -e wifi_debug_esp32dev` 成功
- `pio run -e esp32s3_devkitc1` 成功

### 2026-05-01 — ESP32 DevKitでWi-Fi / Calendar確認完了、次はe-ink表示

**確認済み**
- ESP32 DevKit環境でWi-Fi接続確認まで進行
- Google Calendar / Apps Script 経由のJSON取得経路も通信としては確認済み
- `[gcal] body: []` は通信失敗ではなく、Apps Script側が該当イベントなしとして空配列を返している状態
- Wi-Fi / Calendarの通信系は一旦OKとして、次工程をe-ink表示確認へ移す

**現在の制約**
- 自作基板が手元にないため、拡張基板込みの最終配線確認は保留
- 当面はESP32 DevKit + ジャンパ配線でe-ink表示の単体確認を進める

**次タスク**
- `display_test` 環境でWaveshare 5.79inch e-Paper (B)の表示確認
- まずは電源制御 `EPD_PWR`、SPI配線、`BUSY` の待機が正しく動くか確認
- 表示が出たらメイン画面レイアウトへ進む

**ESP32 DevKit e-ink配線**
- EPD DIN/MOSI: GPIO23
- EPD CLK/SCK: GPIO18
- EPD CS: GPIO27
- EPD DC: GPIO26
- EPD RST: GPIO25
- EPD BUSY: GPIO33
- EPD PWR: GPIO32
