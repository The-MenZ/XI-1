#include "Custom_VEML7700.h"

bool CustomVEML7700::begin() {
    // I2C address 0x10 (fixed)
    if (!veml.begin(&Wire)) {
        Serial.println("❌ VEML7700 initialization failed");
        return false;
    }

    // Gain: 1/8 (optimized for low light, avoids saturation)
    // Supported gains: 0=1x, 1=2x, 2=1/4x, 3=1/8x
    veml.setGain(3);  // 1/8 gain

    // Integration time: 100ms (balanced setting)
    // Supported times: 0=25ms, 1=50ms, 2=100ms, 3=200ms, 4=400ms, 5=800ms
    veml.setIntegrationTime(2);  // 100ms

    Serial.println("✅ VEML7700 initialized successfully");
    return true;
}

VEML7700Data CustomVEML7700::read() {
    VEML7700Data data = {0.0f, 0.0f, 0};

    // Read illuminance
    data.lux = veml.readLux();

    // Read white channel
    data.white = veml.readWhite();

    // Read raw ALS data
    data.als = veml.readALS();

    return data;
}

void CustomVEML7700::setGain(uint8_t gain) {
    veml.setGain(gain);
    Serial.print("💡 VEML7700 gain changed to: ");
    switch(gain) {
        case 0:
            Serial.println("1x");
            break;
        case 1:
            Serial.println("2x");
            break;
        case 2:
            Serial.println("1/4x");
            break;
        case 3:
            Serial.println("1/8x");
            break;
        default:
            Serial.println("Unknown");
    }
}

void CustomVEML7700::setIntegrationTime(uint8_t it) {
    veml.setIntegrationTime(it);
    Serial.print("💡 VEML7700 integration time changed to: ");
    switch(it) {
        case 0:
            Serial.println("25ms");
            break;
        case 1:
            Serial.println("50ms");
            break;
        case 2:
            Serial.println("100ms");
            break;
        case 3:
            Serial.println("200ms");
            break;
        case 4:
            Serial.println("400ms");
            break;
        case 5:
            Serial.println("800ms");
            break;
        default:
            Serial.println("Unknown");
    }
}
