#ifndef CUSTOM_AHT21_H
#define CUSTOM_AHT21_H

#include <Arduino.h>
#include <Adafruit_AHTX0.h>

/**
 * @brief Data structure for AHT21 Temperature & Humidity Sensor
 *
 * AHT21 sensor readings: temperature and relative humidity
 */
struct AHT21Data {
    float temperature;   // °C
    float humidity;      // %
};

class CustomAHT21 {
public:
    bool begin();
    AHT21Data read();

private:
    Adafruit_AHTX0 aht;
};

#endif // CUSTOM_AHT21_H
