#ifndef ADC_UTIL_H
#define ADC_UTIL_H

#include <Arduino.h>
#include <esp_adc_cal.h>

// ============================================================================
// ADC 生値 → 電圧 変換（ESP32-C3）
// ============================================================================
// ESP32-C3 の ADC（11dB 減衰）は 4095 が 3.3V に対応せず、測れるのは約 2.5V まで。
// チップごとの誤差もあるため、eFuse の校正値を使って電圧に変換する。
// analogReadMilliVolts() と同じ校正だが、あちらは呼ぶたびに校正値を計算し直すので、
// 大量にサンプリングするセンサー向けに一度だけ計算して使い回す。
// ESP32-C3 の校正カーブは一次式なので、0 と 4095 の2点から傾きを求める。
inline float adcRawToVolts(float raw) {
  static bool initialized = false;
  static float offset_mv = 0.0f;
  static float mv_per_count = 0.0f;

  if (!initialized) {
    esp_adc_cal_characteristics_t chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &chars);
    offset_mv = esp_adc_cal_raw_to_voltage(0, &chars);
    mv_per_count = (esp_adc_cal_raw_to_voltage(4095, &chars) - offset_mv) / 4095.0f;
    initialized = true;
  }

  return (offset_mv + raw * mv_per_count) / 1000.0f;
}

#endif // ADC_UTIL_H
