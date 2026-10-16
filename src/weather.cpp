#include "weather.h"
#include <WiFi.h>
#include "network.h"
#include "settings.h"

// main.cpp で定義
extern bool isTimeSynced();

#define WEATHER_HOST "api.open-meteo.com"
#define WEATHER_INTERVAL_MS (30 * 60 * 1000UL)  // 取得の間隔
#define WEATHER_RETRY_MS (5 * 60 * 1000UL)      // 取得に失敗したときの再試行の間隔
#define WEATHER_TIMEOUT_MS 8000                 // 接続から受信し終えるまでの上限
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

// json の "object":{...} の中から "key": の値の先頭を返す（値が配列なら index 番目）。
// Open-Meteo の応答は形が決まっていて入れ子も浅いので、JSON ライブラリは使わず文字列を探す
static const char* findValue(const char* json, const char* object, const char* key, int index) {
  char pattern[40];
  snprintf(pattern, sizeof(pattern), "\"%s\":{", object);
  const char* start = strstr(json, pattern);
  if (start == nullptr) {
    return nullptr;
  }
  const char* end = strchr(start, '}');
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);
  const char* p = strstr(start, pattern);
  if (end == nullptr || p == nullptr || p > end) {
    return nullptr;
  }
  p += strlen(pattern);
  if (*p == '[') {
    p++;
    for (int i = 0; i < index; i++) {
      p = strchr(p, ',');
      if (p == nullptr || p > end) {
        return nullptr;
      }
      p++;
    }
  }
  return p;
}

// 数値を読む（null などで読めなければ false）
// 応答の数値は「-12.3」の形だけなので自前で読む（strtof は指数表記なども扱うぶん約5KB大きい）
static bool readNumber(const char* json, const char* object, const char* key, int index, float* out) {
  const char* p = findValue(json, object, key, index);
  if (p == nullptr) {
    return false;
  }
  bool negative = *p == '-';
  if (negative) {
    p++;
  }
  if (!isdigit(*p)) {
    return false;
  }
  float value = 0;
  while (isdigit(*p)) {
    value = value * 10 + (*p++ - '0');
  }
  if (*p == '.') {
    p++;
    float scale = 0.1f;
    while (isdigit(*p)) {
      value += (*p++ - '0') * scale;
      scale *= 0.1f;
    }
  }
  *out = negative ? -value : value;
  return true;
}

// 応答の本文を data に読み込む（必要な値が欠けていたら false）
static bool parseWeather(const char* json, WeatherData* data) {
  float code, is_day;
  if (!readNumber(json, "current", "temperature_2m", 0, &data->temp) ||
      !readNumber(json, "current", "weather_code", 0, &code) ||
      !readNumber(json, "current", "is_day", 0, &is_day)) {
    return false;
  }
  data->code = (int8_t)code;
  data->is_day = is_day != 0;

  for (int i = 0; i < 2; i++) {
    WeatherDay& day = data->days[i];
    float precip;
    const char* date = findValue(json, "daily", "time", i);
    if (date == nullptr || *date != '"' ||
        !readNumber(json, "daily", "weather_code", i, &code) ||
        !readNumber(json, "daily", "temperature_2m_max", i, &day.temp_max) ||
        !readNumber(json, "daily", "temperature_2m_min", i, &day.temp_min)) {
      return false;
    }
    strlcpy(day.date, date + 1, sizeof(day.date));
    day.code = (int8_t)code;
    // 降水確率は地域によっては null（不明）になる
    day.precip_prob = readNumber(json, "daily", "precipitation_probability_max", i, &precip) ? (int8_t)precip : -1;
  }
  return true;
}

static bool fetchWeather(const String& lat, const String& lon) {
  Serial.printf("🌤  天気を取得中（%s, %s）...\n", lat.c_str(), lon.c_str());

  WiFiClient client;
  if (!client.connect(WEATHER_HOST, 80, WEATHER_TIMEOUT_MS)) {
    Serial.println("   ⚠️  天気: サーバーに接続できません");
    return false;
  }

  // HTTP/1.0 で頼むと、サーバーは送り終えたら切断する（chunked 形式にならない）
  // timezone=auto で「今日」「明日」をその地域の日付で区切る
  client.printf(
    "GET /v1/forecast?latitude=%s&longitude=%s"
    "&current=temperature_2m,weather_code,is_day"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
    "&timezone=auto&forecast_days=2 HTTP/1.0\r\n"
    "Host: " WEATHER_HOST "\r\n"
    "Connection: close\r\n\r\n",
    lat.c_str(), lon.c_str());

  String response;
  response.reserve(1200);
  unsigned long start = millis();
  while ((client.connected() || client.available()) && millis() - start < WEATHER_TIMEOUT_MS &&
         response.length() < WEATHER_RESPONSE_MAX) {
    if (client.available()) {
      response += (char)client.read();
    } else {
      delay(10);
    }
  }
  client.stop();

  // 1行目は "HTTP/1.0 200 OK"
  if (!response.startsWith("HTTP/1.") || response.substring(9, 12) != "200") {
    Serial.printf("   ⚠️  天気: 応答エラー（%s）\n", response.substring(0, response.indexOf('\r')).c_str());
    return false;
  }
  int body = response.indexOf("\r\n\r\n");
  WeatherData data = {};
  if (body < 0 || !parseWeather(response.c_str() + body + 4, &data)) {
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
