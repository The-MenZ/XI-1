#ifndef CUSTOM_ENS160_H
#define CUSTOM_ENS160_H

#include <Arduino.h>
#include "ScioSense_ENS160.h"

#define ENS160_I2C_ADDRESS 0x53

/**
 * @brief Data structure for ENS160 Air Quality Sensor
 *
 * ENS160 sensor readings: CO2 equivalent, TVOC, and Air Quality Index
 */
struct ENS160Data {
    float eco2;          // ppm (eCO2)
    float tvoc;          // ppb (TVOC)
    uint8_t aqi;         // 1-5 (Air Quality Index)
};

class CustomENS160 {
public:
    CustomENS160() : ens(ENS160_I2C_ADDRESS) {}
    bool begin();
    ENS160Data read();
    bool available() { return ens.available(); }

private:
    ScioSense_ENS160 ens;
};

#endif // CUSTOM_ENS160_H
