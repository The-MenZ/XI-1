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
};

class CustomMICS5524 {
private:
  const int ADC_PIN;
  const int HEATER_PIN;       // EN ピン（ヒーター制御）
  const float REF_VOLTAGE = 3.3;     // ESP32-C3の参照電圧
  const int ADC_RESOLUTION = 4095;   // 12ビットADC
  const int SAMPLE_SIZE = 100;       // サンプル数
  const float LOAD_RESISTANCE = 10000.0; // ロード抵抗 (10kΩ)
  
  // キャリブレーション値 (新鮮な空気中での基準値)
  float Ro = 10000.0;  // 新鮮な空気中のセンサー抵抗 (Ω)
  
  // CO検出の標準曲線係数
  const float CO_A = 8.3;      // log-log グラフの傾き
  const float CO_B = -0.913;   // 傾斜
  
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
  
  // キャリブレーション（新鮮な空気中で呼び出す）
  void calibrate();
  
  // センサー読み込み
  MICS5524Data read();
};

#endif

