#ifndef DISPLAY_SCREENS_H
#define DISPLAY_SCREENS_H

#include <GxEPD2_BW.h>
#include "Custom_ENS160.h"
#include "Custom_AHT21.h"
#include "Custom_BMP280.h"
#include "Custom_VEML7700.h"
#include "Custom_MAX9814.h"
#include "Custom_MICS5524.h"
#include "weather.h"

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

// 画面4: Time（パタパタ時計。黒い札2枚に時と分を24時間表示で描く）
// synced が false（NTP で時刻が合う前）なら、数字の代わりに「??」を出す
void displayTime(int hour, int minute, bool synced);

// 画面5: 月日表示（画面を4分割し、月日の4桁を大きな丸ゴシック風の数字で描く）
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

// 画面9: Weather（天気。今の天気と、今日・明日の予報）
// 地域が未設定、またはまだ取得できていなければその旨を出す
void displayWeather(const WeatherData& weather, bool has_location, bool wifi_connected);

// 画面10: Pressure（天気図風の H/L と等圧線、3時間の気圧の傾向）
// 海面気圧が 1013 hPa 以上なら H、未満なら L。1013 との間に 4hPa ごとに等圧線を引く
// trend_3h: 約3時間前からの気圧の変化（hPa）。履歴が3時間分たまるまでは NAN（傾向のバッジを出さない）
void displayPressure(const BMP280Data& bmp280_data, bool bmp280_enabled, float trend_3h);

// 画面11: Sound（レトロな VU メーター）
// 針は補正後の騒音（マイク基板のノイズの高さを引いた dB、+0〜+40）。+30dB から右は黒い帯（うるさい）
void displaySound(const MAX9814Data& max9814_data, bool sensor_enabled);

// 画面12: Light（猫の目）
// 明るいと瞳が縦に細く、暗いとまんまる。10,000 lx から上は目を細め、10 lx 未満は画面を黒くする。下に照度の数字
void displayLight(const VEML7700Data& veml7700_data, bool sensor_enabled);

#endif // DISPLAY_SCREENS_H
