# Codex Progress Log

このファイルは、KiCad開発でCodexと確認・変更した内容を逐次保存するための作業ログです。

## 2026-05-03

### PCBネット・未配線確認

対象PCB:

`E-ink smart display board.kicad_pcb`

確認結果:

- フットプリント数: 4
  - U1: ESP32-DEVKIT-V1
  - U2: AHT20_BMP280
  - J1: JST XH 3pin
  - J2: JST ZH 9pin
- ネット総数: 29
- 配線トラック: 0
- ビア: 0
- ゾーン: 3
- 複数パッドを持つ未配線候補ネット: 12
- 推定未配線 airwire: 17

未配線候補:

| Net | Pads | 推定未配線 |
| --- | --- | ---: |
| GND | U2.GND, J2.2, U1.2, U1.29, J1.3 | 4 |
| Net-(J1-Pin_1) | U2.VDD, J2.1, U1.1, J1.1 | 3 |
| Net-(J1-Pin_2) | U1.19, J1.2 | 1 |
| Net-(J2-Pin_3) | J2.3, U1.15 | 1 |
| Net-(J2-Pin_4) | J2.4, U1.9 | 1 |
| Net-(J2-Pin_5) | J2.5, U1.27 | 1 |
| Net-(J2-Pin_6) | J2.6, U1.26 | 1 |
| Net-(J2-Pin_7) | J2.7, U1.25 | 1 |
| Net-(J2-Pin_8) | J2.8, U1.22 | 1 |
| Net-(J2-Pin_9) | J2.9, U1.21 | 1 |
| Net-(U1-D21) | U2.SDA, U1.11 | 1 |
| Net-(U1-D22) | U2.SCL, U1.14 | 1 |

メモ:

- KiCad MCPのPCB/Schematic問い合わせはSDK側が空応答を返したため、PCBファイルを直接読んで確認した。
- `unconnected-(...)` の単一パッドネットは、意図的な未接続候補として未配線数から除外した。

### 運用ルール

以後、CodexでKiCad開発を進めるときは、作業後にこのファイルへ以下を追記する。

- 実施日
- 対象ファイル
- 変更・確認内容
- 残タスク
- KiCad MCPの実行結果や制約

### ファームウェア由来ピンアサイン反映

参照元:

`C:\Users\posak\Desktop\趣味\E-ink-Smart-Display\firmware\src\config.h`

対象ボード:

`ESP32-DEVKIT-V1:MODULE_ESP32_DEVKIT_V1`

対象PCB:

`E-ink smart display board.kicad_pcb`

反映内容:

- `BOARD_ESP32_DEVKIT` のピン定義をPCBネットへ反映した。
- 自動生成風のネット名を、信号名ベースへ整理した。
- e-Paper用SPI/制御線のうち、CS/DC/RSTがファームウェア定義とずれていたため修正した。
- 現在の `.kicad_sch` は空に近い状態のため、今回はPCBファイルのパッドネットを直接更新した。

反映後の接続:

| Signal | Firmware GPIO | ESP32 DevKit Pad | Connector/Sensor |
| --- | ---: | --- | --- |
| 3V3 | 3V3 | U1.1 | J1.1, J2.1, U2.VDD |
| GND | GND | U1.2, U1.29 | J1.3, J2.2, U2.GND |
| EPD_DIN | GPIO23 | U1.15 / D23 | J2.3 |
| EPD_CLK | GPIO18 | U1.9 / D18 | J2.4 |
| EPD_CS | GPIO27 | U1.25 / D27 | J2.5 |
| EPD_DC | GPIO26 | U1.24 / D26 | J2.6 |
| EPD_RST | GPIO25 | U1.23 / D25 | J2.7 |
| EPD_BUSY | GPIO33 | U1.22 / D33 | J2.8 |
| EPD_PWR | GPIO32 | U1.21 / D32 | J2.9 |
| I2C_SDA | GPIO21 | U1.11 / D21 | U2.SDA |
| I2C_SCL | GPIO22 | U1.14 / D22 | U2.SCL |
| PIR_OUT | GPIO34 | U1.19 / D34 | J1.2 |

修正された不一致:

- `EPD_CS`: GPIO12/U1.27 から GPIO27/U1.25 へ変更
- `EPD_DC`: GPIO14/U1.26 から GPIO26/U1.24 へ変更
- `EPD_RST`: GPIO27/U1.25 から GPIO25/U1.23 へ変更
- U1.26/D14 と U1.27/D12 は未接続へ戻した。

残タスク:

- KiCad上でPCBを開き直す、または再読み込みして変更を反映する。
- 回路図側が必要なら、現在空の `.kicad_sch` を復元または作成し、PCBと同期できる状態にする。
- KiCadのDRC/未配線表示で、今回のネット割り当てを確認する。

### PCB/回路図/ファームウェア整合性の再確認

対象ファイル:

- `firmware/src/config.h`
- `E-ink smart display board.kicad_pcb`
- `E-ink smart display board.kicad_sch`

確認内容:

- PCB側の主要ネットは `BOARD_ESP32_DEVKIT` のファームウェア定義と一致していることを確認した。
  - EPD: DIN=GPIO23, CLK=GPIO18, CS=GPIO27, DC=GPIO26, RST=GPIO25, BUSY=GPIO33, PWR=GPIO32
  - I2C: SDA=GPIO21, SCL=GPIO22
  - PIR: GPIO34
- PCB上の実装部品は現状 4点。
  - U1: ESP32 DevKit
  - U2: AHT20/BMP280
  - J1: JST XH 3pin / PIR
  - J2: JST ZH 9pin / e-Paper
- `.kicad_sch` にはPCBと一致しない古い/別案の構成が残っている。
  - ESP32-WROOM-32単体シンボルになっており、PCBのESP32 DevKitフットプリントと一致しない。
  - e-Paperが8pinコネクタ、センサが4pinコネクタ、PIRがJ3として書かれており、PCBのJ1/J2構成と一致しない。
  - USB-C、AMS1117、LED、抵抗、コンデンサなど、PCBに未配置の部品が含まれている。
  - `I2C_SCL_GPIO20`、`PIR_GPIO1` など、現在のDevKitファームウェア定義と異なるラベルが残っている。
  - `EPD_MOSI`/`EPD_SCK` と PCB側の `EPD_DIN`/`EPD_CLK` で命名が揺れている。

設計上の判断:

- 現状では回路図からPCBへ同期すると、PCB側で直したネット割り当てを壊す可能性がある。
- 次の作業は「PCBを正として回路図を作り直す/整理する」か、「回路図を捨ててPCB直編集で配線を進める」かを決める必要がある。

推奨残タスク:

- まず回路図をPCBの実装構成に合わせて再作成し、以後は回路図を正としてPCB同期できる状態に戻す。
- 回路図を整理した後、J2 e-Paper 9pin、J1 PIR 3pin、U2 I2Cセンサ、U1 ESP32 DevKitのネットを確定する。
- その後にPCB配線、GNDゾーン確認、DRC確認へ進む。

### 回路図をPCB準拠で1から作り直し

対象ファイル:

- `E-ink smart display board.kicad_sch`
- `E-ink smart display board.kicad_sch.codex-backup-20260503`
- `codex_schematic_test.net`
- `codex_erc.rpt`
- `codex_drc.rpt`

実施内容:

- 既存回路図をバックアップしたうえで、PCB上の現行4部品だけの回路図へ作り直した。
  - U1: ESP32-DEVKIT-V1
  - U2: AHT20_BMP280
  - J1: PIR Sensor / JST XH 3pin
  - J2: Waveshare 5.79in e-Paper B / JST ZH 9pin
- 古い/別案の部品を回路図から除外した。
  - ESP32-WROOM-32単体
  - USB-C
  - AMS1117
  - LED
  - 抵抗/コンデンサ
  - 旧8pin e-Paperコネクタ
  - 旧4pinセンサコネクタ
- ネット名をPCB側に合わせて統一した。
  - `EPD_DIN`
  - `EPD_CLK`
  - `EPD_CS`
  - `EPD_DC`
  - `EPD_RST`
  - `EPD_BUSY`
  - `EPD_PWR`
  - `I2C_SDA`
  - `I2C_SCL`
  - `PIR_OUT`
  - `3V3`
  - `GND`
- ESP32 DevKitやセンサモジュールの電源ピンは、モジュール間接続の回路図として扱うため `passive` にした。

KiCad CLI確認:

- `kicad-cli sch export netlist` 成功。
- ネットリスト上の接続はPCB側ネット割り当てと一致。

| Net | Schematic nodes |
| --- | --- |
| 3V3 | J1.1, J2.1, U1.1, U2.VDD |
| GND | J1.3, J2.2, U1.2, U1.29, U2.GND |
| EPD_DIN | J2.3, U1.15 |
| EPD_CLK | J2.4, U1.9 |
| EPD_CS | J2.5, U1.25 |
| EPD_DC | J2.6, U1.24 |
| EPD_RST | J2.7, U1.23 |
| EPD_BUSY | J2.8, U1.22 |
| EPD_PWR | J2.9, U1.21 |
| I2C_SDA | U1.11, U2.SDA |
| I2C_SCL | U1.14, U2.SCL |
| PIR_OUT | J1.2, U1.19 |

ERC結果:

- エラー: 0
- 警告: 4
- 警告内容: 回路図内に埋め込んだ `Local` シンボルライブラリが、プロジェクトの `sym-lib-table` には未登録。
- 実接続のエラーではないが、KiCad上の警告を消すならプロジェクトローカルの `sym-lib-table` と `local.kicad_sym` を作る。

PCB DRC結果:

- DRC警告: 1
  - U2リファレンス文字高さが 0.7874 mm で、設定最小 0.8000 mm をわずかに下回る。
- 未配線: 17
  - これは現状トラック未配線のため想定どおり。

残タスク:

- KiCadで回路図を開き、表示崩れがないか確認する。
- 必要なら `Local` シンボル警告を消すため、プロジェクトローカルのシンボルライブラリを作成する。
- PCB配線を開始する。
- U2リファレンス文字高さを 0.8 mm 以上に直す。

### PCBエディター作業方針の相談

対象:

- KiCad PCBエディター上の手作業
- `E-ink smart display board.kicad_pcb`

相談・確認内容:

- 今後はユーザーがKiCadを直接操作し、Codexは `CODEX_PROGRESS.md` を読んだうえで設計相談に乗る運用に変更した。
- 回路図では、長い実線配線ではなく「短いワイヤ + ネットラベル」で接続する方針を確認した。
  - 特にJ2 e-Paper 9pin、U1 ESP32 DevKit、U2 I2C、J1 PIR間はラベル接続を基本とする。
- J2 e-Paper 9pinのラベル割り当てを再確認した。

| J2 pin | Net |
| ---: | --- |
| 1 | 3V3 |
| 2 | GND |
| 3 | EPD_DIN |
| 4 | EPD_CLK |
| 5 | EPD_CS |
| 6 | EPD_DC |
| 7 | EPD_RST |
| 8 | EPD_BUSY |
| 9 | EPD_PWR |

- J1 PIR 3pinのラベル割り当てを再確認した。

| J1 pin | Net |
| ---: | --- |
| 1 | 3V3 |
| 2 | PIR_OUT |
| 3 | GND |

- `PIR_OUT` ラベル接続例について、ワイヤ終点とラベル座標が一致していれば電気的に接続されることを確認した。
- ERCの「電源入力ピンが電源出力ピンによって駆動されない」警告について相談した。
  - 回路図上に電源出力ピンが存在しないため出る典型的なERC警告。
  - 対応案として `3V3` と `GND` に `PWR_FLAG` を置く方針を提示。
  - ESP32 DevKitの3V3ピンをPower Outputに変更するより、`PWR_FLAG` の方が無難。
- PCBエディターに進む方針を確認した。
  - まず「回路図からPCBを更新」する。
  - 次にラッツネストとネット名を確認する。
  - 配線は信号線、3V3、GNDゾーンの順で進める。
- 基板外形の方針を確認した。
  - 外形: 約72 mm x 40 mm
  - 四隅にM3用 3.2 mm取り付け穴
  - 穴中心は外形から4 mm内側を初期案とする。
  - Edge.Cutsと取り付け穴は機械寸法なのでロック推奨。
- ESP32 DevKitはレイアウトがほぼ固定のため、U1フットプリントをロックしてよい方針を確認した。
  - フットプリントのロックはレイヤーではなく、U1フットプリント自体のプロパティ/右クリック操作で行う。
- ESP32 DevKit上辺と基板Edge.Cutsの位置合わせについて相談した。
  - 数値座標で合わせるか、グリッドを細かくして実物外形線をEdge.Cutsに合わせる。
  - フットプリント内のどの線が実物外形に近いかを基準にする。
- ESP32アンテナ配置について相談した。
  - アンテナが基板上に来る配置はWi-Fi/BLE感度に影響する可能性がある。
  - 可能ならアンテナ部分を基板外へ逃がす。
  - 難しい場合でも、アンテナ直下と周辺は `F.Cu`/`B.Cu` の銅箔、GNDゾーン、配線、ビアを避ける。
  - KiCadではアンテナ部にKeepout Areaを置くのが推奨。

残タスク:

- ユーザー側でPCBエディターの配置/外形/穴/ロックを進める。
- ESP32アンテナ部のKeepout Areaを作るか、アンテナを基板外へ逃がす配置にするか決める。
- 配置が固まったら、信号線配線とGNDゾーンの相談へ進む。

### 電源デカップリング/バルクコンデンサー追加

対象ファイル:

- `E-ink smart display board.kicad_sch`
- `E-ink smart display board.kicad_sch.codex-backup-before-caps-20260503`
- `codex_schematic_with_caps.net`
- `codex_erc_with_caps.rpt`
- `E-ink smart display board_with_caps.pdf`
- `schematic_with_caps_svg/E-ink smart display board.svg`

実施内容:

- 回路図に `Local:C` シンボルを追加し、3V3-GND間のコンデンサーを4点追加した。

| Ref | Value | Footprint | 用途 |
| --- | --- | --- | --- |
| C1 | 47uF | `Capacitor_SMD:C_1206_3216Metric` | J2 e-Paperコネクタ近傍のバルク |
| C2 | 0.1uF | `Capacitor_SMD:C_0603_1608Metric` | J2 e-Paperコネクタ近傍の高周波バイパス |
| C3 | 10uF | `Capacitor_SMD:C_0805_2012Metric` | 3V3レール/モジュール電源分岐近傍のバルク |
| C4 | 0.1uF | `Capacitor_SMD:C_0603_1608Metric` | U2 AHT20/BMP280近傍のバイパス |

確認結果:

- `kicad-cli sch export netlist` 成功。
- ネットリスト上で C1〜C4 は `/3V3` と `/GND` に接続されていることを確認した。
- `kicad-cli sch export pdf` 成功。
- `kicad-cli sch export svg` 成功。
- ERC結果はエラー0、警告8。
  - 警告は追加分を含む `Local` シンボルライブラリ未登録警告のみ。
  - 実接続エラーではない。

残タスク:

- KiCadで回路図を開き、コンデンサー配置と表示を目視確認する。
- 「回路図からPCBを更新」で C1〜C4 のフットプリントをPCBへ追加する。
- PCB上では C1/C2をJ2近傍、C4をU2近傍、C3を3V3分岐またはESP32 DevKit 3V3近傍へ配置する。
- 必要なら `Local` ライブラリ警告を消すため、プロジェクトローカルの `sym-lib-table` と `local.kicad_sym` を作成する。

### USB常時給電前提の3.3V電源接続確認

対象ファイル:

- `E-ink smart display board.kicad_sch`
- `codex_schematic_usb_powered.net`
- `codex_erc_usb_powered.rpt`
- `E-ink smart display board_usb_powered.pdf`

設計前提:

- ESP32 DevKitはUSBから常時給電する。
- e-Paper、PIR、AHT20/BMP280、追加コンデンサーはESP32 DevKitの3.3V出力から給電する。
- そのため回路図上の電源ネット名は `3.3V` とし、U1の3V3ピンをこのネットの供給元として扱う。

実施内容:

- U1の3V3ピンが `no_connect` 扱いになっていたため、`3.3V` ネットへ接続した。
- ESP32 DevKitのGNDピンは、USB給電済みモジュールの外部接続端子として扱うため、ERC上は `passive` 扱いにした。
- 孤立していた未接続フラグを削除した。

確認結果:

- `kicad-cli sch export netlist` 成功。
- ネットリスト上で `/3.3V` に U1.1、C1〜C4、J1.1、J2.1、U2.VDD が含まれることを確認した。
- `kicad-cli sch erc` 結果: エラー0、警告0。
- `kicad-cli sch export pdf` 成功。

注意:

- ESP32 DevKitの3.3Vレギュレータ容量を超えないよう、e-Paper更新時のピーク電流を確認する。
- PCBでは3.3V配線を細くしすぎず、C1/C2をe-Paperコネクタ近傍、C4をU2近傍、C3を3.3V分岐またはU1近傍へ置く。
