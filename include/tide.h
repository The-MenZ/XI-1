#ifndef TIDE_H
#define TIDE_H

#include <Arduino.h>
#include <time.h>

// ============================================================================
// 潮位と日の出・日の入り（Open-Meteo の Marine API）
// ============================================================================
// 設定ページで入れた釣り場（緯度・経度）の潮位を1時間ごとに、昨日から明日までの3日分取る。
// 同じリクエストで日の出・日の入りも取る。WiFi 接続中に3時間ごと（失敗したら5分後、
// 釣り場を変えたらすぐ）取得する。海から遠い地点では潮位が null になる（no_sea）

#define TIDE_HOURS 72        // 潮位の件数（昨日 0時から明日 23時まで）
#define TIDE_DAYS 3          // 日の出・日の入りの日数（昨日・今日・明日）
#define TIDE_MAX_EVENTS 12   // 満潮・干潮を覚えておく数（3日で多くても8回ほど）

// 満潮・干潮
struct TideEvent {
  time_t time;       // 時刻（1時間ごとの値を放物線でつないで求める）
  int16_t level_cm;  // 潮位（平均海面からの cm）
  bool high;         // 満潮なら true
};

struct TideData {
  bool valid;                      // 今の釣り場のデータを取得できていれば true（no_sea のときも true）
  bool no_sea;                     // 潮位が取れない（海から遠い）地点なら true
  time_t start;                    // level_cm[0] の時刻。以降は1時間ごと
  uint8_t count;                   // level_cm に入っている件数
  int16_t level_cm[TIDE_HOURS];    // 潮位（平均海面からの cm）
  time_t sunrise[TIDE_DAYS];       // 日の出（0 = 不明）
  time_t sunset[TIDE_DAYS];        // 日の入り（0 = 不明）
  TideEvent events[TIDE_MAX_EVENTS];
  uint8_t event_count;
  time_t fetched_at;               // 取得した時刻（時刻が未同期だったときは 0）
};

// loop() から毎回呼ぶ（取得する時期なら取得する。取得中の1秒ほどは他の処理が止まる）
void tideLoop();

const TideData& getTide();

#endif // TIDE_H
