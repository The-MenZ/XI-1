#include "Custom_MICS5524.h"

float CustomMICS5524::calculateMedian(float *arr, int size) {
    float temp[5];
    for (int i = 0; i < size; i++) {
        temp[i] = arr[i];
    }

    // Bubble sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            if (temp[i] > temp[j]) {
                float t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    }
    return temp[size / 2];
}

CustomMICS5524::CustomMICS5524(int adc_pin, int heater_pin)
    : ADC_PIN(adc_pin), HEATER_PIN(heater_pin) {
    for (int i = 0; i < 5; i++) {
        voltage_history[i] = 0.0f;
    }
}

void CustomMICS5524::begin() {
    pinMode(ADC_PIN, INPUT);
    if (HEATER_PIN >= 0) {
        pinMode(HEATER_PIN, OUTPUT);
        enableHeater(true);
    }

    Serial.println("✅ MICS5524 initialized successfully");
}

void CustomMICS5524::enableHeater(bool enable) {
    if (HEATER_PIN >= 0) {
        if (enable) {
            digitalWrite(HEATER_PIN, LOW);   // EN=Low enables heater
        } else {
            digitalWrite(HEATER_PIN, HIGH);  // EN=High disables heater
        }
    }
}

void CustomMICS5524::calibrate() {
    // Calibrate in fresh air
    // Calculate reference resistance by averaging samples
    float sum_resistance = 0.0f;
    const int calib_samples = 50;

    for (int i = 0; i < calib_samples; i++) {
        int raw = analogRead(ADC_PIN);
        float v = (float)raw / ADC_RESOLUTION * REF_VOLTAGE;

        // Rs = Rl * (Vref - Vout) / Vout
        if (v > 0.01f) {  // Prevent division by zero
            float rs = LOAD_RESISTANCE * (REF_VOLTAGE - v) / v;
            sum_resistance += rs;
        }
        delay(10);
    }

    Ro = sum_resistance / calib_samples;
    Serial.print("📊 MICS5524 calibrated: Ro=");
    Serial.print(Ro);
    Serial.println(" Ω");
}

MICS5524Data CustomMICS5524::read() {
    MICS5524Data data = {0, 0.0f, 0.0f, 0.0f, 0.0f, false};

    // Sampling
    double sum_voltage = 0.0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int val = analogRead(ADC_PIN);
        float v = (float)val / ADC_RESOLUTION * REF_VOLTAGE;
        sum_voltage += v;
        delayMicroseconds(100);
    }

    // Average voltage
    float mean_voltage = sum_voltage / SAMPLE_SIZE;

    // Filtering with median
    voltage_history[history_index] = mean_voltage;
    history_index = (history_index + 1) % 5;

    if (!history_filled && history_index == 0) {
        history_filled = true;
    }

    float filtered_voltage;
    if (history_filled) {
        filtered_voltage = calculateMedian(voltage_history, 5);
    } else {
        filtered_voltage = mean_voltage;
    }

    // Store data
    data.raw_value = (int)(mean_voltage / REF_VOLTAGE * ADC_RESOLUTION);
    data.voltage = filtered_voltage;

    // Calculate sensor resistance
    // Rs = Rl * (Vref - Vout) / Vout
    if (filtered_voltage > 0.01f) {
        data.resistance = LOAD_RESISTANCE * (REF_VOLTAGE - filtered_voltage) / filtered_voltage;
    } else {
        data.resistance = LOAD_RESISTANCE;
    }

    // Rs/Ro ratio
    if (Ro > 0) {
        data.rs_ro_ratio = data.resistance / Ro;
    } else {
        data.rs_ro_ratio = 1.0f;
    }

    // PPM calculation (CO reference curve)
    // log(ppm) = A * log(Rs/Ro) + B
    if (data.rs_ro_ratio > 0.01f) {
        float log_ratio = log10(data.rs_ro_ratio);
        float log_ppm = CO_A * log_ratio + CO_B;
        data.ppm_equivalent = pow(10, log_ppm);

        // Limit unrealistic values
        if (data.ppm_equivalent < 0) data.ppm_equivalent = 0;
        if (data.ppm_equivalent > 10000) data.ppm_equivalent = 10000;
    } else {
        data.ppm_equivalent = 0;
    }

    // Heater status
    data.heater_enabled = (HEATER_PIN >= 0) && (digitalRead(HEATER_PIN) == LOW);

    return data;
}
