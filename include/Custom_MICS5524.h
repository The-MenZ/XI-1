#ifndef CUSTOM_MICS5524_H
#define CUSTOM_MICS5524_H

#include <Arduino.h>
#include <cmath>

/**
 * @brief Data structure for MICS5524 Gas Sensor
 *
 * Detects: CO, NH3, C2H5OH, H2, CH4, and other combustible gases
 */
struct MICS5524Data {
    int raw_value;           // Raw ADC value (0-4095)
    float voltage;           // Analog output voltage (V)
    float resistance;        // Sensor resistance (Ω)
    float rs_ro_ratio;       // Rs/Ro ratio (gas detection indicator)
    float ppm_equivalent;    // ppm equivalent (CO reference)
    bool heater_enabled;     // Heater status
    bool calibrated;         // Ro calibrated (false while warming up)
    float ro;                // 今の基準の抵抗 Ro（Ω。キャリブレーション前は 0）
    float rs_ro_min;         // 測れる Rs/Ro の下限（出力が ADC の上限電圧のとき。キャリブレーション前は 0）
    float warmup_progress;   // ヒーターのウォームアップの進み具合（0〜1。1 で完了）
};

class CustomMICS5524 {
private:
  const int ADC_PIN;
  const int HEATER_PIN;       // EN ピン（ヒーター制御）
  // 検出回路（センサー + ロード抵抗）の電源電圧。Adafruit MiCS5524 基板は 5V 給電
  // 注意: 5V 給電ではガス濃度が非常に高いと出力が 3.3V を超えうる（ESP32 の入力上限）
  const float CIRCUIT_VOLTAGE = 5.0;
  const int SAMPLE_SIZE = 100;       // サンプル数
  const float LOAD_RESISTANCE = 10000.0; // ロード抵抗 (10kΩ)

  // ヒーターが温まって値が安定するまでの時間
  const unsigned long WARMUP_MS = 3UL * 60 * 1000;  // 3分

  // 基準の抵抗 Ro（Ω）。Rs/Ro が 1 = ガス画面の HP 100
  // キャリブレーションでその時の Rs にし、その後は直近 RO_DAYS 日でいちばんきれいだったとき
  // （Rs がいちばん大きかったとき）の Rs にする。汚れた空気の中で起動しても、きれいな空気が来れば直る
  float Ro = 10000.0;
  bool calibrated = false;
  unsigned long heater_on_time = 0;

  // 1日ごとのいちばん大きい Rs を RO_DAYS 日分覚えておき、その最大を Ro にする。古い日は1日ごとに捨てる
  // （何日も汚れたままでも約1週間は汚いと出て、それより長いずれはセンサーのずれとして吸収する）
  // 一番新しい分は途中の「今日」なので、8日分あれば必ず丸7日以上を見られる。
  // RAM は float 4個（16B）単位で取られるので、7日分にしても使う量は同じ
  static const int RO_DAYS = 8;
  const unsigned long DAY_MS = 24UL * 60 * 60 * 1000;
  float daily_max_rs[RO_DAYS] = {};  // Ω。0 はまだ測っていない日
  int day_index = 0;                 // 今日の分の添字
  unsigned long day_start = 0;       // 今日の分を数え始めた時刻（millis）

  // 基準（キャリブレーションと日ごとの記録）に入れる出力電圧の下限（V）。これより低い出力（Rs 約 490kΩ 以上）は
  // コネクタの抜けかけなどとみなして入れない。入れると Ro が跳ね上がったまま8日残り、元に戻っても HP がほぼ 0 になる。
  // この個体のきれいな空気は約 0.44V（Ro ≈ 104kΩ）。Ro がもっと大きい個体では、この値を下げる
  const float MIN_BASELINE_VOLTAGE = 0.1f;

  // 今の Rs を今日の分に入れ（1日たっていたら次の日へ進める）、Ro を直近 RO_DAYS 日の最大にする
  void updateBaseline(float rs);

  // CO 換算の近似式（DFRobot_MICS ライブラリと同じ）
  // ppm = (0.425 - Rs/Ro) / 0.000405 （Rs/Ro が下がるほど CO が多い、1〜1000ppm）
  const float CO_RATIO_THRESHOLD = 0.425;
  const float CO_RATIO_PER_PPM = 0.000405;

  // フィルタ用バッファ
  float voltage_history[5];
  int history_index = 0;
  bool history_filled = false;

  // 中央値を計算
  float calculateMedian(float *arr, int size);

public:
  CustomMICS5524(int adc_pin, int heater_pin = -1);

  void begin();

  // ヒーターの有効/無効制御
  void enableHeater(bool enable);

  // ヒーターがウォームアップ時間を過ぎたか
  bool isWarmedUp() const;
  bool isCalibrated() const { return calibrated; }

  // キャリブレーション（ウォームアップ後、新鮮な空気中で呼び出す）
  // 今の Rs を Ro にし、それまでの日ごとの記録は捨てる
  void calibrate();

  // センサー読み込み
  MICS5524Data read();
};

#endif
