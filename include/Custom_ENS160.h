#ifndef CUSTOM_ENS160_H
#define CUSTOM_ENS160_H

#include <Arduino.h>
#include "ScioSense_ENS160.h"

#define ENS160_I2C_ADDRESS 0x53

// DATA_STATUS レジスタの有効フラグ（データシートの VALIDITY FLAG）
#define ENS160_VALIDITY_NORMAL  0  // 通常
#define ENS160_VALIDITY_WARMUP  1  // ウォームアップ中（電源を入れてから約3分）
#define ENS160_VALIDITY_STARTUP 2  // 初回起動中（新品を初めて使ってから約1時間）
#define ENS160_VALIDITY_INVALID 3  // 値が無効

/**
 * @brief Data structure for ENS160 Air Quality Sensor
 *
 * ENS160 sensor readings: CO2 equivalent, TVOC, and Air Quality Index
 */
struct ENS160Data {
    float eco2;          // ppm (eCO2)
    float tvoc;          // ppb (TVOC)
    uint8_t aqi;         // 1-5 (Air Quality Index)
    uint8_t validity;        // 値の有効フラグ（ENS160_VALIDITY_*）
    float warmup_progress;   // ウォームアップ（初回起動）の進み具合（0〜1。1 で完了）
};

class CustomENS160 {
public:
    CustomENS160() : ens(ENS160_I2C_ADDRESS) {}
    bool begin();
    ENS160Data read();
    bool available() { return ens.available(); }

private:
    // 進み具合の目安にする時間（センサーは残り時間を教えてくれないので、電源を入れてからの時間で出す）
    const unsigned long WARMUP_MS = 3UL * 60 * 1000;    // ウォームアップ: 3分
    const unsigned long STARTUP_MS = 60UL * 60 * 1000;  // 初回起動: 1時間

    ScioSense_ENS160 ens;
    unsigned long start_time = 0;  // begin() した時刻（begin() でセンサーをリセットするので、ここからウォームアップが始まる）

    uint8_t readValidity();
};

#endif // CUSTOM_ENS160_H
