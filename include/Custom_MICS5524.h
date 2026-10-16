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

  // キャリブレーション値 (新鮮な空気中での基準値)
  float Ro = 10000.0;  // 新鮮な空気中のセンサー抵抗 (Ω)
  bool calibrated = false;
  unsigned long heater_on_time = 0;

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
  void calibrate();

  // センサー読み込み
  MICS5524Data read();
};

#endif
