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
