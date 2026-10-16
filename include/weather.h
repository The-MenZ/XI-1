#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <time.h>

// ============================================================================
// 天気予報（Open-Meteo）
// ============================================================================
// 設定ページで選んだ地域（緯度・経度）の天気を Open-Meteo（https://open-meteo.com/）から取る。
// API キーが不要で、HTTP（TLS なし）で取れるので TLS のライブラリを使わずに済む。
// WiFi 接続中に30分ごと（失敗したら5分後）と、地域を変えたときに取得する

// 1日分の予報
struct WeatherDay {
  char date[11];        // YYYY-MM-DD（その地域の日付）
  int8_t code;          // WMO 天気コード
  float temp_max;       // 最高気温（°C）
  float temp_min;       // 最低気温（°C）
  int8_t precip_prob;   // 降水確率（その日の最大値、%。-1 = 不明）
};

struct WeatherData {
  bool valid;           // 今の地域の天気を取得できていれば true
  float temp;           // 現在の気温（°C）
  int8_t code;          // 現在の WMO 天気コード
  bool is_day;          // 現在が昼か（夜の晴れは月で描く）
  WeatherDay days[2];   // [0] = 今日、[1] = 明日
  time_t fetched_at;    // 取得した時刻（時刻が未同期だったときは 0）
};

// loop() から毎回呼ぶ（取得する時期なら取得する。取得中の1秒ほどは他の処理が止まる）
void weatherLoop();

const WeatherData& getWeather();

#endif // WEATHER_H
