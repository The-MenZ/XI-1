#include "Custom_VEML7700.h"

bool CustomVEML7700::begin() {
    // I2C address 0x10 (fixed)
    if (!veml.begin(&Wire)) {
        Serial.println("❌ VEML7700 initialization failed");
        return false;
    }

    // Gain: 1/8 + Integration time: 100ms
    // 室内〜窓際の明るさ（約3万 lux まで）を飽和せずに測れる設定
    // レジスタ値は名前順ではない（例: GAIN_1_8 = 2, GAIN_1_4 = 3）ので必ず定数で指定する
    veml.setGain(VEML7700_GAIN_1_8);
    veml.setIntegrationTime(VEML7700_IT_100MS);

    Serial.println("✅ VEML7700 initialized successfully");
    return true;
}

VEML7700Data CustomVEML7700::read() {
    VEML7700Data data = {0.0f, 0.0f, 0};

    // Read illuminance
    // 低ゲインでは高照度で直線性がずれるため、データシートの補正式を適用する
    data.lux = veml.readLux(VEML_LUX_CORRECTED);

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
        case VEML7700_GAIN_1:
            Serial.println("1x");
            break;
        case VEML7700_GAIN_2:
            Serial.println("2x");
            break;
        case VEML7700_GAIN_1_8:
            Serial.println("1/8x");
            break;
        case VEML7700_GAIN_1_4:
            Serial.println("1/4x");
            break;
        default:
            Serial.println("Unknown");
    }
}

void CustomVEML7700::setIntegrationTime(uint8_t it) {
    veml.setIntegrationTime(it);
    Serial.print("💡 VEML7700 integration time changed to: ");
    switch(it) {
        case VEML7700_IT_25MS:
            Serial.println("25ms");
            break;
        case VEML7700_IT_50MS:
            Serial.println("50ms");
            break;
        case VEML7700_IT_100MS:
            Serial.println("100ms");
            break;
        case VEML7700_IT_200MS:
            Serial.println("200ms");
            break;
        case VEML7700_IT_400MS:
            Serial.println("400ms");
            break;
        case VEML7700_IT_800MS:
            Serial.println("800ms");
            break;
        default:
            Serial.println("Unknown");
    }
}
