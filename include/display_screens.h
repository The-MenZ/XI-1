#ifndef DISPLAY_SCREENS_H
#define DISPLAY_SCREENS_H

#include <GxEPD2_BW.h>
#include "Custom_ENS160.h"
#include "Custom_AHT21.h"
#include "Custom_BMP280.h"
#include "Custom_VEML7700.h"
#include "Custom_MAX9814.h"
#include "Custom_MICS5524.h"

// ============================================================================
// 画面表示関数群
// ============================================================================
// これらの関数はmain.cppから呼び出され、各画面の表示を担当します
// センサーデータを明示的にパラメータとして受け取ります
// 画面番号は main.cpp の SCREEN_ENABLED / display_mode の添字（0始まり）と同じ

extern GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;

// Climate Modern 画面の温湿度履歴（リングバッファ）の件数
// 24時間 ÷ 10分 = 144データポイント。main.cpp の配列と表示側の添字計算で共用する
#define HISTORY_SIZE 144

// 画面0: Air Quality (空気質)
void displayAirQuality(const ENS160Data& ens160_data, bool sensor_enabled);

// 画面1: Climate (温熱環境)
void displayClimate(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled
);

// 画面2: Ambience (視聴覚環境)
void displayAmbience(
  const VEML7700Data& veml7700_data, bool veml7700_enabled,
  const MAX9814Data& max9814_data, bool max9814_enabled
);

// 画面3: Gas Sensor (ガスセンサー)
void displayGasSensor(const MICS5524Data& mics5524_data, bool sensor_enabled);

// 画面4: 日時表示（NTP同期時刻）
void displayDateTime(bool isWiFiConnected, const String& dateTimeStr);

// 画面5: 月日表示（シンプル & 洒落たデザイン）
void displayMonthDay(int month, int day);

// 画面6: WiFi QRコード表示（中央に WiFi マーク。パスワードが空なら暗号化なしの WiFi として出す）
void displayWiFiQR(const String& ssid, const String& password);

// 画面7: Climate Modern (温度/湿度モダンデザイン)
void displayClimateModern(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled,
  const float* temp_history, const float* humid_history,
  int history_count, int history_index
);

// 画面8: 設定ページ QRコード表示（中央に歯車。url が空なら未接続の表示）
void displaySettingsQR(const String& url);

#endif // DISPLAY_SCREENS_H
