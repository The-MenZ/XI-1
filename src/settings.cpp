#include "settings.h"
#include <Preferences.h>

// wifi_config.h は任意。あれば NVS に WiFi 設定がないときの既定値になる
#if __has_include("wifi_config.h")
#include "wifi_config.h"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#define DEFAULT_INTERVAL_SEC 10
#define DEFAULT_NTP_SERVER "pool.ntp.org"
#define DEFAULT_TIMEZONE "JST-9"
#define DEFAULT_ALTITUDE_M 0  // 0 m なら海面気圧の換算をしないのと同じ
#define DEFAULT_MIC_FLOOR_DB 55  // 実機で静かにしたときの騒音レベル（54.4〜55.1dB）。静かなときに針が 30dB で止まるよう切り上げた値

// NVS の名前空間（キーは15文字以内）
static const char* NVS_NAMESPACE = "xi1";

Settings settings;

// 画面数（screen_mask と一緒に保存し、次に読み込むときに増えた画面を見分ける）
static uint8_t screen_count = 0;

void loadSettings(uint16_t default_screen_mask, uint8_t num_screens) {
  screen_count = num_screens;

  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true);  // 読み込み専用
  settings.wifi_ssid = prefs.getString("ssid", WIFI_SSID);
  settings.wifi_password = prefs.getString("pass", WIFI_PASSWORD);

  // 保存したときの画面数より後ろの画面（ファームウェア更新で増えた画面）は既定値を使う
  uint8_t saved_count = prefs.getUChar("screens_n", 0);
  uint16_t saved_part = (1 << saved_count) - 1;
  settings.screen_mask = (prefs.getUShort("screens", 0) & saved_part) | (default_screen_mask & ~saved_part);

  settings.interval_sec = prefs.getUShort("interval", DEFAULT_INTERVAL_SEC);
  settings.ntp_server = prefs.getString("ntp", DEFAULT_NTP_SERVER);
  settings.timezone = prefs.getString("tz", DEFAULT_TIMEZONE);
  settings.weather_place = prefs.getString("w_place", "");
  settings.weather_lat = prefs.getString("w_lat", "");
  settings.weather_lon = prefs.getString("w_lon", "");
  settings.altitude_m = prefs.getShort("alt", DEFAULT_ALTITUDE_M);
  settings.mic_floor_db = prefs.getShort("mic_floor", DEFAULT_MIC_FLOOR_DB);
  prefs.end();

  settings.interval_sec = constrain(settings.interval_sec, INTERVAL_SEC_MIN, INTERVAL_SEC_MAX);
  settings.altitude_m = constrain(settings.altitude_m, ALTITUDE_M_MIN, ALTITUDE_M_MAX);
  settings.mic_floor_db = constrain(settings.mic_floor_db, MIC_FLOOR_DB_MIN, MIC_FLOOR_DB_MAX);
}

void saveSettings() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString("ssid", settings.wifi_ssid);
  prefs.putString("pass", settings.wifi_password);
  prefs.putUShort("screens", settings.screen_mask);
  prefs.putUChar("screens_n", screen_count);
  prefs.putUShort("interval", settings.interval_sec);
  prefs.putString("ntp", settings.ntp_server);
  prefs.putString("tz", settings.timezone);
  prefs.putString("w_place", settings.weather_place);
  prefs.putString("w_lat", settings.weather_lat);
  prefs.putString("w_lon", settings.weather_lon);
  prefs.putShort("alt", settings.altitude_m);
  prefs.putShort("mic_floor", settings.mic_floor_db);
  prefs.end();
}

void resetSettings() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}
