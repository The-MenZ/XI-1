#include "Custom_AHT20.h"

bool CustomAHT20::begin() {
    if (!aht20.begin()) {
        Serial.println("❌ AHT20 initialization failed");
        Serial.println("    - Incorrect I2C address");
        Serial.println("    - Sensor not connected");
        Serial.println("    - I2C communication error");
        return false;
    }

    Serial.println("✅ AHT20 initialized successfully");

    // Test read
    sensors_event_t humidity, temp;
    aht20.getEvent(&humidity, &temp);
    Serial.print("    Test: Temp=");
    Serial.print(temp.temperature);
    Serial.print("°C, Humidity=");
    Serial.print(humidity.relative_humidity);
    Serial.println("%");

    return true;
}

AHT20Data CustomAHT20::read() {
    AHT20Data data = {0.0f, 0.0f};
    sensors_event_t humidity, temp;
    aht20.getEvent(&humidity, &temp);

    data.temperature = temp.temperature;
    data.humidity = humidity.relative_humidity;

    return data;
}
