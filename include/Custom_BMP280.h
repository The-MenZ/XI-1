#ifndef CUSTOM_BMP280_H
#define CUSTOM_BMP280_H

#include <Arduino.h>
#include <Adafruit_BMP280.h>

/**
 * @brief Data structure for BMP280 Pressure & Temperature Sensor
 *
 * BMP280 sensor readings: temperature, pressure, and calculated altitude
 */
struct BMP280Data {
    float temperature;   // °C
    float pressure;      // hPa（設置場所の気圧）
    float sea_level_pressure;  // hPa（設置場所の標高で海面の高さに換算した気圧。天気図の気圧と比べられる）
    float altitude;      // m (above sea level)
};

class CustomBMP280 {
public:
    bool begin();
    BMP280Data read();
    void setSeaLevelPressure(float pressure);
    // 設置場所の標高（m）。sea_level_pressure の換算に使う
    void setStationAltitude(float altitude_m);

private:
    Adafruit_BMP280 bmp;
    float sea_level_pressure = 1013.25f;  // Standard atmospheric pressure (hPa)
    float station_altitude = 0.0f;
};

#endif // CUSTOM_BMP280_H
