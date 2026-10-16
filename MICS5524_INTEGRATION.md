# MICS5524 ガスセンサー統合ガイド

## 概要
MICS5524 ガスセンサー（Adafruit ブレークアウト #3199）をESP32-C3プロジェクトに統合しました。このセンサーは一酸化炭素（CO）、アンモニア（NH3）、エタノール、水素（H2）、メタンなどの還元性ガスに反応します。

> ⚠️ MICS5524 は**ガスの種類を区別できません**（非選択性）。表示する ppm は「CO だったら何 ppm 相当か」という換算値なので、目安として扱ってください。

## センサー仕様

### 検出対象ガス
- **一酸化炭素 (CO)**: 1～1000 ppm
- **アンモニア (NH3)**: 1～500 ppm
- **エタノール (C2H5OH)**: 10～500 ppm
- **水素 (H2)**: 1～1000 ppm
- **メタン/プロパン/イソブタン**: 1,000+ ppm

### 主な仕様
- **基板**: Adafruit MiCS5524 ブレークアウト (#3199)
- **電源**: 5V DC
- **ヒーター消費電流**: 約 25～35 mA
- **出力**: アナログ電圧（ガスを検知すると上昇する）
- **ロード抵抗（コード上の値）**: 10 kΩ（`LOAD_RESISTANCE`）

## ハードウェア接続

### ピン配置

```
MICS5524ボード  ←→  ESP32-C3 Super Mini
─────────────────────────────────────
GND           ←→  GND
VCC           ←→  5V
OUT           ←→  GPIO3 (ADC1_CH3) [MICS5524_PIN]
EN            ←→  GPIO10           [MICS5524_HEATER_PIN]
```

### 接続詳細

| MICS5524ピン | 説明 | ESP32-C3ピン | 説明 |
|---|---|---|---|
| VCC | 電源（5V） | 5V | 5V電源 |
| GND | グラウンド | GND | グラウンド |
| OUT | アナログ出力 | GPIO3 (ADC1_CH3) | アナログ入力 |
| EN | ヒーター制御（LOW = ON / HIGH = OFF） | GPIO10 | デジタル出力 |

> GPIO1 は e-Paper の DC ピンなので使えません。他のピン割り当ては e-Paper SPI（CLK=GPIO4, MOSI=GPIO6, CS=GPIO7, DC=GPIO1, RST=GPIO2, BUSY=GPIO5）、I2C（SDA=GPIO8, SCL=GPIO9）、MAX9814（GPIO0）です。

### ⚠️ 5V 給電と ADC 入力の注意
- 5V で給電しているため、ガス濃度が非常に高いと OUT の電圧が **3.3V（ESP32-C3 の GPIO 入力上限）を超える**ことがあります。強い蒸気にさらす場合（アルコールを直接吹きかけるなど）は、OUT と GPIO3 の間に分圧回路を入れることを検討してください（分圧した場合は、コード側で電圧を元の値に戻す必要があります）。
- ESP32-C3 の ADC（11dB 減衰）で測れるのは約 2.5V までです。それ以上の出力は頭打ちになります。

### ロード抵抗 (RL) について
コードはセンサー抵抗 Rs を次の式で求めます（`CIRCUIT_VOLTAGE = 5.0`、`LOAD_RESISTANCE = 10000.0`）。

```
Rs = RL × (Vc − Vout) / Vout
```

Rs/Ro は比なので RL の値は打ち消され、Rs/Ro と ppm は RL の値に依存しません。RL が影響するのはシリアルに出す抵抗値（Ω）の絶対値だけです。

## ソフトウェア実装

### ファイル構成
- `include/Custom_MICS5524.h` / `src/Custom_MICS5524.cpp` … ドライバ本体
- `include/adc_util.h` … ADC 生値 → 電圧変換（`adcRawToVolts()`、MAX9814 と共用）

### 初期化とキャリブレーション（`main.cpp`）
```cpp
#define MICS5524_PIN         3   // GPIO3  (ADC1_CH3)
#define MICS5524_HEATER_PIN  10  // GPIO10 (EN ピン)

CustomMICS5524 mics5524(MICS5524_PIN, MICS5524_HEATER_PIN);

// setup()
mics5524.begin();  // ヒーター ON（EN=LOW）、ウォームアップ開始

// loop()（10秒ごと）
// ヒーターが温まったら一度だけ新鮮な空気の基準値（Ro）を取る
if (!mics5524.isCalibrated() && mics5524.isWarmedUp()) {
  mics5524.calibrate();
}
mics5524_data = mics5524.read();
```

`setup()` では `calibrate()` を呼びません。起動から3分経ってから `loop()` が一度だけ呼びます。

### データ読み込み
```cpp
MICS5524Data data = mics5524.read();

// 利用可能なデータ
data.raw_value;          // ADC生値（100サンプルの平均, 0〜4095）
data.voltage;            // フィルタ済み電圧 (V)
data.resistance;         // センサー抵抗 Rs (Ω)
data.rs_ro_ratio;        // Rs/Ro 比率（キャリブレーション前は 0）
data.ppm_equivalent;     // ppm換算値（CO基準、キャリブレーション前は 0）
data.heater_enabled;     // ヒーター状態（EN が LOW なら true）
data.calibrated;         // Ro 取得済みか（ウォームアップ中は false）
```

### 制御
```cpp
// ヒーター制御
mics5524.enableHeater(true);   // ヒーター ON（EN=LOW）
mics5524.enableHeater(false);  // ヒーター OFF（EN=HIGH）

// 状態確認
mics5524.isWarmedUp();    // begin() から3分経過したか
mics5524.isCalibrated();  // Ro を取得済みか

// 手動で再キャリブレーション（新鮮な空気中で実行）
mics5524.calibrate();
```

## 主な特徴

### 1. **ADC 電圧変換**
- ESP32-C3 の ADC（11dB 減衰）は 4095 が 3.3V に対応せず、測れるのは約 2.5V まで
- そのため `adcRawToVolts()`（`include/adc_util.h`）で eFuse の校正値を使って電圧に変換
- 校正値の計算は初回だけ行い、以降は使い回す

### 2. **フィルタリング**
- `read()` 1回で 100 サンプルを平均
- さらに直近 5 回分の電圧の中央値を使ってノイズを除去（10秒周期なので約50秒分）

### 3. **キャリブレーション**
- 起動（`begin()`）から **3分間** はヒーターのウォームアップ
- その後 `loop()` から `calibrate()` を **一度だけ** 実行し、その時点の空気を「新鮮な空気」とみなして Ro（基準抵抗）を決定（50サンプル × 10ms の平均）
- Ro は保存されないので、起動のたびに取り直す

### 4. **ppm計算**
- DFRobot_MICS ライブラリと同じ CO 換算の近似式を使用

  ```
  ppm = (0.425 − Rs/Ro) / 0.000405
  ```

- 1 ppm 未満は 0、上限は 1000 ppm
- Rs/Ro が 0.425 を下回るまでは 0 ppm のまま（新鮮な空気では Rs/Ro ≒ 1）

### 5. **ヒーター制御**
- EN ピンが LOW でヒーター ON、HIGH で OFF
- `begin()` でヒーターを ON にする

## 画面表示

`main.cpp` には8つの表示画面があり、10秒ごとに切り替わります（`SCREEN_ENABLED` で有効/無効を設定）：

0. **Air Quality** - ENS160（eCO2/TVOC/AQI）
1. **Climate** - AHT21/BMP280（温度/湿度/気圧/高度）
2. **Ambience** - VEML7700/MAX9814（照度/騒音）
3. **Gas Sensor** - MICS5524（Rs/Ro。ウォームアップ中は "Warming up"）
4. **Date & Time** - NTP 時刻（既定で無効）
5. **Month Day** - 月日
6. **WiFi QR** - WiFi 接続用 QR コード
7. **Climate Modern** - 温湿度の円形ゲージ＋過去24時間グラフ

Gas Sensor 画面に出すのはタイトルと Rs/Ro（またはウォームアップ表示）だけです。電圧・抵抗・ppm・ヒーター状態はシリアルモニターにのみ出力します。

## 出力例

### シリアルモニター
（値は一例）

起動時：
```
💨 MICS5524ガスセンサーを初期化中...✅ MICS5524 initialized successfully
✅ 成功（ウォームアップ3分後にキャリブレーション）
```

ウォームアップ中（起動から3分間）：
```
  💨 ガス(MICS5524)生データ: 1640
  💨 ガス(MICS5524)電圧: 1.000 V
  💨 ガス(MICS5524)抵抗: 40000 Ω
  💨 Rs/Ro比・ppm: ウォームアップ中
  💨 ヒーター: ON
```

3分経過後のキャリブレーション（1回だけ）：
```
📊 MICS5524 calibrated: Ro=40000.00 Ω
```

キャリブレーション後（アルコールを近づけた例）：
```
  💨 ガス(MICS5524)生データ: 3280
  💨 ガス(MICS5524)電圧: 2.000 V
  💨 ガス(MICS5524)抵抗: 15000 Ω
  💨 Rs/Ro比: 0.375
  💨 ppm換算(CO基準): 123.5 ppm
  💨 ヒーター: ON
```

### e-paperディスプレイ（画面3: Gas Sensor）
ウォームアップ中：
```
┌─────────────────────┐
│Gas Sensor           │
│                     │
│                     │
│Warming up           │
│                     │
└─────────────────────┘
```

キャリブレーション後：
```
┌─────────────────────┐
│Gas Sensor           │
│                     │
│                     │
│Rs/Ro: 0.38          │
│                     │
└─────────────────────┘
```

## キャリブレーション手順

1. **新鮮な空気の中で起動する**（換気した部屋や屋外。アルコールなどを近くに置かない）
2. 起動から3分間はウォームアップ（画面は "Warming up"、シリアルは「Rs/Ro比・ppm: ウォームアップ中」）
3. 3分経過後、`loop()` が自動で `calibrate()` を一度だけ実行（シリアルに `📊 MICS5524 calibrated: Ro=... Ω`）
4. やり直したいときは、新鮮な空気の中で再起動する

## トラブルシューティング

### 問題: センサーが反応しない
- **原因**: ヒーターが動いていない、または 5V が来ていない
- **解決**: VCC が 5V に、EN が GPIO10 に接続されているか確認。シリアルの「ヒーター: OFF」は EN が HIGH の状態

### 問題: ppm がずっと 0
- **原因**: 近似式では Rs/Ro が 0.425 を下回るまで 0 ppm になる
- **解決**: 新鮮な空気では正常。変化は Rs/Ro で確認する

### 問題: Rs/Ro が 1 から大きくずれたまま
- **原因**: 汚れた空気の中で起動したため Ro が不適切
- **解決**: 新鮮な空気の中で再起動してキャリブレーションし直す

### 問題: 電圧が約 2.5V で頭打ちになる
- **原因**: ADC の測定上限（11dB 減衰で約 2.5V）。ガス濃度がかなり高い
- **解決**: 3.3V を超えるおそれがあるので、強い蒸気から離す・分圧回路を検討する

### 問題: 値が大きく変動
- **原因**: 温度変化やセンサーの安定化未完了
- **解決**: 中央値フィルタが効くまで（約50秒）待つ。起動直後はしばらく様子を見る

## 拡張のポイント

### より正確なガス判定
現在は DFRobot_MICS ライブラリと同じ CO 換算の近似式を使用しています。係数は `Custom_MICS5524.h` で定義しています：

```cpp
// ppm = (0.425 - Rs/Ro) / 0.000405 （Rs/Ro が下がるほど CO が多い、1〜1000ppm）
const float CO_RATIO_THRESHOLD = 0.425;
const float CO_RATIO_PER_PPM = 0.000405;
```

### 複数ガス識別
MICS5524 単体では複数ガスの同時識別は不可です。複数の異なるセンサーと組み合わせることで実現可能です。

## 参考資料
- [Adafruit MICS5524 Product Page](https://www.adafruit.com/product/3199)
- [DFRobot_MICS ライブラリ](https://github.com/DFRobot/DFRobot_MICS)（ppm 換算式の出典）
- 実装参考: `include/adc_util.h`（ADC 電圧変換）、`Custom_MAX9814.h`（同じ変換を使うアナログセンサー）
