#include "tide.h"
#include "network.h"
#include "open_meteo.h"
#include "settings.h"

// main.cpp で定義
extern bool isTimeSynced();

#define TIDE_HOST "marine-api.open-meteo.com"
#define TIDE_INTERVAL_MS (3 * 60 * 60 * 1000UL)  // 取得の間隔（潮は前もって決まっているので長め）
#define TIDE_RETRY_MS (5 * 60 * 1000UL)          // 取得に失敗したときの再試行の間隔
#define TIDE_RESPONSE_MAX 3072                   // 受け取る応答の上限（通常は約2KB）

// 隣り合う満潮と干潮の差がこれより小さければ、どちらも満潮・干潮として扱わない（cm）。
// 1時間ごとの値には数cm の小さな山と谷が出ることがあり、そのままだと満潮・干潮の時刻がずれて出る
#define TIDE_MIN_RANGE_CM 5

static TideData tide = {};

// 最後に取得を試した釣り場と時刻、その結果
static String requested_lat;
static String requested_lon;
static unsigned long last_attempt = 0;
static bool last_ok = false;

const TideData& getTide() {
  return tide;
}

// 1時間ごとの潮位から満潮・干潮を探す。山（谷）の前後3点を通る放物線の頂点を、その時刻と潮位にする
static void findTideEvents(TideData* data) {
  data->event_count = 0;
  for (int i = 1; i + 1 < data->count && data->event_count < TIDE_MAX_EVENTS; i++) {
    int a = data->level_cm[i - 1], b = data->level_cm[i], c = data->level_cm[i + 1];
    // 同じ値が2つ続く山（谷）は、後ろの点で見つける
    bool high = b >= a && b > c;
    bool low = b <= a && b < c;
    if (!high && !low) {
      continue;
    }
    float den = a - 2 * b + c;
    float offset = den != 0 ? 0.5f * (a - c) / den : 0;
    TideEvent& event = data->events[data->event_count++];
    event.time = data->start + (time_t)lroundf((i + offset) * 3600);
    event.level_cm = (int16_t)lroundf(b - 0.25f * (a - c) * offset);
    event.high = high;
  }

  // 差が小さい満潮と干潮の組を、なくなるまで取り除く
  for (int k = 0; k + 1 < data->event_count;) {
    if (abs(data->events[k].level_cm - data->events[k + 1].level_cm) < TIDE_MIN_RANGE_CM) {
      memmove(&data->events[k], &data->events[k + 2], (data->event_count - k - 2) * sizeof(TideEvent));
      data->event_count -= 2;
      k = k > 0 ? k - 1 : 0;  // 取り除いたことで、1つ前の組の差も変わる
    } else {
      k++;
    }
  }
}

// 応答の本文を data に読み込む（必要な値が欠けていたら false）
static bool parseTide(const char* json, TideData* data) {
  if (!openMeteoReadTime(json, "hourly", "time", 0, &data->start)) {
    return false;
  }
  // 潮位は先頭から続いている分だけ使う。先頭から null なら海から遠い地点
  data->count = 0;
  for (int i = 0; i < TIDE_HOURS; i++) {
    float level;
    if (!openMeteoReadNumber(json, "hourly", "sea_level_height_msl", i, &level)) {
      break;
    }
    data->level_cm[data->count++] = (int16_t)lroundf(level * 100);
  }
  data->no_sea = data->count < 3;
  for (int i = 0; i < TIDE_DAYS; i++) {
    if (!openMeteoReadTime(json, "daily", "sunrise", i, &data->sunrise[i])) {
      data->sunrise[i] = 0;
    }
    if (!openMeteoReadTime(json, "daily", "sunset", i, &data->sunset[i])) {
      data->sunset[i] = 0;
    }
  }
  findTideEvents(data);
  return true;
}

static bool fetchTide(const String& lat, const String& lon) {
  Serial.printf("🌊 潮位を取得中（%s, %s）...\n", lat.c_str(), lon.c_str());

  // 時刻は unixtime（本体の時計とそのまま比べられる）。timezone=auto で日の区切りをその地点の日付にする
  String path = "/v1/marine?latitude=" + lat + "&longitude=" + lon +
                "&hourly=sea_level_height_msl&daily=sunrise,sunset"
                "&timezone=auto&timeformat=unixtime&past_days=1&forecast_days=2";
  String body;
  if (!openMeteoGet(TIDE_HOST, path, TIDE_RESPONSE_MAX, "潮位", body)) {
    return false;
  }
  TideData data = {};
  if (!parseTide(body.c_str(), &data)) {
    Serial.println("   ⚠️  潮位: 応答を読み取れません");
    return false;
  }

  data.valid = true;
  data.fetched_at = isTimeSynced() ? time(nullptr) : 0;
  tide = data;
  if (tide.no_sea) {
    Serial.println("   ⚠️  潮位: この地点では潮位が取れません（海から遠い）");
    return true;
  }
  Serial.printf("   ✅ 潮位: %d件、満潮・干潮 %d回\n", tide.count, tide.event_count);
  for (int i = 0; i < tide.event_count; i++) {
    struct tm t;
    localtime_r(&tide.events[i].time, &t);
    Serial.printf("      %s %d/%d %d:%02d %dcm\n", tide.events[i].high ? "満潮" : "干潮",
                  t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, tide.events[i].level_cm);
  }
  return true;
}

void tideLoop() {
  if (!hasTideLocation()) {
    tide.valid = false;
    requested_lat = requested_lon = "";
    return;
  }

  // 釣り場が変わったら、前の釣り場の潮は出さずにすぐ取り直す
  bool location_changed = settings.tide_lat != requested_lat || settings.tide_lon != requested_lon;
  if (location_changed) {
    tide.valid = false;
  } else if (millis() - last_attempt < (last_ok ? TIDE_INTERVAL_MS : TIDE_RETRY_MS)) {
    return;
  }

  if (!isWiFiConnected() || isAPMode()) {
    return;
  }

  requested_lat = settings.tide_lat;
  requested_lon = settings.tide_lon;
  last_attempt = millis();
  last_ok = fetchTide(requested_lat, requested_lon);
}
