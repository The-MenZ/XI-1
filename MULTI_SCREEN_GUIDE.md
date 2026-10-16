# 複数画面切り替え機能 - ガイド

## 📱 機能概要

ESP32-C3 + e-paper ディスプレイが **11の画面を10秒ごと（設定ページで変更可）に自動切り替え**します。

- 画面は `src/main.cpp` の `NUM_SCREENS`（= 11）で管理し、各画面の有効/無効は設定ページで変更する（NVS に保存。既定値は `SCREEN_DEFAULT_ENABLED[]`）
- 無効な画面はスキップされ、次の有効な画面が表示される（`isScreenEnabled()` / `nextEnabledScreen()`）
- デフォルトでは **画面4: Date & Time だけが無効**（10画面をローテーション）
- AP モード中（WiFi に接続できないとき）は、画面6: WiFi QR と画面8: Settings QR だけを交互に表示する
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
│ 画面6: WiFi QR          (保存済みの WiFi)   │
│ ↓ 10秒                                      │
│ 画面7: Climate Modern   (AHT21 / BMP280)    │
│ ↓ 10秒                                      │
│ 画面8: Settings QR      (本体の IP)         │
│ ↓ 10秒                                      │
│ 画面9: Weather          (Open-Meteo)        │
│ ↓ 10秒                                      │
│ 画面10: Pressure        (BMP280)            │
│ ↓ 10秒 (画面0へループ)                      │
└─────────────────────────────────────────────┘
```

画面番号は `display_mode` の値（0〜10）です。シリアル出力では `Screen 1/11`〜`Screen 11/11`（`display_mode + 1`）と表示されます。

---

## 📺 各画面の詳細

基本フォントは全画面共通で `FreeMonoBold12pt7b`（`loop()` で設定）。画面によっては関数内で別フォントに切り替えます。

### 📱 **画面0: Air Quality（空気質）** - `displayAirQuality()`
```
  AQI      ┏━━━━━━━━━┓
 ╭───╮     ┃ ﹀﹀  ☀ ┃
     │     ┣━━━━━━━━━┫
  ───╯     ┃  /\     ┃
 ╰───╯    ━┻━━━━━━━━━┻━
★★★☆☆
         STUFFY
   eCO2 900  TVOC 400
```

**表示内容:**
- AQI（ENS160 の AQI-UBA、1〜5）の大きな白抜きの数字と星（AQI 1 で★5つ）
- 右の上げ下げ窓。空気が悪いほど部屋全体に点々のもやがかかり、AQI 4 で下の窓を少し上げ、AQI 5 で全開にする（窓の外はいつも晴れ）
- 下の帯に、AQI 1〜3 は `FRESH` / `GOOD` / `STUFFY`、4・5 は `OPEN!` / `OPEN!!`。その下に eCO2（ppm）と TVOC（ppb）
- ENS160 のウォームアップ中（起動から約3分。新品を初めて使うときは約1時間）は `?` と閉じたブラインドで、10% 進むごとに羽根が1枚上がる
- 詳しくは [AIR_QUALITY_SCREEN.md](AIR_QUALITY_SCREEN.md)

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
- 気圧（BMP280, hPa。設置場所の気圧で、海面気圧への換算はしない。換算した値は画面10で表示）
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
╔════════════════════════╗
║ HP ■■■■■■■■■■      98 ║
╚════════════════════════╝
     │          │   ♪
     │   ◯<     │ ♪
     │ (    )   │
    ━━━━━━━━━━━━━
```

**表示内容:**
- 炭鉱のカナリアと RPG 風の HP ゲージ（MICS5524）。HP は Rs/Ro（新鮮な空気中の基準抵抗 Ro との比）から計算し、1.0 で 100、測れる下限で 0
- HP 70 以上は歌う、30〜69 はしょんぼり、30 未満は床でひっくり返る
- 起動から約3分間はヒーターのウォームアップ中のため、卵と `WARMING UP` を表示。ウォームアップ後に `loop()` で一度だけキャリブレーション（Ro 取得）し、以降カナリアを表示
- 詳しくは [GAS_SCREEN.md](GAS_SCREEN.md)

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

使う場合は設定ページで画面4を ON にします。

---

### 📱 **画面5: Month Day（月日）** - `displayMonthDay()`
```
  1 | 0
 ---+---
  1 | 6
```

画面を4分割し、上段に月、下段に日を1桁ずつ大きく表示（2桁ゼロ埋め）。数字はフォントを使わず、線の太い丸ゴシック風に描きます。詳細は `MONTHDAY_SCREEN.md`。

---

### 📱 **画面6: WiFi QR** - `displayWiFiQR()`

保存済みの WiFi（設定ページで設定）から `WIFI:T:WPA;S:...;P:...;;` 形式の QR コードを生成し、中央に WiFi マークを重ねて表示します（バージョンは 1〜10 から自動選択、誤り訂正レベル Q）。AP モード中は本体の AP に接続するための QR になります。詳細は `WIFI_QR_SCREEN.md`。

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

### 📱 **画面8: Settings QR（設定ページ）** - `displaySettingsQR()`

設定ページの URL（`http://<本体のIP>/`、AP モード中は `http://192.168.4.1/`）の QR コードを、中央に歯車を重ねて表示します。WiFi 切断中は "WiFi: Not Connected" と表示します。詳細は `WIFI_QR_SCREEN.md`。

---

### 📱 **画面9: Weather（天気）** - `displayWeather()`

設定ページで選んだ地域の天気を表示します（Open-Meteo から30分ごとに取得）。

- 上半分: 今の天気の名前（左上）、取得した時刻（右上）、天気アイコン、今の気温
- 下半分: 今日（左）と明日（右）の日付・天気アイコン・最高/最低気温・降水確率
- 地域が未設定なら "Set the location on the settings page"、まだ取得できていなければ "Loading..."（WiFi 切断中は "WiFi: Not Connected"）

詳細は `WEATHER_SCREEN.md`。

---

### 📱 **画面10: Pressure（気圧 H/L）** - `displayPressure()`

天気図風に、中央へ巨大な **H**（1013 hPa 以上）または **L**（未満）と今の海面気圧を出し、1013 hPa との間に 4 hPa ごとの等圧線を引きます。線が多いほど 1013 から離れた強い高気圧・低気圧です。右下の黒いバッジは約3時間前からの気圧の変化（▲ / ▼ / ▲▲ / ▼▼ / →）。

- 気圧は設定ページの「設置場所の標高」で海面の高さに換算した値（`BMP280Data::sea_level_pressure`）
- 傾向は `main.cpp` の `pressure_history`（10分ごと × 18件、hPa×10 の整数）から計算。起動後 約3時間はバッジを出さない
- BMP280 未接続時は `BMP280 / Not / Connected` と表示

詳細は `PRESSURE_SCREEN.md`。

---

## 🔧 カスタマイズ

### 1. 画面の有効/無効を切り替える

設定ページ（画面8の QR から開く）のチェックボックスで切り替えます。設定は NVS に保存され、再起動しても残ります。

保存する前（初回起動時・初期化後）の既定値は `src/main.cpp` の `SCREEN_DEFAULT_ENABLED` です。

```cpp
#define NUM_SCREENS 11
const bool SCREEN_DEFAULT_ENABLED[NUM_SCREENS] = {
  true,   // 画面0: Air Quality (空気質)
  true,   // 画面1: Climate (温熱環境)
  true,   // 画面2: Ambience (視聴覚環境)
  true,   // 画面3: Gas Sensor (ガスセンサー)
  false,  // 画面4: Date & Time (日時表示)
  true,   // 画面5: Month Day (月日表示)
  true,   // 画面6: WiFi QR Code (WiFi QR)
  true,   // 画面7: Climate Modern (温度/湿度モダンデザイン)
  true,   // 画面8: Settings QR Code (設定ページの URL)
  true,   // 画面9: Weather (天気。地域は設定ページで選ぶ)
  true,   // 画面10: Pressure (天気図風の H/L と等圧線)
};
```

### 2. 切り替え間隔を変更

設定ページの「切り替え間隔（秒）」で変更します（5〜3600秒、既定10秒）。この間隔は **センサー読み取りの間隔も兼ねています**。

### 3. 画面の順序を変更

表示順は `display_mode` の番号順（0→10）です。順序を変えるには `loop()` 内の `switch(display_mode)` の `case` 番号を入れ替え、`SCREEN_DEFAULT_ENABLED` と `SCREEN_NAMES` の並びも合わせます。

### 4. 画面の追加

1. `include/display_screens.h` に関数を宣言し、`src/display_screens.cpp` に実装する（必要なデータは引数で受け取る）
2. `main.cpp` の `NUM_SCREENS` を増やし、`SCREEN_DEFAULT_ENABLED` と `SCREEN_NAMES`（設定ページに出す名前）に要素を追加する
3. `switch(display_mode)` に `case` を追加する

```cpp
// display_screens.h
void displayCustom(const AHT21Data& aht21_data, bool aht21_enabled);

// main.cpp
#define NUM_SCREENS 12
const bool SCREEN_DEFAULT_ENABLED[NUM_SCREENS] = {
  // ... 既存の11画面 ...
  true,   // 画面11: Custom
};
const char* const SCREEN_NAMES[NUM_SCREENS] = {
  // ... 既存の11画面 ...
  "Custom",
};

// loop() の switch 内
case 11:
  displayCustom(aht21_data, sensor_aht21_enabled);
  break;
```

画面の有効/無効は NVS に16ビットのビットマスクで保存しているので、画面は16個まで追加できます。設定を保存した後にファームウェアで画面を増やした場合、増えた画面には `SCREEN_DEFAULT_ENABLED` の値が使われます（保存時の画面数も NVS に記録して見分けています）。

---

## 📊 シリアル出力例

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 1/11
📊 センサーデータ更新:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  eCO2: 410 ppm
  TVOC: 25 ppb
  AQI : 1 (優秀)
  （ENS160 のウォームアップ中は、この下に「⏳ ENS160 準備中（有効フラグ 1、進み具合 8%）」）
  🌡️  温度(AHT21): 24.12 °C (生値 34.12 °C)
  💧 湿度(AHT21): 44.75 % (生値 25.10 %)
  ...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔄 初回フルリフレッシュ実行
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 2/10
...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 4/10
...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 画面: Screen 6/10     ← Screen 5/10（Date & Time）は無効なのでスキップ
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

// 毎回: WiFi の監視（networkLoop）と設定ページのリクエスト処理（webSettingsLoop）
// 切り替え間隔（既定10秒）ごと（初回は即時）
if (current_time - last_update >= settings.interval_sec * 1000UL || first_display) {
    // 0. 今の画面が無効になっていたら（設定変更・AP モード）次の有効な画面へ
    // 1. 有効なセンサーの値を読み込む（MICS5524 はウォームアップ後と、設定ページで要求されたときにキャリブレーション）
    // 2. 10分ごとに温湿度を履歴へ保存
    // 3. シリアルへ出力
    // 4. リフレッシュ方式を選ぶ（初回・10分ごと: フル、それ以外: 部分）
    // 5. switch(display_mode) で画面を描画
    // 6. 次の有効な画面へ進める
    display_mode = nextEnabledScreen(display_mode);
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
- [ ] 設定ページで2つ以上の画面が ON になっているか（1つだけだとその画面のまま）
- [ ] AP モードになっていないか（AP モード中は WiFi QR と Settings QR だけを交互に表示する）
- [ ] シリアルの `📺 画面: Screen N/10` の番号が進んでいるか
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
- `AIR_QUALITY_SCREEN.md` - 空気質画面
- `MONTHDAY_SCREEN.md` - 月日画面
- `WIFI_QR_SCREEN.md` - WiFi QR 画面
- `MICS5524_INTEGRATION.md` - ガスセンサー
- `VEML7700_INTEGRATION.md` / `VEML7700_QUICK_START.md` - 照度センサー
- `FONT_SYSTEM_ALTERNATIVES.md` - U8g2 フォント（Climate Modern で使用）

---

**最終更新**: 2026年10月17日  
**バージョン**: 2.0  
**対応: ESP32-C3 Super Mini (K2) + WeAct 1.54" e-paper**
