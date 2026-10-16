#ifndef CUSTOM_MAX9814_H
#define CUSTOM_MAX9814_H

#include <Arduino.h>
#include <cmath>
#include "adc_util.h"

// ============================================================================
// MAX9814マイクセンサー用カスタムドライバ（修正版）
// ============================================================================

struct MAX9814Data {
  int raw_value;           // 最新の生値
  float voltage;           // 最新の電圧
  float peak_to_peak;      // ピークツーピーク電圧（V）
  float rms_voltage;       // RMS電圧（200Hz のハイパスフィルタで 50Hz のハムを切った後）
  float db_level;          // デシベルレベル（dB: 中央値フィルタ済）
  float db_level_raw;      // デシベルレベル（dB: 生値）
  float db_above_floor;    // 補正後の騒音（db_level からマイク基板のノイズの高さを引いた dB。0 未満は 0）
  float sample_rate;       // サンプリング周波数（Hz。この read() で測った値）
};

class CustomMAX9814 {
private:
  const int ADC_PIN;
  const int SAMPLE_SIZE = 10000;     // サンプル数
  
  // ★巨大な adc_buffer は削除しました（メモリ節約）
  
  float db_history[5];       // 過去5つのdB値
  int history_index = 0;     
  bool history_filled = false;

  // 測れる下限（RMS 1mV）より小さいときの dB
  const float SILENT_DB = -80.0f;

  // 50Hz の電源のハム（と 100・150Hz の倍音）を切るハイパスフィルタ（2次バターワース）の境目
  // 実機の静かなときのノイズは約 30% が 50Hz とその倍音だった（SOUND_SCREEN.md の「実機で測ったノイズ」）
  // 50Hz は約 -24dB、100Hz は約 -12dB 小さくなる
  const float HUM_CUTOFF_HZ = 200.0f;
  // サンプリング周波数（Hz）。analogRead の速さで決まるので read() のたびに測り、次の read() のフィルタに使う
  float sample_rate = 9000.0f;

  // マイク基板のノイズの高さ（dB）。静かにしても指でマイクをふさいでも、この高さより下がらない
  // 騒音レベルからこの値を引いて、静かなときが 0 になるようにする（setNoiseFloor() で設定ページの値を入れる）
  float noise_floor_db = 0.0f;

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

  // 騒音レベルから引くマイク基板のノイズの高さ（dB）を設定する
  void setNoiseFloor(float db) {
    noise_floor_db = db;
  }
  
  // マイク信号からノイズレベルを計算
  MAX9814Data read() {
    MAX9814Data data = {0, 0, 0, 0, 0, 0, 0, 0};

    int min_val = 4095;
    int max_val = 0;

    // 合計値を格納する変数はオーバーフローを防ぐため double を推奨
    double sum_voltage = 0;       // 平均算出用（DC成分）
    double sum_filtered_sq = 0;   // フィルタ後の二乗和（DC と 50Hz のハムを除いたエネルギー）

    // ハイパスフィルタの係数（双一次変換。前回測ったサンプリング周波数を使う）
    const float SQRT2 = 1.41421356f;
    // k = tan(π × 境目 / サンプリング周波数)。角度が約 0.07 と小さいので tan(x) ≈ x + x³/3 で求める
    // （誤差は 1e-6 以下。tanf を使うとライブラリの分 Flash が 1.5KB ほど増える）
    float w = PI * HUM_CUTOFF_HZ / sample_rate;
    float k = w + w * w * w / 3.0f;
    float norm = 1.0f / (1.0f + SQRT2 * k + k * k);
    float a1 = 2.0f * (k * k - 1.0f) * norm;
    float a2 = (1.0f - SQRT2 * k + k * k) * norm;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;  // 1つ前・2つ前の入力と出力（V）

    // --- 1. サンプリングループ ---
    unsigned long start_us = micros();
    for (int i = 0; i < SAMPLE_SIZE; i++) {
      int val = analogRead(ADC_PIN);

      if (val < min_val) min_val = val;
      if (val > max_val) max_val = val;

      // 電圧に変換（eFuse 校正値込み）
      float v = adcRawToVolts(val);

      // 最初の値がずっと続いていたことにして、フィルタの立ち上がりの揺れ（DC バイアスの段差）を出さない
      if (i == 0) x1 = x2 = v;
      float y = norm * (v - 2.0f * x1 + x2) - a1 * y1 - a2 * y2;
      x2 = x1;
      x1 = v;
      y2 = y1;
      y1 = y;

      sum_voltage += v;
      sum_filtered_sq += y * y;

      // サンプリング間隔
      // ESP32のADCは高速なので、1秒測定したいならWaitが必要です
      delayMicroseconds(50);
    }
    sample_rate = SAMPLE_SIZE * 1e6f / (micros() - start_us);
    data.sample_rate = sample_rate;

    // --- 2. 統計計算 ---

    // 平均値（これが真のDCオフセットになります）
    double mean_voltage = sum_voltage / SAMPLE_SIZE;

    // RMS = フィルタ後の二乗平均の平方根（ハイパスフィルタが DC も切るので、平均を引かなくてよい）
    data.rms_voltage = (float)sqrt(sum_filtered_sq / SAMPLE_SIZE);
    
    // その他のデータを格納
    data.raw_value = (min_val + max_val) / 2;
    data.voltage = mean_voltage; // 平均電圧（DCバイアス）
    data.peak_to_peak = adcRawToVolts(max_val) - adcRawToVolts(min_val);

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
      data.db_level_raw = SILENT_DB;  // ノイズフロア（dB SPL）
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

    // --- 5. マイク基板のノイズの高さを引く（静かなときが 0 になる） ---
    data.db_above_floor = data.db_level > noise_floor_db ? data.db_level - noise_floor_db : 0.0f;

    return data;
  }
};

#endif