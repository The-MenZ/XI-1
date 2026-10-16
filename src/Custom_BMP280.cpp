#include "Custom_BMP280.h"

bool CustomBMP280::begin() {
    // Try I2C address 0x77 (GND connected)
    if (!bmp.begin(0x77)) {
        // Try default address 0x76
        if (!bmp.begin(0x76)) {
            Serial.println("❌ BMP280 initialization failed");
            return false;
        }
    }

    // Standard sampling configuration
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);

    Serial.println("✅ BMP280 initialized successfully");
    return true;
}

BMP280Data CustomBMP280::read() {
    BMP280Data data = {0.0f, 0.0f, 0.0f, 0.0f};

    data.temperature = bmp.readTemperature();
    data.pressure = bmp.readPressure() / 100.0f;  // Pa → hPa
    data.sea_level_pressure = bmp.seaLevelForAltitude(station_altitude, data.pressure);
    data.altitude = bmp.readAltitude(sea_level_pressure);

    return data;
}

void CustomBMP280::setSeaLevelPressure(float pressure) {
    sea_level_pressure = pressure;
    Serial.print("📊 BMP280 sea level pressure set to ");
    Serial.print(pressure);
    Serial.println(" hPa");
}

void CustomBMP280::setStationAltitude(float altitude_m) {
    station_altitude = altitude_m;
}
