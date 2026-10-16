#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <WiFi.h>
#include <time.h>
#include "Custom_ENS160.h"
#include "Custom_AHT21.h"
#include "Custom_BMP280.h"
#include "Custom_VEML7700.h"
#include "Custom_MAX9814.h"
#include "Custom_MICS5524.h"
#include "display_screens.h"
#include "settings.h"
#include "network.h"
#include "web_settings.h"
#include "weather.h"

// ============================================================================
// GxEPD2 用フォント定義
// ============================================================================
// Adafruit GFX Library フォント
// フォントのデータは読み込んだ .cpp ごとに1つずつ入るので、main.cpp と display_screens.cpp で
// 同じフォントを使うと2つ入ってしまう。main.cpp では全画面共通のフォントだけを使い、
// 各画面のフォントは display_screens.cpp で読み込む
#include <Fonts/FreeMonoBold12pt7b.h>

// ============================================================================
// WiFi + NTP 設定
// ============================================================================

// NTPサーバーとタイムゾーンは設定ページで変更できる（settings.h）
// 設定したサーバーが応答しないときの予備
#define NTP_SERVER_FALLBACK "time.nist.gov"

// これより前の時刻は未同期とみなす（2024-01-01 00:00:00 UTC）
#define TIME_SYNCED_THRESHOLD 1704067200

// WiFi は切断されても自動で再接続し（network.cpp）、NTP も ESP-IDF が定期的に（3時間ごと）
// 再同期するので、接続状態や同期状態は都度問い合わせる

// ============================================================================
// 関数プロトタイプ宣言
// ============================================================================
void setupNTP();
void applyTimeSettings();
bool isTimeSynced();
String getCurrentDateTime();

// ============================================================================
// ESP32-C3 Super Mini (K2) 向け ピン配置
// ============================================================================
// ============================================================================
// WeAct Studio 1.54" e-paper (200x200 SSD1681) - ドライバ: GxEPD2_154_D67
// 参照: https://github.com/WeActStudio/WeActStudio.EpaperModule/blob/master/Example/EpaperModuleTest_Arduino_ESP32C3/EpaperModuleTest_Arduino_ESP32C3.ino
// ============================================================================
// SPI用ピン (公式推奨配置に修正)
#define EPD_CLK   4   // GPIO4  (SPI Clock / SCL)
#define EPD_MOSI  6   // GPIO6  (SPI Data / SDA)
#define EPD_CS    7   // GPIO7  (Chip Select / CS)
#define EPD_DC    1   // GPIO1  (Data/Command / DC)
#define EPD_RST   2   // GPIO2  (Reset / RES)
#define EPD_BUSY  5   // GPIO5  (Busy Signal / BUSY)

// I2C用ピン (センサー用)
#define I2C_SDA   8   // GPIO8  (I2C SDA)
#define I2C_SCL   9   // GPIO9  (I2C SCL)

// アナログ入力ピン (MAX9814マイクセンサー用)
#define MAX9814_PIN   0   // GPIO0  (ADC0 - マイクセンサー出力)

// アナログ入力ピン (MICS5524ガスセンサー用)
#define MICS5524_PIN   3   // GPIO3  (ADC1_CH3 - MICS5524ガスセンサー出力)
#define MICS5524_HEATER_PIN  10   // GPIO10 (ヒーター制御 EN ピン)

// AHT21 の自己発熱補正（ガスセンサーのヒーター等で室温より約10°C高く出るため）
// 温度からこの値を引き、湿度も同じ温度差で換算し直す
#define AHT21_TEMP_OFFSET  10.0f

// ============================================================================
// e-paper初期化: WeAct 1.54" 200x200 SSD1681
// ============================================================================
// ドライバ: GxEPD2_154_D67 (公式サンプルコードから確認済み - 1.54" 200x200用)
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
    GxEPD2_154_D67(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY)
);

// センサーオブジェクト
CustomENS160 ens160;
CustomAHT21 aht21;
CustomBMP280 bmp280;
CustomVEML7700 veml7700;
CustomMAX9814 max9814(MAX9814_PIN);
CustomMICS5524 mics5524(MICS5524_PIN, MICS5524_HEATER_PIN);

// センサーデータ（グローバル）
ENS160Data ens160_data;
AHT21Data aht21_data;
BMP280Data bmp280_data;
VEML7700Data veml7700_data;
MAX9814Data max9814_data;
MICS5524Data mics5524_data;

// センサー有効フラグ
bool sensor_ens160_enabled = false;
bool sensor_aht21_enabled = false;
bool sensor_bmp280_enabled = false;
bool sensor_veml7700_enabled = false;
bool sensor_max9814_enabled = false;
bool sensor_mics5524_enabled = false;

// ============================================================================
// データ履歴管理（過去24時間分）
// ============================================================================
// HISTORY_SIZE（144 = 24時間 ÷ 10分）は display_screens.h で定義
float temp_history[HISTORY_SIZE];
float humid_history[HISTORY_SIZE];
int history_index = 0;
int history_count = 0;  // 実際に保存されているデータ数（起動直後は0から増える）
unsigned long last_history_save = 0;
const unsigned long HISTORY_INTERVAL = 10 * 60 * 1000;  // 10分（ミリ秒）

// ============================================================================
// 気圧の履歴（Pressure 画面の3時間の傾向矢印用）
// ============================================================================
// 温湿度の履歴と同じ10分ごとに、設置場所の気圧を hPa×10 の整数で保存する
// 18件たまると、一番古い値が約3時間前（170〜180分前）になる
#define PRESSURE_HISTORY_SIZE 18
int16_t pressure_history[PRESSURE_HISTORY_SIZE];
uint8_t pressure_history_index = 0;
uint8_t pressure_history_count = 0;

// ============================================================================
// 画面表示の有効/無効設定
// ============================================================================
// 各画面の有効/無効は設定ページで変更できる（NVS に保存）
// ここは設定を保存する前（初回起動時・初期化後）の既定値
// 無効な画面はスキップして次の有効な画面を表示します
// ============================================================================
// 画面の番号は NVS に保存した screen_mask のビット位置なので、新しい画面は末尾に足す
#define NUM_SCREENS 11
#define SCREEN_WIFI_QR 6
#define SCREEN_SETTINGS_QR 8
#define SCREEN_WEATHER 9
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

// 設定ページのチェックボックスに出す画面名
const char* const SCREEN_NAMES[NUM_SCREENS] = {
  "Air Quality（空気質）",
  "Climate（温熱環境）",
  "Ambience（視聴覚環境）",
  "Gas Sensor（ガスセンサー）",
  "Date & Time（日時）",
  "Month Day（月日）",
  "WiFi QR",
  "Climate Modern（温湿度グラフ）",
  "Settings QR（設定ページ）",
  "Weather（天気）",
  "Pressure（気圧 H/L）",
};

uint16_t defaultScreenMask() {
  uint16_t mask = 0;
  for (int i = 0; i < NUM_SCREENS; i++) {
    if (SCREEN_DEFAULT_ENABLED[i]) mask |= 1 << i;
  }
  return mask;
}

// 画面 i を表示するか（AP モード中は WiFi QR と設定ページ QR だけを交互に出す）
bool isScreenEnabled(uint8_t i) {
  if (isAPMode()) {
    return i == SCREEN_WIFI_QR || i == SCREEN_SETTINGS_QR;
  }
  return settings.screen_mask & (1 << i);
}

// current の次に表示する有効な画面（有効な画面がなければ current のまま）
uint8_t nextEnabledScreen(uint8_t current) {
  uint8_t next = current;
  do {
    next = (next + 1) % NUM_SCREENS;
  } while (!isScreenEnabled(next) && next != current);
  return next;
}

// 有効な画面数を計算
uint8_t countEnabledScreens() {
  uint8_t count = 0;
  for (int i = 0; i < NUM_SCREENS; i++) {
    if (isScreenEnabled(i)) count++;
  }
  return count;
}

// 気圧の履歴に今の値を足す
// 設置場所の気圧を保存するので、設定ページで標高を変えても傾向はずれない
void savePressureHistory(float pressure) {
  pressure_history[pressure_history_index] = (int16_t)lroundf(pressure * 10);
  pressure_history_index = (pressure_history_index + 1) % PRESSURE_HISTORY_SIZE;
  if (pressure_history_count < PRESSURE_HISTORY_SIZE) {
    pressure_history_count++;
  }
}

// 約3時間前からの気圧の変化（hPa）。履歴がまだ3時間分たまっていなければ NAN
float pressureTrend3h(float pressure) {
  if (pressure_history_count < PRESSURE_HISTORY_SIZE) {
    return NAN;
  }
  // 満タンのときは、次に書き込む位置に一番古い値が入っている
  return pressure - pressure_history[pressure_history_index] / 10.0f;
}

// 関数プロトタイプ宣言
void printAQIStatus(uint8_t aqi);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================================================");
  Serial.println("  ESP32-C3 Super Mini (K2) + WeAct 1.54\" e-paper 起動");
  Serial.println("=================================================================\n");
  
  // 設定ページで保存した設定を NVS から読み込む
  loadSettings(defaultScreenMask(), NUM_SCREENS);
  
  // I2C バス初期化（GPIO8=SDA, GPIO9=SCL）
  Serial.print("📡 I2Cバスを初期化中...");
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
  Serial.println("✅ 成功");
  
  // ENS160初期化
  Serial.print("📊 ENS160を初期化中...");
  ens160.begin();
  if (ens160.available()) {
    Serial.println("✅ 成功");
    sensor_ens160_enabled = true;
  } else {
    Serial.println("⚠️  スキップ（デバイス検出不可）");
  }
  
  // AHT21初期化
  Serial.print("🌡️  AHT21を初期化中...");
  aht21.begin();
  aht21.setTemperatureOffset(AHT21_TEMP_OFFSET);
  // AHT21の有効性を確認（簡易版：存在すればOK）
  Wire.beginTransmission(0x38);
  if (Wire.endTransmission() == 0) {
    Serial.println("✅ 成功");
    sensor_aht21_enabled = true;
  } else {
    Serial.println("⚠️  スキップ（I2Cアドレス 0x38 が見つかりません）");
  }
  
  // BMP280初期化
  Serial.print("🔽 BMP280を初期化中...");
  bmp280.begin();
  // BMP280の有効性を確認（0x76 または 0x77）
  Wire.beginTransmission(0x76);
  byte bmp280_error = Wire.endTransmission();
  if (bmp280_error != 0) {
    Wire.beginTransmission(0x77);
    bmp280_error = Wire.endTransmission();
  }
  if (bmp280_error == 0) {
    Serial.println("✅ 成功");
    sensor_bmp280_enabled = true;
  } else {
    Serial.println("⚠️  スキップ（I2Cアドレス 0x76/0x77 が見つかりません）");
  }
  
  // VEML7700初期化
  Serial.print("💡 VEML7700を初期化中...");
  veml7700.begin();
  // VEML7700の有効性を確認
  Wire.beginTransmission(0x10);
  if (Wire.endTransmission() == 0) {
    Serial.println("✅ 成功");
    sensor_veml7700_enabled = true;
  } else {
    Serial.println("⚠️  スキップ（I2Cアドレス 0x10 が見つかりません）");
  }
  
  // MAX9814初期化
  Serial.print("🔊 MAX9814マイクセンサーを初期化中...");
  max9814.begin();
  Serial.println("✅ 成功");
  sensor_max9814_enabled = true;
  
  // MICS5524初期化
  Serial.print("💨 MICS5524ガスセンサーを初期化中...");
  mics5524.begin();
  // キャリブレーションはヒーターが温まってから loop() で実行する
  Serial.println("✅ 成功（ウォームアップ3分後にキャリブレーション）");
  sensor_mics5524_enabled = true;
  
  // SPI初期化
  Serial.print("📡 SPIバスを初期化中...");
  SPI.begin(EPD_CLK, -1, EPD_MOSI);  // MISO は使わないので -1
  Serial.println("✅ 成功");
  
  // e-paper初期化
  Serial.print("📺 e-Paperを初期化中...");
  // 公式サンプルコードパラメータを使用
  display.init(115200, true, 50, false);  // 公式のdelayTime=50に変更
  delay(100);
  display.setRotation(0);  // 回転調整（0-3）
  display.setFullWindow();
  Serial.println("✅ 成功");
  
  // ===================================================================
  // e-paper デバッグ情報出力
  // ===================================================================
  Serial.println("\n📺 e-Paper デバッグ情報:");
  Serial.print("   - 画面サイズ: ");
  Serial.print(display.width());
  Serial.print(" × ");
  Serial.print(display.height());
  Serial.println(" px");
  Serial.println("   - ドライバIC: SSD1681");
  Serial.print("   - 回転: ");
  Serial.println(display.getRotation());
  
  // ===================================================================
  // 重要: 解像度チェック
  // ===================================================================
  if (display.width() != 200 || display.height() != 200) {
    Serial.println("\n⚠️  警告: 画面サイズが 200x200 ではありません！");
    Serial.println("   - 別のドライバを試してください");
    Serial.println("   - GxEPD2_154 を試してみてください");
  } else {
    Serial.println("\n✅ 画面サイズ確認: 200x200 OK");
  }
  
  Serial.println("\n✅ 全初期化完了！\n");
  
  // ===================================================================
  // I2C スキャン（デバッグ用）
  // ===================================================================
  Serial.println("📡 I2Cバススキャン中...");
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("  ✅ I2Cデバイス検出: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println("  ⚠️  I2Cデバイスが見つかりません");
  } else {
    Serial.print("  合計: ");
    Serial.print(nDevices);
    Serial.println(" 個のデバイス検出");
  }
  Serial.println();
  
  // e-paper 初期表示 - テストコード（シンプル）
  delay(500);
  
  Serial.println("\n📺 e-paper に背景色を表示中...");
  
  display.setFullWindow();
  display.firstPage();
  do {
    // 単純にホワイトで塗りつぶす
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
  
  Serial.println("   ✅ 背景（白）表示完了");
  delay(2000);
  
  Serial.println("\n📺 e-paper にテキストを表示中...");
  
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(10, 50);
    display.println("TEST");
    display.setCursor(10, 100);
    display.println("SCREEN");
  } while (display.nextPage());
  
  Serial.println("   ✅ テキスト表示完了");
  delay(2000);

  // ===================================================================
  // WiFi + NTP + 設定ページ 初期化
  // ===================================================================
  setupNetwork();  // つながらなければ AP モードになる
  setupNTP();
  setupWebSettings(SCREEN_NAMES, NUM_SCREENS);
  
  // ===================================================================
  // セットアップ完了サマリー
  // ===================================================================
  Serial.println("\n=================================================================");
  Serial.println("  🎉 セットアップ完了");
  Serial.println("=================================================================");
  
  Serial.println("\n📡 WiFi接続状態:");
  if (isAPMode()) {
    Serial.printf("   📶 APモード: %s（パスワードなし）\n", getAPSSID().c_str());
    Serial.printf("   📶 設定ページ: %s\n", getSettingsURL().c_str());
  } else if (isWiFiConnected()) {
    Serial.printf("   ✅ SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("   ✅ IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("   ✅ RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("   ✅ 設定ページ: %s\n", getSettingsURL().c_str());
  } else {
    Serial.println("   ⚠️  WiFi未接続（バックグラウンドで再接続を継続）");
  }

  Serial.println("\n🕐 NTP時刻同期状態:");
  if (isTimeSynced()) {
    Serial.println("   ✅ 同期済み");
    Serial.printf("   ✅ 現在時刻: %s (%s)\n", getCurrentDateTime().c_str(), settings.timezone.c_str());
  } else {
    Serial.println("   ⚠️  未同期（WiFi接続後に自動で同期）");
  }
  
  Serial.println("\n🌤  天気の地域:");
  if (hasWeatherLocation()) {
    Serial.printf("   ✅ %s（%s, %s）\n", settings.weather_place.c_str(),
                  settings.weather_lat.c_str(), settings.weather_lon.c_str());
  } else {
    Serial.println("   ⚠️  未設定（設定ページで選ぶ）");
  }

  Serial.println("\n📊 センサー初期化状態:");
  Serial.printf("   %s ENS160 (空気品質)\n", sensor_ens160_enabled ? "✅" : "⚠️");
  Serial.printf("   %s AHT21 (温湿度)\n", sensor_aht21_enabled ? "✅" : "⚠️");
  Serial.printf("   %s BMP280 (気圧)\n", sensor_bmp280_enabled ? "✅" : "⚠️");
  Serial.printf("   %s VEML7700 (光)\n", sensor_veml7700_enabled ? "✅" : "⚠️");
  Serial.printf("   %s MAX9814 (マイク)\n", sensor_max9814_enabled ? "✅" : "⚠️");
  Serial.printf("   %s MICS5524 (ガス)\n", sensor_mics5524_enabled ? "✅" : "⚠️");
  
  Serial.println("\n📺 ディスプレイ:");
  Serial.printf("   ✅ e-Paper: %d × %d px\n", display.width(), display.height());
  
  // ===================================================================
  // データ履歴の初期化（起動時のセンサー値で埋める）
  // ===================================================================
  Serial.println("\n📊 データ履歴を初期化中...");
  float init_temp = 20.0;  // デフォルト値
  float init_humid = 50.0; // デフォルト値
  
  // センサーから初期値を取得
  if (sensor_aht21_enabled) {
    AHT21Data init_data = aht21.read();
    init_temp = init_data.temperature;  // 自己発熱補正済み
    init_humid = init_data.humidity;
    Serial.printf("   ✅ AHT21から初期値取得: 温度=%.1f°C, 湿度=%.0f%%\n", init_temp, init_humid);
  } else if (sensor_bmp280_enabled) {
    BMP280Data init_data = bmp280.read();
    init_temp = init_data.temperature;
    init_humid = 0.0;  // BMP280には湿度センサーなし
    Serial.printf("   ✅ BMP280から初期値取得: 温度=%.1f°C\n", init_temp);
  } else {
    Serial.println("   ⚠️  センサーなし、デフォルト値使用");
  }
  
  // 全履歴を初期値で埋める
  for (int i = 0; i < HISTORY_SIZE; i++) {
    temp_history[i] = init_temp;
    humid_history[i] = init_humid;
  }
  history_count = HISTORY_SIZE;  // 履歴を満タンに
  history_index = 0;
  last_history_save = millis();  // 初回保存時刻を記録
  
  Serial.printf("   ✅ %d個のデータポイントを初期化完了\n", HISTORY_SIZE);

  // 気圧の履歴は起動時の値を1件だけ入れる
  // （温湿度のように今の値で埋めると、3時間たつまで傾向が「横ばい」と出てしまう）
  if (sensor_bmp280_enabled) {
    savePressureHistory(bmp280.read().pressure);
  }

  Serial.println("\n=================================================================");
  Serial.println("  準備完了！ loop開始\n");
  Serial.println("=================================================================\n");

}

void loop() {
  static uint8_t display_mode = 0;  // 表示モード切り替え
  static unsigned long last_update = 0;
  static unsigned long last_full_refresh = 0;  // 最後のフルリフレッシュ時刻
  static bool first_display = true;  // 初回フルリフレッシュフラグ
  unsigned long current_time = millis();
  
  const unsigned long FULL_REFRESH_INTERVAL = 10 * 60 * 1000;  // 10分

  // WiFi の監視（つながらなければ AP モードへ）と設定ページのリクエスト処理
  networkLoop();
  webSettingsLoop();

  // 天気の取得（30分ごと、地域を変えたときはすぐ）
  weatherLoop();

  // 切り替え間隔（既定10秒）ごとにセンサー値取得 & 画面更新
  if (current_time - last_update >= settings.interval_sec * 1000UL || first_display) {
    last_update = current_time;
    
    // 設定の変更や AP モードへの切り替えで今の画面が無効になっていたら、次の有効な画面へ
    if (!isScreenEnabled(display_mode)) {
      display_mode = nextEnabledScreen(display_mode);
    }
    
    // センサー値の取得
    // 有効なセンサーのみ読み込む
    if (sensor_ens160_enabled && ens160.available()) {
      ens160_data = ens160.read();
    }
    if (sensor_aht21_enabled) {
      aht21_data = aht21.read();
    }
    if (sensor_bmp280_enabled) {
      // 設定ページで標高を変えたらすぐ海面気圧に反映されるよう、毎回渡す
      bmp280.setStationAltitude(settings.altitude_m);
      bmp280_data = bmp280.read();
    }
    if (sensor_veml7700_enabled) {
      veml7700_data = veml7700.read();
    }
    if (sensor_max9814_enabled) {
      max9814_data = max9814.read();
    }
    if (sensor_mics5524_enabled) {
      // ヒーターが温まったら一度だけ新鮮な空気の基準値（Ro）を取る
      // 設定ページから要求されたときも取り直す
      if (mics5524.isWarmedUp() && (!mics5524.isCalibrated() || mics5524_calibration_requested)) {
        mics5524.calibrate();
        mics5524_calibration_requested = false;
      }
      mics5524_data = mics5524.read();
    }
    
    // ========================================================================
    // データ履歴の保存（10分ごと）
    // ========================================================================
    if (current_time - last_history_save >= HISTORY_INTERVAL) {
      if (sensor_aht21_enabled) {
        // AHT21の値は自己発熱補正済み（温度・湿度とも）
        temp_history[history_index] = aht21_data.temperature;
        humid_history[history_index] = aht21_data.humidity;
      } else if (sensor_bmp280_enabled) {
        temp_history[history_index] = bmp280_data.temperature;
        humid_history[history_index] = 0;  // BMP280には湿度センサーなし
      }
      if (sensor_bmp280_enabled) {
        savePressureHistory(bmp280_data.pressure);
      }

      history_index = (history_index + 1) % HISTORY_SIZE;
      if (history_count < HISTORY_SIZE) {
        history_count++;
      }
      last_history_save = current_time;
      
      Serial.println("\n📊 データ履歴を保存しました");
      Serial.printf("   履歴数: %d/%d\n", history_count, HISTORY_SIZE);
    }
    
    // どれか1つ以上のセンサーが有効なら表示
    if (sensor_ens160_enabled || sensor_aht21_enabled || sensor_bmp280_enabled || sensor_veml7700_enabled || sensor_max9814_enabled || sensor_mics5524_enabled) {
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      Serial.print("📺 画面: Screen ");
      Serial.print(display_mode + 1);
      Serial.print("/");
      Serial.println(NUM_SCREENS);
      Serial.println("📊 センサーデータ更新:");
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      
      // ENS160
      if (sensor_ens160_enabled) {
        Serial.print("  eCO2: ");
        Serial.print((uint16_t)ens160_data.eco2);
        Serial.println(" ppm");
        
        Serial.print("  TVOC: ");
        Serial.print((uint16_t)ens160_data.tvoc);
        Serial.println(" ppb");
        
        Serial.print("  AQI : ");
        Serial.print(ens160_data.aqi);
        Serial.print(" (");
        printAQIStatus(ens160_data.aqi);
        Serial.println(")");
        if (ens160_data.validity != ENS160_VALIDITY_NORMAL) {
          Serial.printf("  ⏳ ENS160 準備中（有効フラグ %u、進み具合 %d%%）\n",
                        ens160_data.validity, (int)(ens160_data.warmup_progress * 100));
        }
      } else {
        Serial.println("  📊 ENS160: スキップ（未接続）");
      }
      
      // AHT21
      if (sensor_aht21_enabled) {
        Serial.print("  🌡️  温度(AHT21): ");
        Serial.print(aht21_data.temperature, 2);
        Serial.print(" °C (生値 ");
        Serial.print(aht21_data.raw_temperature, 2);
        Serial.println(" °C)");

        Serial.print("  💧 湿度(AHT21): ");
        Serial.print(aht21_data.humidity, 2);
        Serial.print(" % (生値 ");
        Serial.print(aht21_data.raw_humidity, 2);
        Serial.println(" %)");
      } else {
        Serial.println("  🌡️  AHT21: スキップ（未接続）");
      }
      
      // BMP280
      if (sensor_bmp280_enabled) {
        Serial.print("  🌡️  温度(BMP280): ");
        Serial.print(bmp280_data.temperature, 2);
        Serial.println(" °C");
        
        Serial.print("  🔽 気圧(BMP280): ");
        Serial.print(bmp280_data.pressure, 1);
        Serial.println(" hPa");

        Serial.printf("  🔽 海面気圧(標高 %d m で換算): %.1f hPa\n",
                      settings.altitude_m, bmp280_data.sea_level_pressure);

        Serial.print("  📍 高度(BMP280): ");
        Serial.print(abs(bmp280_data.altitude), 1);
        Serial.println(" m");
      } else {
        Serial.println("  🔽 BMP280: スキップ（未接続）");
      }
      
      // VEML7700
      if (sensor_veml7700_enabled) {
        Serial.print("  💡 照度(VEML7700): ");
        Serial.print(veml7700_data.lux, 2);
        Serial.println(" lux");
        
        Serial.print("  ⚪ ホワイト(VEML7700): ");
        Serial.print(veml7700_data.white, 2);
        Serial.println("");
        
        Serial.print("  📊 ALS生データ(VEML7700): ");
        Serial.println(veml7700_data.als);
      } else {
        Serial.println("  💡 VEML7700: スキップ（未接続）");
      }
      
      // MAX9814
      if (sensor_max9814_enabled) {
        Serial.print("  🔊 マイク(MAX9814)生データ: ");
        Serial.println(max9814_data.raw_value);
        
        Serial.print("  🔊 マイク(MAX9814)電圧: ");
        Serial.print(max9814_data.voltage, 3);
        Serial.println(" V");
        
        Serial.print("  🔊 マイク(MAX9814)ピークツーピーク: ");
        Serial.print(max9814_data.peak_to_peak, 3);
        Serial.println(" V");
        
        Serial.print("  🔊 マイク(MAX9814)RMS: ");
        Serial.print(max9814_data.rms_voltage, 4);
        Serial.println(" V");
        
        Serial.print("  🔊 騒音レベル（測定値）: ");
        Serial.print(max9814_data.db_level_raw, 1);
        Serial.println(" dB");
        
        Serial.print("  🔊 騒音レベル（中央値）: ");
        Serial.print(max9814_data.db_level, 1);
        Serial.println(" dB");
      }
      
      // MICS5524
      if (sensor_mics5524_enabled) {
        Serial.print("  💨 ガス(MICS5524)生データ: ");
        Serial.println(mics5524_data.raw_value);
        
        Serial.print("  💨 ガス(MICS5524)電圧: ");
        Serial.print(mics5524_data.voltage, 3);
        Serial.println(" V");
        
        Serial.print("  💨 ガス(MICS5524)抵抗: ");
        Serial.print(mics5524_data.resistance, 0);
        Serial.println(" Ω");
        
        if (mics5524_data.calibrated) {
          Serial.print("  💨 Rs/Ro比: ");
          Serial.print(mics5524_data.rs_ro_ratio, 3);
          Serial.print("（Ro ");
          Serial.print(mics5524_data.ro, 0);
          Serial.print(" Ω、測れる下限 ");  // ガス画面の HP はこの下限で 0、1.0 で 100
          Serial.print(mics5524_data.rs_ro_min, 3);
          Serial.println("）");

          Serial.print("  💨 ppm換算(CO基準): ");
          Serial.print(mics5524_data.ppm_equivalent, 1);
          Serial.println(" ppm");
        } else {
          Serial.println("  💨 Rs/Ro比・ppm: ウォームアップ中");
        }
        
        Serial.print("  💨 ヒーター: ");
        Serial.println(mics5524_data.heater_enabled ? "ON" : "OFF");
      }
      
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    }
    
    // e-paper 表示更新
    // ===================================================================
    // 重要: 初回はフルリフレッシュ、その後は部分リフレッシュ
    // 10分ごとにゴースト軽減のためフルリフレッシュ
    // （フルリフレッシュは約1.5秒かかり画面が白黒に点滅するため毎回は行わない）
    // ===================================================================
    if (first_display) {
      display.setFullWindow();
      first_display = false;
      last_full_refresh = current_time;
      Serial.println("🔄 初回フルリフレッシュ実行");
    } else if (current_time - last_full_refresh >= FULL_REFRESH_INTERVAL) {
      // 10分ごとのフルリフレッシュ（ゴースト軽減）
      display.setFullWindow();
      last_full_refresh = current_time;
      Serial.println("🔄 定期フルリフレッシュ実行（ゴースト軽減）");
    } else {
      // 通常は部分リフレッシュ
      display.setPartialWindow(0, 0, display.width(), display.height());
    }
    
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.setTextColor(GxEPD_BLACK);
      
      // ==========================================
      // 複数画面モード（10秒ごとに切り替え）
      // ================================================
      // 全画面共通フォント設定（統一）
      // ================================================
      display.setFont(&FreeMonoBold12pt7b);
      
      // 画面ごとの表示関数を呼び出し
      // センサーデータを明示的にパラメータとして渡す
      switch(display_mode) {
        case 0:
          displayAirQuality(ens160_data, sensor_ens160_enabled);
          break;
        case 1:
          displayClimate(aht21_data, sensor_aht21_enabled, bmp280_data, sensor_bmp280_enabled);
          break;
        case 2:
          displayAmbience(veml7700_data, sensor_veml7700_enabled, max9814_data, sensor_max9814_enabled);
          break;
        case 3:
          displayGasSensor(mics5524_data, sensor_mics5524_enabled);
          break;
        case 4:
          displayDateTime(isWiFiConnected(), getCurrentDateTime());
          break;
        case 5: {
          // 月日表示画面
          time_t now = time(nullptr);
          struct tm timeinfo;
          localtime_r(&now, &timeinfo);
          displayMonthDay(timeinfo.tm_mon + 1, timeinfo.tm_mday);
          break;
        }
        case SCREEN_WIFI_QR: {
          // WiFi QRコード表示画面（AP モード中は自分の AP に接続するための QR）
          if (isAPMode()) {
            displayWiFiQR(getAPSSID(), "");
          } else {
            displayWiFiQR(settings.wifi_ssid, settings.wifi_password);
          }
          break;
        }
        case 7:
          // Climate Modern（温度/湿度モダンデザイン）
          displayClimateModern(aht21_data, sensor_aht21_enabled, bmp280_data, sensor_bmp280_enabled,
                               temp_history, humid_history, history_count, history_index);
          break;
        case SCREEN_SETTINGS_QR:
          // 設定ページの URL の QRコード
          displaySettingsQR(getSettingsURL());
          break;
        case SCREEN_WEATHER:
          // 天気（今の天気と、今日・明日の予報）
          displayWeather(getWeather(), hasWeatherLocation(), isWiFiConnected());
          break;
        case 10:
          // Pressure（天気図風の H/L と等圧線、3時間の傾向）
          displayPressure(bmp280_data, sensor_bmp280_enabled, pressureTrend3h(bmp280_data.pressure));
          break;
        default:
          // 無効な画面モード（デフォルトケース）
          // 通常は到達しない
          break;
      }
      
    } while (display.nextPage());
    
    // 画面表示モードを進める（有効な画面のみを表示）
    display_mode = nextEnabledScreen(display_mode);
    
    // 部分リフレッシュでも完了を待機
    delay(100);
  }
  
  // CPUの過負荷を防ぐため、短い遅延を入れる
  delay(100);
}

// ============================================================================
// WiFi + NTP 関連関数
// ============================================================================

void setupNTP() {
  Serial.println("\n=== NTP時刻同期開始 ===");

  // WiFi未接続でも設定しておけば、接続された時点で自動的に同期される
  applyTimeSettings();

  if (!isWiFiConnected()) {
    Serial.println("⚠ WiFi未接続のため、接続後にバックグラウンドで同期します");
    return;
  }

  // NTP同期待機（最大15秒）
  Serial.print("NTP同期中");
  int attempts = 0;
  while (!isTimeSynced() && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  if (isTimeSynced()) {
    Serial.println("✓ NTP同期成功");
    Serial.printf("現在時刻: %s (%s)\n", getCurrentDateTime().c_str(), settings.timezone.c_str());
  } else {
    Serial.println("✗ NTP同期失敗（バックグラウンドで同期を継続）");
  }
}

// NTPサーバーとタイムゾーンを settings から反映する（設定ページで保存したときも呼ばれる）
void applyTimeSettings() {
  // SNTP はサーバー名の文字列を指すポインタをそのまま持ち続けるので、
  // settings.ntp_server（書き換えると領域が移る）ではなく固定の領域に写して渡す
  static char ntp_server[64];
  strlcpy(ntp_server, settings.ntp_server.c_str(), sizeof(ntp_server));
  configTzTime(settings.timezone.c_str(), ntp_server, NTP_SERVER_FALLBACK);
}

bool isTimeSynced() {
  return time(nullptr) > TIME_SYNCED_THRESHOLD;
}

String getCurrentDateTime() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// ENS160 の AQI-UBA は 1〜5（1=Excellent 〜 5=Unhealthy）。0 は未測定
void printAQIStatus(uint8_t aqi) {
  switch (aqi) {
    case 1:
      Serial.print("優秀");
      break;
    case 2:
      Serial.print("良好");
      break;
    case 3:
      Serial.print("中程度");
      break;
    case 4:
      Serial.print("悪い");
      break;
    case 5:
      Serial.print("非常に悪い");
      break;
    default:
      Serial.print("不明");
  }
}
