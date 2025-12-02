#include "display_screens.h"
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <qrcode.h>

// 関数プロトタイプ（main.cppで定義）
extern String getAQIStatusEnglish(uint8_t aqi);

// ============================================================================
// 画面1: Air Quality (空気質)
// ============================================================================
void displayAirQuality(const ENS160Data& ens160_data, bool sensor_enabled) {
  // タイトル
  display.setCursor(2, 24);
  display.println("Air Quality");
  
  if (sensor_enabled) {
    // 行1: eCO2
    display.setCursor(2, 56);
    display.print("eCO2: ");
    display.print((int)ens160_data.eco2);
    display.println("ppm");
    
    // 行2: TVOC
    display.setCursor(2, 88);
    display.print("TVOC: ");
    display.print((int)ens160_data.tvoc);
    display.println("ppb");
    
    // 行3: AQI
    display.setCursor(2, 120);
    display.print("AQI:  ");
    display.print(ens160_data.aqi);
    display.println("/5");
    
    // 行4: AQI ステータス
    display.setCursor(2, 152);
    String aqi_status = getAQIStatusEnglish(ens160_data.aqi);
    display.println(aqi_status.c_str());
  } else {
    display.setCursor(2, 56);
    display.println("ENS160");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
  }
}

// ============================================================================
// 画面2: Climate (温熱環境)
// ============================================================================
void displayClimate(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled
) {
  // タイトル
  display.setCursor(2, 24);
  display.println("Climate");
  
  if (aht21_enabled || bmp280_enabled) {
    // 行1: 温度
    if (aht21_enabled) {
      display.setCursor(2, 56);
      display.print("Temp:  ");
      // 🔧 e-paper表示用にオフセット補正を適用（-10°C）
      display.print((int)(aht21_data.temperature - 10.0));
      display.println("C");
    } else if (bmp280_enabled) {
      display.setCursor(2, 56);
      display.print("Temp:  ");
      display.print((int)bmp280_data.temperature);
      display.println("C");
    }
    
    // 行2: 湿度
    if (aht21_enabled) {
      display.setCursor(2, 88);
      display.print("RH:    ");
      display.print((int)aht21_data.humidity);
      display.println("%");
    }
    
    // 行3: 気圧
    if (bmp280_enabled) {
      display.setCursor(2, 120);
      display.print("hPa:   ");
      display.print((int)bmp280_data.pressure);
      display.println("");
    }
    
    // 行4: 高度
    if (bmp280_enabled) {
      display.setCursor(2, 152);
      display.print("Alt:   ");
      display.print((int)abs(bmp280_data.altitude));
      display.println("m");
    }
  } else {
    display.setCursor(2, 56);
    display.println("No");
    display.setCursor(2, 88);
    display.println("Climate");
    display.setCursor(2, 120);
    display.println("Sensors");
  }
}

// ============================================================================
// 画面3: Ambience (視聴覚環境)
// ============================================================================
void displayAmbience(
  const VEML7700Data& veml7700_data, bool veml7700_enabled,
  const MAX9814Data& max9814_data, bool max9814_enabled
) {
  // タイトル
  display.setCursor(2, 24);
  display.println("Ambience");
  
  if (veml7700_enabled || max9814_enabled) {
    // 行1: 照度
    if (veml7700_enabled) {
      display.setCursor(2, 56);
      display.print("Lux:   ");
      display.print((int)veml7700_data.lux);
      display.println("lx");
      
      // 行2: ホワイトチャネル（参考値）
      display.setCursor(2, 88);
      display.print("White: ");
      display.print((int)veml7700_data.white);
      display.println("");
    } else {
      display.setCursor(2, 56);
      display.println("No Light");
    }
    
    // 行3: 騒音レベル（dB）
    if (max9814_enabled) {
      display.setCursor(2, 120);
      display.print("Sound: ");
      display.print((int)max9814_data.db_level);
      display.println("dB");
      
      // 行4: RMS電圧値
      display.setCursor(2, 152);
      display.print("RMS:   ");
      display.print(max9814_data.rms_voltage, 3);
      display.println("V");
    } else {
      display.setCursor(2, 120);
      display.println("No Sound");
    }
  } else {
    display.setCursor(2, 56);
    display.println("No");
    display.setCursor(2, 88);
    display.println("Ambience");
    display.setCursor(2, 120);
    display.println("Sensors");
  }
}

// ============================================================================
// 画面4: Gas Sensor (ガスセンサー)
// ============================================================================
void displayGasSensor(const MICS5524Data& mics5524_data, bool sensor_enabled) {
  // タイトル
  display.setCursor(2, 24);
  display.println("Gas Sensor");
  
  if (sensor_enabled) {
    // Rs/Ro比率（シンプル表示）
    display.setCursor(2, 120);
    display.print("Rs/Ro: ");
    display.print(mics5524_data.rs_ro_ratio, 2);
    display.println("");
  } else {
    display.setCursor(2, 56);
    display.println("MICS5524");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
  }
}

// ============================================================================
// 画面5: 日時表示（NTP同期時刻）
// ============================================================================
void displayDateTime(bool isWiFiConnected, const String& dateTimeStr) {
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(30, 30);
  display.setFont(&FreeSans12pt7b);
  display.println("Date & Time");
  
  display.setCursor(10, 80);
  display.setFont(&FreeSans18pt7b);
  display.println(dateTimeStr);
  
  display.setCursor(20, 130);
  display.setFont(&FreeSans9pt7b);
  if (isWiFiConnected) {
    display.println("WiFi: Connected");
  } else {
    display.println("WiFi: Not Connected");
  }
}

// ============================================================================
// 画面6: 月日表示（シンプル & 洒落たデザイン）
// ============================================================================
void displayMonthDay(int month, int day) {
  display.setTextColor(GxEPD_BLACK);
  // FreeMonoBold24pt7b: 等幅太字フォント（余白がぴったり）
  display.setFont(&FreeMonoBold24pt7b);
  display.setTextSize(2);
  
  // 月を2文字でゼロパディング（上半分にセンター）
  String month_str = (month < 10 ? "0" : "") + String(month);
  int16_t x_month = (display.width() - (month_str.length() * 60)) / 2;
  display.setCursor(x_month, 85);
  display.println(month_str);
  
  // 日を2文字でゼロパディング（下半分にセンター）
  String day_str = (day < 10 ? "0" : "") + String(day);
  int16_t x_day = (display.width() - (day_str.length() * 60)) / 2;
  display.setCursor(x_day, 175);
  display.println(day_str);
  
  // フォント設定をリセット（次の画面表示に影響しないように）
  display.setTextSize(1);
  display.setFont();
}

// ============================================================================
// 画面7: WiFi QRコード表示
// ============================================================================
void displayWiFiQR(const String& ssid, const String& password) {
  // QRコード用の文字列生成（WiFi設定フォーマット）
  // フォーマット: WIFI:T:認証方式;S:SSID;P:パスワード;;
  String wifi_string = "WIFI:T:WPA;S:" + ssid + ";P:" + password + ";;";
  
  // QRコード用のバッファ確保
  // ricmoo/QRCode: バッファサイズ = version 1 で約150bytes必要
  uint8_t qrcodedata[152];
  struct QRCode qrcode;
  
  // QRコード生成（version 1で十分。21x21モジュール = 189x189ピクセル@module_size=9）
  int err = qrcode_initText(&qrcode, qrcodedata, 1, ECC_LOW, wifi_string.c_str());
  
  Serial.printf("DEBUG: QRCode gen err=%d, size=%d\n", err, qrcode.size);
  
  if (err != 0) {
    // エラー時はエラーメッセージ表示
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 100);
    display.printf("QR Error: %d\n", err);
    return;
  }
  
  // QRコードのドット一個のピクセルサイズ（画面いっぱいに表示）
  int module_size = display.width() / qrcode.size;
  if ((qrcode.size * module_size) > display.height()) {
    module_size = display.height() / qrcode.size;
  }
  int qr_width = qrcode.size * module_size;
  int start_x = (display.width() - qr_width) / 2;
  int start_y = (display.height() - qr_width) / 2;
  
  Serial.printf("DEBUG: module_size=%d, qr_width=%d, start_x=%d, start_y=%d\n", 
                module_size, qr_width, start_x, start_y);
  
  // QRコードを描画（fillRectで高速化）
  display.setTextColor(GxEPD_BLACK);
  int black_pixels = 0;
  
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // 黒い矩形を描画（drawPixelより高速）
        display.fillRect(
          start_x + x * module_size,
          start_y + y * module_size,
          module_size,
          module_size,
          GxEPD_BLACK
        );
        black_pixels++;
      }
    }
  }
  
  Serial.printf("DEBUG: black_pixels=%d (drawn with fillRect)\n", black_pixels);
}
