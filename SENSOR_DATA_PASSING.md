# 📊 センサーデータの渡し方について

## 質問
「各種センサーの値ってdisplay_screens.cppに渡してるんですか？」

## 答え
**はい、明示的にパラメータとして渡しています** ✅

---

## 🔄 データフロー

```
main.cpp (loop() でセンサーデータ取得 → グローバルの ens160_data などに保存)
    ↓
    ↓ パラメータとして渡す（switch(display_mode) 内）
    ↓
display_screens.cpp (データ表示)
```

---

## 📋 描画関数のシグネチャ（`include/display_screens.h`）

```cpp
extern GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;

void displayAirQuality(const ENS160Data& ens160_data, bool sensor_enabled);   // 画面0
void displayClimate(                                                          // 画面1
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled
);
void displayAmbience(                                                         // 画面2
  const VEML7700Data& veml7700_data, bool veml7700_enabled,
  const MAX9814Data& max9814_data, bool max9814_enabled
);
void displayGasSensor(const MICS5524Data& mics5524_data, bool sensor_enabled);// 画面3
void displayDateTime(bool isWiFiConnected, const String& dateTimeStr);        // 画面4
void displayMonthDay(int month, int day);                                     // 画面5
void displayWiFiQR(const String& ssid, const String& password);               // 画面6
void displayClimateModern(                                                    // 画面7
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled,
  const float* temp_history, const float* humid_history,
  int history_count, int history_index
);
void displaySettingsQR(const String& url);                                    // 画面8
void displayPressure(                                                         // 画面10
  const BMP280Data& bmp280_data, bool bmp280_enabled,
  float trend_3h  // 約3時間前からの気圧の変化（hPa）。履歴が3時間分たまるまでは NAN
);
```

## 🧩 データ構造体（`include/Custom_*.h`）

| 構造体 | フィールド |
|--------|-----------|
| `ENS160Data` | `eco2`（ppm）, `tvoc`（ppb）, `aqi`（1〜5。0 は未測定） |
| `AHT21Data` | `temperature`, `humidity`（自己発熱補正後）, `raw_temperature`, `raw_humidity`（生値） |
| `BMP280Data` | `temperature`（°C）, `pressure`（hPa、設置場所の気圧）, `sea_level_pressure`（hPa、設定ページの標高で海面の高さに換算した気圧）, `altitude`（m） |
| `VEML7700Data` | `lux`, `white`, `als`（ALS 生データ） |
| `MAX9814Data` | `raw_value`, `voltage`, `peak_to_peak`, `rms_voltage`, `db_level`（中央値フィルタ済）, `db_level_raw` |
| `MICS5524Data` | `raw_value`, `voltage`, `resistance`, `rs_ro_ratio`, `ppm_equivalent`, `heater_enabled`, `calibrated` |

AHT21 の補正量は `main.cpp` の `AHT21_TEMP_OFFSET`（10 °C）で、`aht21.setTemperatureOffset()` で設定します。湿度は補正後の温度に合わせて換算し直されます。

---

## 📝 具体的な例

### 画面0: Air Quality (空気品質)
```cpp
// main.cpp で呼び出し時にデータを渡す
displayAirQuality(ens160_data, sensor_ens160_enabled);

// display_screens.cpp で受け取る
void displayAirQuality(const ENS160Data& ens160_data, bool sensor_enabled) {
  // ens160_data を使用して表示
  display.print((int)ens160_data.eco2);
}
```

### 画面1: Climate (温熱環境)
```cpp
// main.cpp で呼び出し時に複数のデータを渡す
displayClimate(
  aht21_data, sensor_aht21_enabled, 
  bmp280_data, sensor_bmp280_enabled
);

// display_screens.cpp で受け取る
void displayClimate(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled
) {
  // aht21_data.temperature / aht21_data.humidity と
  // bmp280_data.pressure / bmp280_data.altitude を使用
}
```

---

## 🎯 この方法のメリット

### ✅ **データの流れが明確**
- 関数シグネチャを見れば、どのデータが必要かすぐわかる
- 暗黙的な依存がない

```cpp
// 関数の宣言を見るだけで全てわかる
void displayClimate(
  const AHT21Data& aht21_data, bool aht21_enabled,     // ← ここでデータを指定
  const BMP280Data& bmp280_data, bool bmp280_enabled
);
```

### ✅ **テストが容易**
ダミーデータを渡すだけで、表示の確認ができる：

```cpp
// テストコード例
ENS160Data test_data = {
  .eco2 = 400,
  .tvoc = 50,
  .aqi = 1   // 1=Excellent（0 は未測定扱いで "unknown" と表示される）
};
displayAirQuality(test_data, true);  // ← テスト実行
```

### ✅ **グローバル変数への依存を削減**
- センサーデータの `extern` 宣言が不要
- スコープが明確
- ※ `display` オブジェクト（`display_screens.h`）と `getAQIStatusEnglish()`（`main.cpp` で定義し `display_screens.cpp` で `extern` 宣言）は引き続き `extern` で共有

### ✅ **再利用性が高い**
異なるデータソース（ファイル、データベース等）でも同じ表示関数が使用可能

---

## 🔀 別の方法との比較

### 方法1: 現在の実装（パラメータ渡し） ⭐ **推奨**
```cpp
displayAirQuality(ens160_data, sensor_ens160_enabled);
```
- ✅ 依存関係が明確
- ✅ テスト容易
- ✅ 再利用性高い

### 方法2: extern宣言（以前の実装）
```cpp
extern ENS160Data ens160_data;  // display_screens.cppで宣言
displayAirQuality();  // パラメータなし
```
- ❌ グローバル変数が増加
- ❌ 依存関係が暗黙的
- ❌ テストが難しい

### 方法3: 構造体でまとめる（将来の拡張案・未実装）
```cpp
struct SensorData {
  ENS160Data ens160;
  AHT21Data aht21;
  BMP280Data bmp280;
  // ... 他のセンサー
};

displayAirQuality(sensor_data);
```
- ✅ パラメータが少なくスッキリ
- ✅ センサー追加時は構造体にメンバーを足すだけ

---

## 📋 main.cpp でのデータ渡し（`loop()` の `switch`）

```cpp
// 画面表示関数の呼び出し
switch(display_mode) {
  case 0:
    // 空気品質画面 - ENS160データを渡す
    displayAirQuality(ens160_data, sensor_ens160_enabled);
    break;
    
  case 1:
    // 温熱環境画面 - AHT21 と BMP280データを渡す
    displayClimate(aht21_data, sensor_aht21_enabled, bmp280_data, sensor_bmp280_enabled);
    break;
    
  case 2:
    // 視聴覚環境画面 - VEML7700 と MAX9814データを渡す
    displayAmbience(veml7700_data, sensor_veml7700_enabled, max9814_data, sensor_max9814_enabled);
    break;
    
  case 3:
    // ガスセンサー画面 - MICS5524データを渡す
    displayGasSensor(mics5524_data, sensor_mics5524_enabled);
    break;
    
  case 4:
    // 日時表示画面 - WiFi状態（関数呼び出し）と現在時刻を渡す
    displayDateTime(isWiFiConnected(), getCurrentDateTime());
    break;

  case 5: {
    // 月日表示画面 - 現在時刻から月と日を取り出して渡す
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    displayMonthDay(timeinfo.tm_mon + 1, timeinfo.tm_mday);
    break;
  }

  case SCREEN_WIFI_QR:
    // WiFi QR 画面 - 保存済みの SSID / パスワード（AP モード中は本体の AP）を渡す
    if (isAPMode()) {
      displayWiFiQR(getAPSSID(), "");
    } else {
      displayWiFiQR(settings.wifi_ssid, settings.wifi_password);
    }
    break;

  case 7:
    // Climate Modern 画面 - 温湿度データと24時間分の履歴を渡す
    displayClimateModern(aht21_data, sensor_aht21_enabled, bmp280_data, sensor_bmp280_enabled,
                         temp_history, humid_history, history_count, history_index);
    break;

  case SCREEN_SETTINGS_QR:
    // 設定ページ QR 画面 - 設定ページの URL を渡す
    displaySettingsQR(getSettingsURL());
    break;

  case SCREEN_WEATHER:
    // 天気画面 - 取得済みの天気（weather.cpp）と、地域の設定・WiFi の状態を渡す
    displayWeather(getWeather(), hasWeatherLocation(), isWiFiConnected());
    break;

  case 10:
    // Pressure 画面 - 気圧データと、約3時間前からの変化を渡す
    displayPressure(bmp280_data, sensor_bmp280_enabled, pressureTrend3h(bmp280_data.pressure));
    break;
}
```

- `isWiFiConnected()` は関数（`network.cpp`。`WiFi.status() == WL_CONNECTED` を返す）で、グローバル変数ではありません
- 天気は `loop()` の先頭で呼ぶ `weatherLoop()`（`weather.cpp`）が取得して保持し、`getWeather()` で `WeatherData` を受け取ります。描画関数は通信をしません
- 時刻同期の判定は `isTimeSynced()`（`time(nullptr) > 1704067200`、つまり 2024-01-01 以降なら同期済み）
- `temp_history` / `humid_history` は `HISTORY_SIZE`（144、`display_screens.h` で定義）要素のリングバッファで、10分ごとに保存されます。表示側の添字計算も同じ `HISTORY_SIZE` を使うので、件数を変えるときは定義を1か所変えるだけで済みます
- 気圧の履歴 `pressure_history` は `main.cpp` の中だけで使う18件のリングバッファ（10分ごと、hPa×10 の整数）。表示側には `pressureTrend3h()` で計算した変化だけを渡します

---

## 💡 ポイント

- **センサーデータは参照渡し** (`const ENS160Data&` など) を使用
  - メモリ効率が良い
  - 値の変更が不可（const指定）
  - 月日は `int`、WiFi 情報は `const String&`、履歴は `const float*` で渡す

- **各画面が独立**
  - 必要なデータのみを受け取る
  - 不要な依存がない

- **保守性が向上**
  - 新しい画面追加時も明確
  - データソースの変更が容易

---

## ✅ 検証済み

- ✅ 全データが正しく渡されている
- ✅ リンカエラーなし
- ✅ 依存関係が明確
- ✅ 今後のテスト実装が容易
