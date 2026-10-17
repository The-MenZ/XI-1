#include "Custom_AHT21.h"

bool CustomAHT21::begin() {
    // I2C address 0x38 (fixed)
    if (!aht.begin(&Wire)) {
        Serial.println("❌ AHT21 initialization failed");
        return false;
    }

    Serial.println("✅ AHT21 initialized successfully");
    return true;
}

// 飽和水蒸気圧 (hPa) - Magnus 式
static float saturationVaporPressure(float temp_c) {
    return 6.112f * expf(17.62f * temp_c / (243.12f + temp_c));
}

AHT21Data CustomAHT21::read() {
    AHT21Data data = {0.0f, 0.0f, 0.0f, 0.0f};
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    data.raw_temperature = temp.temperature;
    data.raw_humidity = humidity.relative_humidity;

    // 温度はオフセット分を引く
    data.temperature = data.raw_temperature - temperature_offset;

    // 湿度はまずセンサー自身のずれを生値から引く（換算で約1.8倍に膨らむ前に直す）
    float sensor_humidity = data.raw_humidity - humidity_offset;
    if (sensor_humidity < 0.0f) sensor_humidity = 0.0f;

    // 相対湿度はセンサー自身の温度に対する値なので、温まっていると低く出る。
    // 空気中の水蒸気量は同じまま温度だけ下がったとして換算し直す
    data.humidity = sensor_humidity
                  * saturationVaporPressure(data.raw_temperature)
                  / saturationVaporPressure(data.temperature);
    if (data.humidity > 100.0f) data.humidity = 100.0f;

    return data;
}
