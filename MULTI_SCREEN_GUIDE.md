# 複数画面切り替え機能 - ガイド

## 📱 機能概要

ESP32-C3 + e-paper ディスプレイが **8つの画面を10秒ごとに自動切り替え**します。

- 画面は `src/main.cpp` の `NUM_SCREENS`（= 8）と `SCREEN_ENABLED[]` で管理
- `SCREEN_ENABLED` が `false` の画面はスキップされ、次の有効な画面が表示される
- デフォルトでは **画面4: Date & Time だけが無効**（7画面をローテーション）
- 各画面の描画関数は `src/display_screens.cpp` にあり、センサーデータは引数で受け取る（→ `SENSOR_DATA_PASSING.md`）

> 📝 初期の設計案には「システム情報」画面（Env GOOD/OK/CAUTION・Uptime・Active Sensors）がありましたが、実装されていません。

### 🔄 切り替えサイクル（デフォルト設定）

```
┌─────────────────────────────────────────────┐
│ 画面0: Air Quality      (ENS160)            │
│ ↓ 10秒                                      │
│ 画面1: Climate          (AHT21 / BMP280)    │
│ ↓ 10秒                                      │
│ 画面2: Ambience         (VEML7700 / MAX9814)│
│ ↓ 10秒                                      │
│ 画面3: Gas Sensor       (MICS5524)          │
│ ↓ 10秒                                      │
│ (画面4: Date & Time  ※デフォルト無効→スキップ)│
│ 画面5: Month Day        (NTP時刻)           │
│ ↓ 10秒                                      │
│ 画面6: WiFi QR          (wifi_config.h)     │
│ ↓ 10秒                                      │
│ 画面7: Climate Modern   (AHT21 / BMP280)    │
│ ↓ 10秒 (画面0へループ)                      │
└─────────────────────────────────────────────┘
```

画面番号は `display_mode` の値（0〜7）です。シリアル出力では `Screen 1/8`〜`Screen 8/8`（`display_mode + 1`）と表示されます。

---

## 📺 各画面の詳細

基本フォントは全画面共通で `FreeMonoBold12pt7b`（`loop()` で設定）。画面によっては関数内で別フォントに切り替えます。

### 📱 **画面0: Air Quality（空気質）** - `displayAirQuality()`
```
Air Quality
eCO2: 410ppm
TVOC: 25ppb
AQI:  1/5
Excellent
```

**表示内容:**
- eCO2（推定CO2濃度, ppm）
- TVOC（揮発性有機化合物, ppb）
- AQI（1〜5。1=Excellent, 2=Good, 3=Moderate, 4=Poor, 5=Unhealthy。0 は未測定で `unknown`）
- AQI ステータス（英語。`main.cpp` の `getAQIStatusEnglish()`）

ENS160 未接続時は `ENS160 / Not / Connected` と表示。

---

### 📱 **画面1: Climate（温熱環境）** - `displayClimate()`
```
Climate
Temp:  24C
RH:    45%
hPa:   1013
Alt:   15m
```

**表示内容:**
- 温度（AHT21。自己発熱補正済み。AHT21 がない場合は BMP280 の温度）
- 相対湿度（AHT21。補正後の温度に合わせて換算し直した値）
- 気圧（BMP280, hPa）
- 高度（BMP280, m。絶対値で表示）

AHT21 の補正量は `main.cpp` の `AHT21_TEMP_OFFSET`（10 °C）。両センサーとも未接続なら `No / Climate / Sensors`。

---

### 📱 **画面2: Ambience（視聴覚環境）** - `displayAmbience()`
```
Ambience
Lux:   156lx
White: 143
Sound: 45dB
RMS:   0.012V
```

**表示内容:**
- 照度（VEML7700, lux）
- ホワイトチャネル（VEML7700, 参考値）
- 騒音レベル（MAX9814, dB。直近5回の中央値 `db_level`）
- RMS 電圧（MAX9814, AC成分）

片方のセンサーがない場合は `No Light` / `No Sound` を表示。

---

### 📱 **画面3: Gas Sensor（ガスセンサー）** - `displayGasSensor()`
```
Gas Sensor


Rs/Ro: 0.95
```

**表示内容:**
- Rs/Ro 比（MICS5524。新鮮な空気中の基準抵抗 Ro との比）
- 起動から約3分間はヒーターのウォームアップ中のため `Warming up` と表示。ウォームアップ後に `loop()` で一度だけキャリブレーション（Ro 取得）し、以降 Rs/Ro を表示

---

### 📱 **画面4: Date & Time（日時）** - `displayDateTime()` ※デフォルト無効
```
Date & Time
2026-10-16 12:34:56
WiFi: Connected
```

**表示内容:**
- NTP 同期した現在時刻（JST, `getCurrentDateTime()`）
- WiFi 接続状態（`isWiFiConnected()`）

使う場合は `SCREEN_ENABLED[4]` を `true` にします。

---

### 📱 **画面5: Month Day（月日）** - `displayMonthDay()`
```
     10

     16
```

月と日を2桁ゼロ埋めで大きく表示（`FreeMonoBold24pt7b` × 2倍）。詳細は `MONTHDAY_SCREEN.md`。

---

### 📱 **画面6: WiFi QR** - `displayWiFiQR()`

`wifi_config.h` の `WIFI_SSID` / `WIFI_PASSWORD` から `WIFI:T:WPA;S:...;P:...;;` 形式の QR コードを生成し、画面いっぱいに表示します（バージョンは 1〜10 から自動選択、誤り訂正レベル L）。詳細は `WIFI_QR_SCREEN.md`。

---

### 📱 **画面7: Climate Modern（温湿度ゲージ）** - `displayClimateModern()`
```
      ╭──────────╮      ← 外側: 温度ゲージ（0〜50°C を 270° に割り当て）
    ╱ ╭────────╮ ╲     ← 内側: 湿度ゲージ（0〜100%）
   │  │   24   │  │    ← 温度（大きな数字）
   │  │   45%  │  │    ← 湿度
TEMP
‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥    ← 過去24時間の温度グラフ
HUMIDITY
‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥‥    ← 過去24時間の湿度グラフ
```

**表示内容:**
- 円形ゲージ2本（外側=温度、内側=湿度）
- 中央に温度（整数）と湿度（%）
- 下部に過去24時間の折れ線グラフ（温度: 最小−2 °C〜最大+2 °C の自動レンジ。幅が5 °C 未満なら平均±5 °C / 湿度: 0〜100% 固定）

**履歴データ:**
- `main.cpp` の `temp_history` / `humid_history`（`HISTORY_SIZE` = 144点 = 10分ごと × 24時間。定義は `display_screens.h`）
- 起動時に全144点をその時点のセンサー値で埋めるため、起動直後のグラフは平らな線になる
- RAM 上にのみ保持（再起動で消える）
- AHT21 がなく BMP280 だけの場合は温度のみ表示（湿度ゲージの塗りと湿度グラフは描かない）

文字は **U8g2_for_Adafruit_GFX** で U8g2 フォント（`u8g2_font_logisoso42_tn`, `u8g2_font_helvB18_tf`, `u8g2_font_helvR08_tf`）を描画しています（→ `FONT_SYSTEM_ALTERNATIVES.md`）。

---

## 🔧 カスタマイズ

### 1. 画面の有効/無効を切り替える

`src/main.cpp` の `SCREEN_ENABLED` を編集します。

```cpp
#define NUM_SCREENS 8
const bool SCREEN_ENABLED[NUM_SCREENS] = {
  true,   // 画面0: Air Quality (空気質)
  true,   // 画面1: Climate (温熱環境)
  true,   // 画面2: Ambience (視聴覚環境)
  true,   // 画面3: Gas Sensor (ガスセンサー)
  false,  // 画面4: Date & Time (日時表示)
  true,   // 画面5: Month Day (月日表示)
  true,   // 画面6: WiFi QR Code (WiFi QR)
  true,   // 画面7: Climate Modern (温度/湿度モダンデザイン)
};
```

### 2. 切り替え間隔を変更

`loop()` 冒頭の条件を修正します。この間隔は **センサー読み取りの間隔も兼ねています**。

```cpp
// 現在: 10秒ごと
if (current_time - last_update >= 10000 || first_display) {

// 変更例: 15秒ごと
if (current_time - last_update >= 15000 || first_display) {
```

### 3. 画面の順序を変更

表示順は `display_mode` の番号順（0→7）です。順序を変えるには `loop()` 内の `switch(display_mode)` の `case` 番号を入れ替え、`SCREEN_ENABLED` の並びも合わせます。

### 4. 画面の追加

1. `include/display_screens.h` に関数を宣言し、`src/display_screens.cpp` に実装する（必要なデータは引数で受け取る）
2. `main.cpp` の `NUM_SCREENS` を増やし、`SCREEN_ENABLED` に要素を追加する
3. `switch(display_mode)` に `case` を追加する

```cpp
// display_screens.h
void displayCustom(const AHT21Data& aht21_data, bool aht21_enabled);

// main.cpp
#define NUM_SCREENS 9
const bool SCREEN_ENABLED[NUM_SCREENS] = {
  // ... 既存の8画面 ...
  true,   // 画面8: Custom
};

// loop() の switch 内
case 8:
  displayCustom(aht21_data, sensor_aht21_enabled);
  break;
```

画面送りは `% NUM_SCREENS` で計算しているので、それ以外の変更は不要です。

---

## 📊 シリアル出力例

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 1/8
📊 センサーデータ更新:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  eCO2: 410 ppm
  TVOC: 25 ppb
  AQI : 1 (優秀)
  🌡️  温度(AHT21): 24.12 °C (生値 34.12 °C)
  💧 湿度(AHT21): 44.75 % (生値 25.10 %)
  ...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔄 初回フルリフレッシュ実行
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 2/8
...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 4/8
...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 6/8      ← Screen 5/8（Date & Time）は無効なのでスキップ
...
```

---

## 🚀 ビルド & 実行

```bash
# ビルド
pio run -e esp32-c3-super-mini-k2

# アップロード
pio run -e esp32-c3-super-mini-k2 -t upload

# モニタ
pio device monitor -e esp32-c3-super-mini-k2
```

---

## ⚙️ 技術詳細

### 実装方式（`loop()`）

```cpp
static uint8_t display_mode = 0;

// 10秒ごと（初回は即時）
if (current_time - last_update >= 10000 || first_display) {
    // 1. 有効なセンサーの値を読み込む（MICS5524 はウォームアップ後に一度だけキャリブレーション）
    // 2. 10分ごとに温湿度を履歴へ保存
    // 3. シリアルへ出力
    // 4. リフレッシュ方式を選ぶ（初回・10分ごと: フル、それ以外: 部分）
    // 5. switch(display_mode) で画面を描画
    // 6. 次の有効な画面へ進める
    uint8_t start_mode = display_mode;
    do {
        display_mode = (display_mode + 1) % NUM_SCREENS;
    } while (!SCREEN_ENABLED[display_mode] && display_mode != start_mode);
}
```

### リフレッシュ方式

- 初回・10分ごと（`FULL_REFRESH_INTERVAL`）: **フルリフレッシュ**（ゴースト軽減。約1.5秒かかり、画面が白黒に点滅する）
- 通常更新: **部分リフレッシュ**（点滅なし）

### メモリ

- `GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>` で 200×200 の画面全体をフレームバッファとして RAM に保持（1ページで描画）
- 温湿度履歴: `float` × 144点 × 2 = 約1.1KB

---

## 🐛 トラブルシューティング

### Q: 画面が切り替わらない

**A:** 以下を確認してください
- [ ] `SCREEN_ENABLED` で2つ以上の画面が `true` になっているか（1つだけだとその画面のまま）
- [ ] シリアルの `📺 画面: Screen N/8` の番号が進んでいるか
- [ ] e-paper の配線（特に BUSY）が正しいか

### Q: 特定の画面だけ表示がおかしい

**A:** `src/display_screens.cpp` のその画面の関数を確認
- フォント設定
- `setCursor` の座標
- 渡しているデータ（`main.cpp` の `switch` 内の引数）

### Q: 画面が部分的に残像を起こす

**A:** フルリフレッシュ間隔を短くしてください（`loop()` 内の `FULL_REFRESH_INTERVAL`）
```cpp
const unsigned long FULL_REFRESH_INTERVAL = 5 * 60 * 1000;  // 5分に変更
// const unsigned long FULL_REFRESH_INTERVAL = 0;           // 毎回フルリフレッシュ
```

---

## 📈 パフォーマンス

| 項目 | 値 |
|-----|-----|
| 切り替え間隔 | 10秒 |
| センサー読み取り間隔 | 10秒（画面更新と同時） |
| フルリフレッシュ時間 | 約1.5秒 |
| フルリフレッシュ間隔 | 初回 + 10分ごと |
| 履歴保存 | 10分ごと（144点 = 24時間分） |
| センサー読み込み時間 | 0.5秒以上（MAX9814 が 10,000 サンプル × 50µs 待ちで計測するため） |

---

## 🔗 関連ファイル

- `src/main.cpp` - センサー読み取り・画面切り替え・履歴保存
- `include/display_screens.h` / `src/display_screens.cpp` - 各画面の描画関数
- `SENSOR_DATA_PASSING.md` - 描画関数へのデータの渡し方
- `MONTHDAY_SCREEN.md` - 月日画面
- `WIFI_QR_SCREEN.md` - WiFi QR 画面
- `MICS5524_INTEGRATION.md` - ガスセンサー
- `VEML7700_INTEGRATION.md` / `VEML7700_QUICK_START.md` - 照度センサー
- `FONT_SYSTEM_ALTERNATIVES.md` - U8g2 フォント（Climate Modern で使用）

---

**最終更新**: 2026年10月16日  
**バージョン**: 2.0  
**対応: ESP32-C3 Super Mini (K2) + WeAct 1.54" e-paper**
