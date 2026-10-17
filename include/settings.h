#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// ============================================================================
// 設定（NVS に保存）
// ============================================================================
// Web の設定ページから変更し、ESP32 の NVS（フラッシュ上のキー・値ストア）に保存する。
// 電源を切っても、ファームウェアを書き直しても残る。
// NVS に値がない項目は既定値を使う（WiFi は wifi_config.h があればその値）

struct Settings {
  String wifi_ssid;
  String wifi_password;
  uint16_t screen_mask;   // bit i = 画面 i を表示する
  uint16_t interval_sec;  // 画面の切り替え間隔（秒）
  String ntp_server;
  String timezone;        // POSIX 形式のタイムゾーン（例: JST-9）
  String weather_place;   // 天気の地域名（設定ページに出すだけ。画面には出さない）
  String weather_lat;     // 天気の地域の緯度・経度（10進数の文字列のまま API に渡す。未設定なら空）
  String weather_lon;
  int16_t altitude_m;     // 設置場所の標高（m）。気圧を海面の高さに換算するのに使う
  int16_t mic_floor_db;   // 静かなときの騒音レベル（dB）。マイク基板のノイズの高さ。Sound 画面ではこれが 30dB になる
  String tide_lat;        // 釣り場の緯度・経度（潮位と日の出・日の入りを取る地点。10進数の文字列のまま API に渡す。未設定なら空）
  String tide_lon;
};

extern Settings settings;

// 天気の地域が設定されているか
inline bool hasWeatherLocation() {
  return settings.weather_lat.length() > 0 && settings.weather_lon.length() > 0;
}

// 釣り場が設定されているか
inline bool hasTideLocation() {
  return settings.tide_lat.length() > 0 && settings.tide_lon.length() > 0;
}

// 画面切り替え間隔の範囲（秒）
#define INTERVAL_SEC_MIN 5
#define INTERVAL_SEC_MAX 3600

// 設置場所の標高の範囲（m）
#define ALTITUDE_M_MIN -500
#define ALTITUDE_M_MAX 4000

// 静かなときの騒音レベルの範囲（dB）
#define MIC_FLOOR_DB_MIN 0
#define MIC_FLOOR_DB_MAX 100

// NVS から読み込む（画面の既定値と画面数は、画面定義を持つ main.cpp から渡す）
// 保存したときより画面が増えていたら、増えた画面は既定値を使う
void loadSettings(uint16_t default_screen_mask, uint8_t num_screens);

// 現在の settings を NVS に保存する
void saveSettings();

// NVS の設定をすべて消す（次回起動時に既定値へ戻る）
void resetSettings();

#endif // SETTINGS_H
