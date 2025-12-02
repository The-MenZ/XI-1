#ifndef CUSTOM_VEML7700_H
#define CUSTOM_VEML7700_H

#include <Arduino.h>
#include <Adafruit_VEML7700.h>

/**
 * @brief Data structure for VEML7700 Ambient Light Sensor
 *
 * VEML7700 sensor readings: illuminance and white channel
 */
struct VEML7700Data {
    float lux;           // Illuminance (lux)
    float white;         // White channel measurement
    uint16_t als;        // ALS raw data
};

class CustomVEML7700 {
public:
    bool begin();
    VEML7700Data read();
    void setGain(uint8_t gain);
    void setIntegrationTime(uint8_t it);

private:
    Adafruit_VEML7700 veml;
};

#endif // CUSTOM_VEML7700_H
