#include "Custom_ENS160.h"
#include <Wire.h>

bool CustomENS160::begin() {
    if (!ens.begin()) {
        Serial.println("❌ ENS160 initialization failed");
        return false;
    }

    ens.setMode(ENS160_OPMODE_STD);
    start_time = millis();
    Serial.println("✅ ENS160 initialized successfully");
    return true;
}

// DATA_STATUS の有効フラグ（bit 3〜2）を読む。ライブラリには読む関数がないので直接読む
uint8_t CustomENS160::readValidity() {
    Wire.beginTransmission(ENS160_I2C_ADDRESS);
    Wire.write(ENS160_REG_DATA_STATUS);
    if (Wire.endTransmission() != 0 || Wire.requestFrom(ENS160_I2C_ADDRESS, 1) != 1) {
        return ENS160_VALIDITY_INVALID;
    }
    return (Wire.read() >> 2) & 0x03;
}

ENS160Data CustomENS160::read() {
    ENS160Data data = {0.0f, 0.0f, 0, ENS160_VALIDITY_INVALID, 0.0f};

    if (ens.available()) {
        ens.measure(true);
        data.eco2 = ens.geteCO2();
        data.tvoc = ens.getTVOC();
        data.aqi = ens.getAQI();
        data.validity = readValidity();
    }

    // 時間を過ぎても、フラグが通常に戻るまでは 1 にしない
    if (data.validity == ENS160_VALIDITY_NORMAL) {
        data.warmup_progress = 1.0f;
    } else {
        unsigned long duration = data.validity == ENS160_VALIDITY_STARTUP ? STARTUP_MS : WARMUP_MS;
        data.warmup_progress = min((float)(millis() - start_time) / duration, 0.99f);
    }

    return data;
}
