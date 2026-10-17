#include "weather.h"
#include "network.h"
#include "open_meteo.h"
#include "settings.h"

// main.cpp で定義
extern bool isTimeSynced();

#define WEATHER_HOST "api.open-meteo.com"
#define WEATHER_INTERVAL_MS (30 * 60 * 1000UL)  // 取得の間隔
#define WEATHER_RETRY_MS (5 * 60 * 1000UL)      // 取得に失敗したときの再試行の間隔
#define WEATHER_RESPONSE_MAX 2048               // 受け取る応答の上限（通常は約1KB）

static WeatherData weather = {};

// 最後に取得を試した地域と時刻、その結果
static String requested_lat;
static String requested_lon;
static unsigned long last_attempt = 0;
static bool last_ok = false;

const WeatherData& getWeather() {
  return weather;
}

// 応答の本文を data に読み込む（必要な値が欠けていたら false）
static bool parseWeather(const char* json, WeatherData* data) {
  float code, is_day;
  if (!openMeteoReadNumber(json, "current", "temperature_2m", 0, &data->temp) ||
      !openMeteoReadNumber(json, "current", "weather_code", 0, &code) ||
      !openMeteoReadNumber(json, "current", "is_day", 0, &is_day)) {
    return false;
  }
  data->code = (int8_t)code;
  data->is_day = is_day != 0;

  for (int i = 0; i < 2; i++) {
    WeatherDay& day = data->days[i];
    float precip;
    const char* date = openMeteoFindValue(json, "daily", "time", i);
    if (date == nullptr || *date != '"' ||
        !openMeteoReadNumber(json, "daily", "weather_code", i, &code) ||
        !openMeteoReadNumber(json, "daily", "temperature_2m_max", i, &day.temp_max) ||
        !openMeteoReadNumber(json, "daily", "temperature_2m_min", i, &day.temp_min)) {
      return false;
    }
    strlcpy(day.date, date + 1, sizeof(day.date));
    day.code = (int8_t)code;
    // 降水確率は地域によっては null（不明）になる
    day.precip_prob = openMeteoReadNumber(json, "daily", "precipitation_probability_max", i, &precip) ? (int8_t)precip : -1;
  }
  return true;
}

static bool fetchWeather(const String& lat, const String& lon) {
  Serial.printf("🌤  天気を取得中（%s, %s）...\n", lat.c_str(), lon.c_str());

  // timezone=auto で「今日」「明日」をその地域の日付で区切る
  String path = "/v1/forecast?latitude=" + lat + "&longitude=" + lon +
                "&current=temperature_2m,weather_code,is_day"
                "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
                "&timezone=auto&forecast_days=2";
  String body;
  if (!openMeteoGet(WEATHER_HOST, path, WEATHER_RESPONSE_MAX, "天気", body)) {
    return false;
  }
  WeatherData data = {};
  if (!parseWeather(body.c_str(), &data)) {
    Serial.println("   ⚠️  天気: 応答を読み取れません");
    return false;
  }

  data.valid = true;
  data.fetched_at = isTimeSynced() ? time(nullptr) : 0;
  weather = data;
  Serial.printf("   ✅ 天気: 現在 %.1f°C コード%d / 今日 %s %.0f/%.0f°C 降水%d%% / 明日 %s %.0f/%.0f°C 降水%d%%\n",
                weather.temp, weather.code,
                weather.days[0].date, weather.days[0].temp_max, weather.days[0].temp_min, weather.days[0].precip_prob,
                weather.days[1].date, weather.days[1].temp_max, weather.days[1].temp_min, weather.days[1].precip_prob);
  return true;
}

void weatherLoop() {
  if (!hasWeatherLocation()) {
    weather.valid = false;
    requested_lat = requested_lon = "";
    return;
  }

  // 地域が変わったら、前の地域の天気は出さずにすぐ取り直す
  bool location_changed = settings.weather_lat != requested_lat || settings.weather_lon != requested_lon;
  if (location_changed) {
    weather.valid = false;
  } else if (millis() - last_attempt < (last_ok ? WEATHER_INTERVAL_MS : WEATHER_RETRY_MS)) {
    return;
  }

  if (!isWiFiConnected() || isAPMode()) {
    return;
  }

  requested_lat = settings.weather_lat;
  requested_lon = settings.weather_lon;
  last_attempt = millis();
  last_ok = fetchWeather(requested_lat, requested_lon);
}
