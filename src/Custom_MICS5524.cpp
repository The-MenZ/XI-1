#include "Custom_MICS5524.h"
#include "adc_util.h"

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
    heater_on_time = millis();

    Serial.println("✅ MICS5524 initialized successfully");
}

bool CustomMICS5524::isWarmedUp() const {
    return millis() - heater_on_time >= WARMUP_MS;
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
    int valid_samples = 0;
    const int calib_samples = 50;

    for (int i = 0; i < calib_samples; i++) {
        float v = adcRawToVolts(analogRead(ADC_PIN));

        // Rs = Rl * (Vc - Vout) / Vout
        if (v >= MIN_BASELINE_VOLTAGE) {  // 低すぎる出力は基準にしない（0 での割り算も防ぐ）
            float rs = LOAD_RESISTANCE * (CIRCUIT_VOLTAGE - v) / v;
            sum_resistance += rs;
            valid_samples++;
        }
        delay(10);
    }

    if (valid_samples == 0) {
        Serial.println("⚠️  MICS5524 calibration failed (no valid samples)");
        return;
    }

    Ro = sum_resistance / valid_samples;

    // それまでの日ごとの記録を捨てて、今日の分を今の Ro から始める
    for (int i = 0; i < RO_DAYS; i++) {
        daily_max_rs[i] = 0.0f;
    }
    day_index = 0;
    day_start = millis();
    daily_max_rs[0] = Ro;
    calibrated = true;
    Serial.print("📊 MICS5524 calibrated: Ro=");
    Serial.print(Ro);
    Serial.println(" Ω");
}

void CustomMICS5524::updateBaseline(float rs) {
    // 1日たったら次の日の分へ進め、いちばん古い日の分を捨てる
    while (millis() - day_start >= DAY_MS) {
        day_start += DAY_MS;
        day_index = (day_index + 1) % RO_DAYS;
        daily_max_rs[day_index] = 0.0f;
    }
    if (rs > daily_max_rs[day_index]) {
        daily_max_rs[day_index] = rs;
    }

    Ro = 0.0f;
    for (int i = 0; i < RO_DAYS; i++) {
        if (daily_max_rs[i] > Ro) {
            Ro = daily_max_rs[i];
        }
    }
}

MICS5524Data CustomMICS5524::read() {
    MICS5524Data data = {0, 0.0f, 0.0f, 0.0f, 0.0f, false, false, 0.0f, 0.0f, 0.0f};

    // Sampling
    long sum_raw = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        sum_raw += analogRead(ADC_PIN);
        delayMicroseconds(100);
    }

    // Average voltage
    float mean_raw = (float)sum_raw / SAMPLE_SIZE;
    float mean_voltage = adcRawToVolts(mean_raw);

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
    data.raw_value = (int)(mean_raw + 0.5f);
    data.voltage = filtered_voltage;

    // Calculate sensor resistance
    // Rs = Rl * (Vc - Vout) / Vout
    if (filtered_voltage > 0.01f) {
        data.resistance = LOAD_RESISTANCE * (CIRCUIT_VOLTAGE - filtered_voltage) / filtered_voltage;
    } else {
        data.resistance = LOAD_RESISTANCE;
    }

    // Rs/Ro と ppm はウォームアップ後のキャリブレーションが済んでから
    data.calibrated = calibrated;
    if (calibrated && Ro > 0) {
        // 基準の Ro を直近の記録に合わせてから比を出す（Ro は直近の最大なので Rs/Ro は 1 を超えない）
        // 出力が低すぎるときは基準に入れず、Ro はそのまま（このときだけ Rs/Ro が 1 を超えうる）
        if (filtered_voltage >= MIN_BASELINE_VOLTAGE) {
            updateBaseline(data.resistance);
        }
        data.ro = Ro;
        data.rs_ro_ratio = data.resistance / Ro;

        // PPM calculation (CO reference)
        // Rs/Ro が閾値より高い（きれいな空気）なら 0、下がるほど CO が多い
        float ppm = (CO_RATIO_THRESHOLD - data.rs_ro_ratio) / CO_RATIO_PER_PPM;
        if (ppm < 1.0f) ppm = 0.0f;
        if (ppm > 1000.0f) ppm = 1000.0f;
        data.ppm_equivalent = ppm;

        // ADC で測れる上限の電圧のときの Rs。ガスがこれより濃くても Rs はここで頭打ちになる
        float v_max = adcRawToVolts(4095);
        data.rs_ro_min = LOAD_RESISTANCE * (CIRCUIT_VOLTAGE - v_max) / v_max / Ro;
    }

    unsigned long elapsed = millis() - heater_on_time;
    data.warmup_progress = elapsed >= WARMUP_MS ? 1.0f : (float)elapsed / WARMUP_MS;

    // Heater status
    data.heater_enabled = (HEATER_PIN >= 0) && (digitalRead(HEATER_PIN) == LOW);

    return data;
}
