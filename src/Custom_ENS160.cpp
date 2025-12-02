#include "Custom_ENS160.h"

bool CustomENS160::begin() {
    if (!ens.begin()) {
        Serial.println("❌ ENS160 initialization failed");
        return false;
    }

    ens.setMode(ENS160_OPMODE_STD);
    Serial.println("✅ ENS160 initialized successfully");
    return true;
}

ENS160Data CustomENS160::read() {
    ENS160Data data = {0.0f, 0.0f, 0};

    if (ens.available()) {
        ens.measure(true);
        data.eco2 = ens.geteCO2();
        data.tvoc = ens.getTVOC();
        data.aqi = ens.getAQI();
    }

    return data;
}
