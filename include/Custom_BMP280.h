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
    float pressure;      // hPa
    float altitude;      // m (above sea level)
};

class CustomBMP280 {
public:
    bool begin();
    BMP280Data read();
    void setSeaLevelPressure(float pressure);

private:
    Adafruit_BMP280 bmp;
    float sea_level_pressure = 1013.25f;  // Standard atmospheric pressure (hPa)
};

#endif // CUSTOM_BMP280_H
