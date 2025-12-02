#ifndef Custom_AHT20
#define Custom_AHT20

#include <Arduino.h>
#include <Adafruit_AHTX0.h>

/**
 * @brief struct for data from AHT20 sensor
 * 
 * AHT20 is included in AHT20+BMP280 module
 */
struct AHT20Data {
    float temperature;   // °C
    float humidity;      // %
};

class CustomAHT20 {
public:
    bool begin();
    AHT20Data read();

private:
    Adafruit_AHTX0 aht20;
};

#endif

