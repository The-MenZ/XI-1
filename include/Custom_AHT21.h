#ifndef CUSTOM_AHT21_H
#define CUSTOM_AHT21_H

#include <Arduino.h>
#include <Adafruit_AHTX0.h>

/**
 * @brief Data structure for AHT21 Temperature & Humidity Sensor
 *
 * AHT21 sensor readings: temperature and relative humidity
 * temperature / humidity は自己発熱補正後の値、raw_* はセンサーの生値
 */
struct AHT21Data {
    float temperature;       // °C（補正後）
    float humidity;          // %（補正後）
    float raw_temperature;   // °C（生値）
    float raw_humidity;      // %（生値）
};

class CustomAHT21 {
public:
    bool begin();
    AHT21Data read();

    // 筐体内の発熱で室温より高く測れてしまう分（°C）を設定する
    void setTemperatureOffset(float offset) { temperature_offset = offset; }

    // センサー自身が湿度を高く測ってしまう分（%、生値に対して）を設定する
    void setHumidityOffset(float offset) { humidity_offset = offset; }

private:
    Adafruit_AHTX0 aht;
    float temperature_offset = 0.0f;
    float humidity_offset = 0.0f;
};

#endif // CUSTOM_AHT21_H
