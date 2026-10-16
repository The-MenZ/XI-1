#include "display_screens.h"
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <qrcode.h>
#include <U8g2_for_Adafruit_GFX.h>

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
      // 自己発熱補正済みの値（補正量は main.cpp の AHT21_TEMP_OFFSET）
      display.print((int)aht21_data.temperature);
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
  
  if (sensor_enabled && !mics5524_data.calibrated) {
    // ヒーターのウォームアップ中（起動から3分間）
    display.setCursor(2, 120);
    display.println("Warming up");
  } else if (sensor_enabled) {
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

// 誤り訂正レベル L・バイトモードで各バージョンに入る最大バイト数（v1〜v10）
// ricmoo/QRCode は容量チェックをせず、溢れるとスタック上のバッファを壊すため、
// ここで収まるバージョンを選んでから生成する
static const uint16_t QR_BYTE_CAPACITY_L[] = {17, 32, 53, 78, 106, 134, 154, 192, 230, 271};
static const uint8_t QR_MAX_VERSION = sizeof(QR_BYTE_CAPACITY_L) / sizeof(QR_BYTE_CAPACITY_L[0]);

// WiFi QR の値に含まれる特殊文字（\ ; , : "）をバックスラッシュでエスケープ
static String escapeWiFiQRField(const String& value) {
  String escaped;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '\\' || c == ';' || c == ',' || c == ':' || c == '"') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}

void displayWiFiQR(const String& ssid, const String& password) {
  // QRコード用の文字列生成（WiFi設定フォーマット）
  // フォーマット: WIFI:T:認証方式;S:SSID;P:パスワード;;
  String wifi_string = "WIFI:T:WPA;S:" + escapeWiFiQRField(ssid) +
                       ";P:" + escapeWiFiQRField(password) + ";;";

  // 文字列が収まる最小のバージョンを選ぶ（小さいほど1マスを大きく描ける）
  uint8_t version = 0;
  for (uint8_t v = 1; v <= QR_MAX_VERSION; v++) {
    if (wifi_string.length() <= QR_BYTE_CAPACITY_L[v - 1]) {
      version = v;
      break;
    }
  }

  if (version == 0) {
    Serial.printf("DEBUG: QRCode data too long (%u bytes)\n", wifi_string.length());
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 100);
    display.print("QR: data too long");
    return;
  }

  // QRコード用のバッファ確保（バージョンに応じたサイズ）
  uint8_t qrcodedata[qrcode_getBufferSize(version)];
  struct QRCode qrcode;

  int err = qrcode_initText(&qrcode, qrcodedata, version, ECC_LOW, wifi_string.c_str());

  Serial.printf("DEBUG: QRCode gen err=%d, version=%d, size=%d, bytes=%u\n",
                err, version, qrcode.size, wifi_string.length());

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

// ============================================================================
// 画面8: Climate Modern (温度/湿度モダンデザイン)
// ============================================================================
void displayClimateModern(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled,
  const float* temp_history, const float* humid_history, 
  int history_count, int history_index
) {
  // U8g2フォント用のオブジェクト作成
  static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
  static bool u8g2_initialized = false;
  
  if (!u8g2_initialized) {
    u8g2Fonts.begin(display);
    u8g2_initialized = true;
  }
  
  // センサーデータチェック
  if (!aht21_enabled && !bmp280_enabled) {
    display.setCursor(2, 56);
    display.println("No");
    display.setCursor(2, 88);
    display.println("Climate");
    display.setCursor(2, 120);
    display.println("Sensors");
    return;
  }
  
  // 温度と湿度の値を取得
  float temperature = 0.0;
  int humidity = 0;
  
  if (aht21_enabled) {
    // 自己発熱補正済みの値（補正量は main.cpp の AHT21_TEMP_OFFSET）
    temperature = aht21_data.temperature;
    humidity = (int)aht21_data.humidity;
  } else if (bmp280_enabled) {
    temperature = bmp280_data.temperature;
  }
  
  // 画面中心座標
  int16_t centerX = 100;
  int16_t centerY = 75;
  
  // ===== 円形ゲージ（アーチ）描画 =====
  int16_t outerRadius = 70;
  int16_t innerRadius = 60;
  int16_t humidRadius = 60;  // 温度ゲージの内側と同じ（隙間なし）
  int16_t humidInnerRadius = 50;
  
  float tempAngle = (temperature / 50.0) * 270.0;  // 0-50°Cを270度にマップ
  float humidAngle = (humidity / 100.0) * 270.0;  // 0-100%を270度にマップ
  
  // 範囲制限
  if (tempAngle < 0) tempAngle = 0;
  if (tempAngle > 270) tempAngle = 270;
  if (humidAngle < 0) humidAngle = 0;
  if (humidAngle > 270) humidAngle = 270;
  
  // 外側の円弧（温度ゲージ背景・枠線のみ）
  for (int angle = 135; angle <= 405; angle += 2) {
    float rad = angle * PI / 180.0;
    // 外周
    int16_t x_outer = centerX + cos(rad) * outerRadius;
    int16_t y_outer = centerY + sin(rad) * outerRadius;
    display.drawPixel(x_outer, y_outer, GxEPD_BLACK);
    // 内周
    int16_t x_inner = centerX + cos(rad) * innerRadius;
    int16_t y_inner = centerY + sin(rad) * innerRadius;
    display.drawPixel(x_inner, y_inner, GxEPD_BLACK);
  }
  
  // 温度ゲージ（黒ベタ・完全な塗りつぶし）
  for (float angle = 135.0; angle <= 135.0 + tempAngle; angle += 0.5) {
    float rad = angle * PI / 180.0;
    int16_t x1 = centerX + cos(rad) * innerRadius;
    int16_t y1 = centerY + sin(rad) * innerRadius;
    int16_t x2 = centerX + cos(rad) * outerRadius;
    int16_t y2 = centerY + sin(rad) * outerRadius;
    display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
    // さらに隣接する線も描画して完全に埋める
    display.drawLine(x1+1, y1, x2+1, y2, GxEPD_BLACK);
  }
  
  // 内側の円弧（湿度ゲージ背景・枠線のみ）
  for (int angle = 135; angle <= 405; angle += 2) {
    float rad = angle * PI / 180.0;
    // 外周
    int16_t x_outer = centerX + cos(rad) * humidRadius;
    int16_t y_outer = centerY + sin(rad) * humidRadius;
    display.drawPixel(x_outer, y_outer, GxEPD_BLACK);
    // 内周
    int16_t x_inner = centerX + cos(rad) * humidInnerRadius;
    int16_t y_inner = centerY + sin(rad) * humidInnerRadius;
    display.drawPixel(x_inner, y_inner, GxEPD_BLACK);
  }
  
  // 湿度ゲージ（黒ベタ・完全な塗りつぶし）
  if (aht21_enabled) {
    for (float angle = 135.0; angle <= 135.0 + humidAngle; angle += 0.5) {
      float rad = angle * PI / 180.0;
      int16_t x1 = centerX + cos(rad) * humidInnerRadius;
      int16_t y1 = centerY + sin(rad) * humidInnerRadius;
      int16_t x2 = centerX + cos(rad) * humidRadius;
      int16_t y2 = centerY + sin(rad) * humidRadius;
      display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
      // さらに隣接する線も描画して完全に埋める
      display.drawLine(x1+1, y1, x2+1, y2, GxEPD_BLACK);
    }
  }
  
  // ===== 中央の大きな温度表示 =====
  u8g2Fonts.setFontMode(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  
  u8g2Fonts.setFont(u8g2_font_logisoso42_tn);  // 大きな数字フォント
  
  // 温度を整数で表示（小数点なし）
  u8g2Fonts.setCursor(73, 90);
  u8g2Fonts.print((int)temperature);
  
  // 湿度を右側に表示
  if (aht21_enabled) {
    u8g2Fonts.setFont(u8g2_font_helvB18_tf);
    u8g2Fonts.setCursor(80, 120);
    u8g2Fonts.print(humidity);
    u8g2Fonts.print("%");
  }

  // ===== 下部：過去24時間のグラフ（実データ） =====
  int16_t graphY = 135;
  int16_t graphHeight = 25;
  int16_t graphWidth = 180;
  int16_t graphX = 10;
  
  // ========== 温度グラフ ==========
  // "TEMP" ラベル（常に表示）
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  u8g2Fonts.setCursor(graphX, graphY - 5);
  u8g2Fonts.print("TEMP");
  
  // グリッド線（常に表示）
  for (int i = 0; i <= 2; i++) {
    int16_t y = graphY + i * (graphHeight / 2);
    for (int x = graphX; x < graphX + graphWidth; x += 4) {
      display.drawPixel(x, y, GxEPD_BLACK);
    }
  }
  
  if (history_count >= 2) {  // データがある場合のみ折れ線を描画
    
    // 温度の最小・最大を計算（動的範囲）
    float temp_min = temp_history[0];
    float temp_max = temp_history[0];
    for (int i = 0; i < history_count; i++) {
      if (temp_history[i] < temp_min) temp_min = temp_history[i];
      if (temp_history[i] > temp_max) temp_max = temp_history[i];
    }
    
    // グラフ範囲に±2°Cの余白を追加
    float graph_temp_min = temp_min - 2.0;
    float graph_temp_max = temp_max + 2.0;
    
    // 温度変化がほとんどない場合の対処
    if (graph_temp_max - graph_temp_min < 5.0) {
      float temp_avg = (temp_min + temp_max) / 2.0;
      graph_temp_min = temp_avg - 5.0;
      graph_temp_max = temp_avg + 5.0;
    }
    
    // 0°Cラインを強調表示（範囲内にある場合）
    if (graph_temp_min <= 0.0 && graph_temp_max >= 0.0) {
      // 0°CのY座標を計算（mapの代わりに手動計算）
      float normalized = (0.0 - graph_temp_min) / (graph_temp_max - graph_temp_min);
      int16_t zero_y = graphY + graphHeight - (int16_t)(normalized * graphHeight);
      
      // 黒実線（太め）で描画
      display.drawLine(graphX, zero_y, graphX + graphWidth, zero_y, GxEPD_BLACK);
      display.drawLine(graphX, zero_y+1, graphX + graphWidth, zero_y+1, GxEPD_BLACK);
    }
    
    // 温度グラフを描画
    for (int i = 0; i < history_count - 1; i++) {
      // 古い順に描画（history_indexから開始して円環状にアクセス）
      int idx1 = (history_index - history_count + i + 144) % 144;
      int idx2 = (history_index - history_count + i + 1 + 144) % 144;
      
      float t1 = temp_history[idx1];
      float t2 = temp_history[idx2];
      
      // X座標（時間軸）
      int16_t x1 = graphX + (i * graphWidth) / (history_count - 1);
      int16_t x2 = graphX + ((i + 1) * graphWidth) / (history_count - 1);
      
      // Y座標（温度軸）- 上下反転に注意
      float norm1 = (t1 - graph_temp_min) / (graph_temp_max - graph_temp_min);
      float norm2 = (t2 - graph_temp_min) / (graph_temp_max - graph_temp_min);
      int16_t y1 = graphY + graphHeight - (int16_t)(norm1 * graphHeight);
      int16_t y2 = graphY + graphHeight - (int16_t)(norm2 * graphHeight);
      
      display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
    }
  }
  
  // ========== 湿度グラフ ==========
  if (aht21_enabled) {
    graphY = 175;
    
    // "HUMIDITY" ラベル（常に表示）
    u8g2Fonts.setFont(u8g2_font_helvR08_tf);
    u8g2Fonts.setCursor(graphX, graphY - 5);
    u8g2Fonts.print("HUMIDITY");
    
    // グリッド線（常に表示）
    for (int i = 0; i <= 2; i++) {
      int16_t y = graphY + i * (graphHeight / 2);
      for (int x = graphX; x < graphX + graphWidth; x += 4) {
        display.drawPixel(x, y, GxEPD_BLACK);
      }
    }
    
    // データがある場合のみ湿度グラフを描画
    if (history_count >= 2) {
      for (int i = 0; i < history_count - 1; i++) {
        int idx1 = (history_index - history_count + i + 144) % 144;
        int idx2 = (history_index - history_count + i + 1 + 144) % 144;
        
        float h1 = humid_history[idx1];
        float h2 = humid_history[idx2];
        
        // X座標
        int16_t x1 = graphX + (i * graphWidth) / (history_count - 1);
        int16_t x2 = graphX + ((i + 1) * graphWidth) / (history_count - 1);
        
        // Y座標（0-100%固定）
        int16_t y1 = graphY + graphHeight - (int16_t)((h1 / 100.0) * graphHeight);
        int16_t y2 = graphY + graphHeight - (int16_t)((h2 / 100.0) * graphHeight);
        
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
      }
    }
  }
}
