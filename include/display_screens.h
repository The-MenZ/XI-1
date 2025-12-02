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

extern GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;

// 画面1: Air Quality (空気質)
void displayAirQuality(const ENS160Data& ens160_data, bool sensor_enabled);

// 画面2: Climate (温熱環境)
void displayClimate(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled
);

// 画面3: Ambience (視聴覚環境)
void displayAmbience(
  const VEML7700Data& veml7700_data, bool veml7700_enabled,
  const MAX9814Data& max9814_data, bool max9814_enabled
);

// 画面4: Gas Sensor (ガスセンサー)
void displayGasSensor(const MICS5524Data& mics5524_data, bool sensor_enabled);

// 画面5: 日時表示（NTP同期時刻）
void displayDateTime(bool isWiFiConnected, const String& dateTimeStr);

// 画面6: 月日表示（シンプル & 洒落たデザイン）
void displayMonthDay(int month, int day);

// 画面7: WiFi QRコード表示
void displayWiFiQR(const String& ssid, const String& password);

#endif // DISPLAY_SCREENS_H
