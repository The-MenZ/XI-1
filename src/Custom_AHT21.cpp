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

AHT21Data CustomAHT21::read() {
    AHT21Data data = {0.0f, 0.0f};
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    data.temperature = temp.temperature;
    data.humidity = humidity.relative_humidity;

    return data;
}
