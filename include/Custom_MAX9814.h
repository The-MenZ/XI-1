#ifndef CUSTOM_MAX9814_H
#define CUSTOM_MAX9814_H

#include <Arduino.h>
#include <cmath>

// ============================================================================
// MAX9814マイクセンサー用カスタムドライバ（修正版）
// ============================================================================

struct MAX9814Data {
  int raw_value;           // 最新の生値
  float voltage;           // 最新の電圧
  float peak_to_peak;      // ピークツーピーク電圧（V）
  float rms_voltage;       // RMS電圧（AC成分のみ）
  float db_level;          // デシベルレベル（dB: 中央値フィルタ済）
  float db_level_raw;      // デシベルレベル（dB: 生値）
};

class CustomMAX9814 {
private:
  const int ADC_PIN;
  const float REF_VOLTAGE = 3.3;     // ESP32-C3の参照電圧
  const int ADC_RESOLUTION = 4095;   // 12ビットADC
  const int SAMPLE_SIZE = 10000;     // サンプル数
  
  // ★巨大な adc_buffer は削除しました（メモリ節約）
  
  float db_history[5];       // 過去5つのdB値
  int history_index = 0;     
  bool history_filled = false; 
  
  // 中央値を計算
  float calculateMedian(float *arr, int size) {
    float temp[5];
    // 配列をコピー（元の履歴を壊さないため）
    for (int i = 0; i < size; i++) temp[i] = arr[i];
    
    // バブルソート
    for (int i = 0; i < size - 1; i++) {
      for (int j = i + 1; j < size; j++) {
        if (temp[i] > temp[j]) {
          float t = temp[i];
          temp[i] = temp[j];
          temp[j] = t;
        }
      }
    }
    return temp[size / 2];
  }
  
public:
  CustomMAX9814(int pin) : ADC_PIN(pin) {
    // 初期値
    for (int i = 0; i < 5; i++) db_history[i] = -60.0;
  }
  
  void begin() {
    pinMode(ADC_PIN, INPUT);
  }
  
  // マイク信号からノイズレベルを計算
  MAX9814Data read() {
    MAX9814Data data = {0, 0, 0, 0, 0, 0};
    
    int min_val = 4095;
    int max_val = 0;
    
    // 合計値を格納する変数はオーバーフローを防ぐため double を推奨
    double sum_voltage = 0;       // 平均算出用（DC成分）
    double sum_voltage_sq = 0;    // 二乗和算出用（全エネルギー）
    
    // --- 1. サンプリングループ ---
    for (int i = 0; i < SAMPLE_SIZE; i++) {
      int val = analogRead(ADC_PIN);
      
      if (val < min_val) min_val = val;
      if (val > max_val) max_val = val;
      
      // 電圧に変換
      float v = (float)val / ADC_RESOLUTION * REF_VOLTAGE;
      
      sum_voltage += v;
      sum_voltage_sq += (v * v);
      
      // サンプリング間隔
      // ESP32のADCは高速なので、1秒測定したいならWaitが必要です
      delayMicroseconds(50); 
    }
    
    // --- 2. 統計計算 (標準偏差 = AC成分のRMS) ---
    
    // 平均値（これが真のDCオフセットになります）
    double mean_voltage = sum_voltage / SAMPLE_SIZE;
    
    // 二乗の平均
    double mean_sq = sum_voltage_sq / SAMPLE_SIZE;
    
    // 分散 = (二乗の平均) - (平均の二乗)
    // これにより、DC成分がきれいに相殺され、AC成分の分散だけが残ります
    double variance = mean_sq - (mean_voltage * mean_voltage);
    
    // 計算誤差でマイナスになるのを防ぐ
    if (variance < 0) variance = 0;
    
    // RMS (AC成分) = √分散
    data.rms_voltage = (float)sqrt(variance);
    
    // その他のデータを格納
    data.raw_value = (min_val + max_val) / 2;
    data.voltage = mean_voltage; // 平均電圧（DCバイアス）
    data.peak_to_peak = ((float)(max_val - min_val) / ADC_RESOLUTION) * REF_VOLTAGE;

    // --- 3. dB SPL（Sound Pressure Level）変換 ---
    // MAX9814スペック
    const float MIC_SENSITIVITY = 0.01413;  // V/Pa （-37dBV/Pa）
    const float GAIN = 100.0;               // 40dB増幅（VCC接続時）
    const float REFERENCE_PRESSURE = 20e-6; // 20μPa（0dB SPL基準）
    
    if (data.rms_voltage > 0.001) {  // ノイズフロア
      // 1. 増幅前のマイク出力に戻す
      float mic_output = data.rms_voltage / GAIN;
      
      // 2. 音圧を計算 [Pa]
      float sound_pressure = mic_output / MIC_SENSITIVITY;
      
      // 3. dB SPL を計算
      data.db_level_raw = 20.0 * log10(sound_pressure / REFERENCE_PRESSURE);
    } else {
      data.db_level_raw = -80.0;  // ノイズフロア（dB SPL）
    }
    
    // --- 4. 中央値フィルタ処理 ---
    db_history[history_index] = data.db_level_raw;
    history_index = (history_index + 1) % 5;
    
    if (!history_filled && history_index == 0) history_filled = true;
    
    if (history_filled) {
      data.db_level = calculateMedian(db_history, 5);
    } else {
      data.db_level = data.db_level_raw;
    }
    
    return data;
  }
};

#endif