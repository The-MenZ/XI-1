#include "display_screens.h"
// 全画面共通のフォント FreeMonoBold12pt7b は main.cpp が読み込んで設定する
// （ここでも使うとフォントのデータが2つ入ってしまう）
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <qrcode.h>
#include <U8g2_for_Adafruit_GFX.h>

// 関数プロトタイプ（main.cppで定義）
extern String getAQIStatusEnglish(uint8_t aqi);

// U8g2 のフォントで描くためのオブジェクト（Climate Modern と Pressure の画面で共用）
// begin() は描画先を覚えるだけなので、使う画面の先頭で毎回呼ぶ
static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// ============================================================================
// 画面0: Air Quality (空気質)
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
// 画面1: Climate (温熱環境)
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
      display.print(lroundf(aht21_data.temperature));
      display.println("C");
    } else if (bmp280_enabled) {
      display.setCursor(2, 56);
      display.print("Temp:  ");
      display.print(lroundf(bmp280_data.temperature));
      display.println("C");
    }
    
    // 行2: 湿度
    if (aht21_enabled) {
      display.setCursor(2, 88);
      display.print("RH:    ");
      display.print(lroundf(aht21_data.humidity));
      display.println("%");
    }
    
    // 行3: 気圧
    if (bmp280_enabled) {
      display.setCursor(2, 120);
      display.print("hPa:   ");
      display.print(lroundf(bmp280_data.pressure));
      display.println("");
    }
    
    // 行4: 高度
    if (bmp280_enabled) {
      display.setCursor(2, 152);
      display.print("Alt:   ");
      display.print(lroundf(fabsf(bmp280_data.altitude)));
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
// 画面2: Ambience (視聴覚環境)
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
      display.print(lroundf(veml7700_data.lux));
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
      display.print(lroundf(max9814_data.db_level));
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
// 画面3: Gas Sensor (ガスセンサー)
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
// 画面4: 日時表示（NTP同期時刻）
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
// 画面5: 月日表示（シンプル & 洒落たデザイン）
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
// 画面6: WiFi QRコード表示 / 画面8: 設定ページ QRコード表示
// ============================================================================

// 中央にアイコンを重ねて隠れたマスは誤り訂正で補うので、誤り訂正レベルは Q（約25%まで復元）
// 誤り訂正レベル Q・バイトモードで各バージョンに入る最大バイト数（v1〜v10）
// ricmoo/QRCode は容量チェックをせず、溢れるとスタック上のバッファを壊すため、
// ここで収まるバージョンを選んでから生成する
static const uint16_t QR_BYTE_CAPACITY_Q[] = {11, 20, 32, 46, 60, 74, 86, 108, 130, 151};
static const uint8_t QR_MAX_VERSION = sizeof(QR_BYTE_CAPACITY_Q) / sizeof(QR_BYTE_CAPACITY_Q[0]);

// v7 以降は中央に位置合わせパターンが入るので、アイコンを重ねない
static const uint8_t QR_ICON_MAX_VERSION = 6;

// QR 中央のアイコン（32×32、1bit、Adafruit GFX の drawBitmap 形式）
// 隠れるのは QR 全体の 8% 以下（v2〜v6、200px 表示時）
#define QR_ICON_SIZE 32
#define QR_ICON_MARGIN 2  // アイコンの周りの白い余白（px）

// WiFi マーク（WiFi QR 用）
static const uint8_t ICON_WIFI[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x3F, 0xFC, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x07, 0xFF, 0xFF, 0xE0,
  0x1F, 0xF0, 0x0F, 0xF8, 0x3F, 0x00, 0x00, 0xFC, 0x7C, 0x00, 0x00, 0x3E,
  0xF8, 0x07, 0xE0, 0x1F, 0xE0, 0x7F, 0xFE, 0x07, 0x41, 0xFF, 0xFF, 0x82,
  0x03, 0xFF, 0xFF, 0xC0, 0x07, 0xE0, 0x07, 0xE0, 0x0F, 0x80, 0x01, 0xF0,
  0x07, 0x00, 0x00, 0xE0, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x3F, 0xFC, 0x00,
  0x00, 0x7F, 0xFE, 0x00, 0x00, 0x7E, 0x7E, 0x00, 0x00, 0x30, 0x0C, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x07, 0xE0, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// 歯車（設定ページ QR 用）
static const uint8_t ICON_GEAR[] = {
  0x00, 0x01, 0x80, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x07, 0xE0, 0x00,
  0x01, 0x07, 0xE0, 0x80, 0x03, 0x87, 0xE1, 0xC0, 0x07, 0xC7, 0xE3, 0xE0,
  0x0F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF8, 0x0F, 0xFF, 0xFF, 0xF0,
  0x07, 0xFF, 0xFF, 0xE0, 0x03, 0xFF, 0xFF, 0xC0, 0x03, 0xFC, 0x3F, 0xC0,
  0x03, 0xF8, 0x1F, 0xC0, 0x7F, 0xF0, 0x0F, 0xFE, 0x7F, 0xE0, 0x07, 0xFE,
  0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0x7F, 0xE0, 0x07, 0xFE,
  0x7F, 0xF0, 0x0F, 0xFE, 0x03, 0xF8, 0x1F, 0xC0, 0x03, 0xFC, 0x3F, 0xC0,
  0x03, 0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xF0,
  0x1F, 0xFF, 0xFF, 0xF8, 0x0F, 0xFF, 0xFF, 0xF0, 0x07, 0xC7, 0xE3, 0xE0,
  0x03, 0x87, 0xE1, 0xC0, 0x01, 0x07, 0xE0, 0x80, 0x00, 0x07, 0xE0, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0x00, 0x01, 0x80, 0x00,
};

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

// text を QR コードにして画面中央に描き、icon があれば中央に重ねる
static void drawQRCode(const String& text, const uint8_t* icon) {
  // 文字列が収まる最小のバージョンを選ぶ（小さいほど1マスを大きく描ける）
  uint8_t version = 0;
  for (uint8_t v = 1; v <= QR_MAX_VERSION; v++) {
    if (text.length() <= QR_BYTE_CAPACITY_Q[v - 1]) {
      version = v;
      break;
    }
  }

  if (version == 0) {
    Serial.printf("DEBUG: QRCode data too long (%u bytes)\n", text.length());
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 100);
    display.print("QR: data too long");
    return;
  }

  // QRコード用のバッファ確保（バージョンに応じたサイズ）
  uint8_t qrcodedata[qrcode_getBufferSize(version)];
  struct QRCode qrcode;

  int err = qrcode_initText(&qrcode, qrcodedata, version, ECC_QUARTILE, text.c_str());

  Serial.printf("DEBUG: QRCode gen err=%d, version=%d, size=%d, bytes=%u\n",
                err, version, qrcode.size, text.length());

  if (err != 0) {
    // エラー時はエラーメッセージ表示
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 100);
    display.printf("QR Error: %d\n", err);
    return;
  }
  
  // QRコードのドット一個のピクセルサイズ（画面いっぱいに表示）
  // 周りに最低1マスの白い余白を残す（余白がないとカメラが QR の境目を見つけにくい）
  int module_size = display.width() / (qrcode.size + 2);
  if (((qrcode.size + 2) * module_size) > display.height()) {
    module_size = display.height() / (qrcode.size + 2);
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

  if (icon == nullptr || version > QR_ICON_MAX_VERSION) {
    return;
  }

  // アイコン + 余白を覆う奇数マスの白い正方形を、QR の中央のマスに合わせて置く
  // （QR の一辺は必ず奇数マスなので、奇数マスの正方形はちょうど中央に収まる）
  int box = (QR_ICON_SIZE + QR_ICON_MARGIN * 2 + module_size - 1) / module_size;
  if (box % 2 == 0) {
    box++;
  }
  int box_px = box * module_size;
  int box_x = start_x + (qrcode.size - box) / 2 * module_size;
  int box_y = start_y + (qrcode.size - box) / 2 * module_size;
  display.fillRect(box_x, box_y, box_px, box_px, GxEPD_WHITE);
  display.drawBitmap(
    box_x + (box_px - QR_ICON_SIZE) / 2,
    box_y + (box_px - QR_ICON_SIZE) / 2,
    icon, QR_ICON_SIZE, QR_ICON_SIZE, GxEPD_BLACK
  );
}

void displayWiFiQR(const String& ssid, const String& password) {
  // QRコード用の文字列生成（WiFi設定フォーマット）
  // フォーマット: WIFI:T:認証方式;S:SSID;P:パスワード;; （パスワードなしは T:nopass）
  String wifi_string;
  if (password.length() == 0) {
    wifi_string = "WIFI:T:nopass;S:" + escapeWiFiQRField(ssid) + ";;";
  } else {
    wifi_string = "WIFI:T:WPA;S:" + escapeWiFiQRField(ssid) +
                  ";P:" + escapeWiFiQRField(password) + ";;";
  }
  drawQRCode(wifi_string, ICON_WIFI);
}

void displaySettingsQR(const String& url) {
  if (url.length() == 0) {
    // WiFi 切断中（自動再接続を待っている間）は URL がない
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);
    display.setCursor(20, 90);
    display.print("Settings");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 120);
    display.print("WiFi: Not Connected");
    return;
  }
  drawQRCode(url, ICON_GEAR);
}

// ============================================================================
// 画面7: Climate Modern (温度/湿度モダンデザイン)
// ============================================================================
void displayClimateModern(
  const AHT21Data& aht21_data, bool aht21_enabled,
  const BMP280Data& bmp280_data, bool bmp280_enabled,
  const float* temp_history, const float* humid_history, 
  int history_count, int history_index
) {
  u8g2Fonts.begin(display);

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
    humidity = (int)lroundf(aht21_data.humidity);
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
  u8g2Fonts.print(lroundf(temperature));
  
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
      int idx1 = (history_index - history_count + i + HISTORY_SIZE) % HISTORY_SIZE;
      int idx2 = (history_index - history_count + i + 1 + HISTORY_SIZE) % HISTORY_SIZE;
      
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
        int idx1 = (history_index - history_count + i + HISTORY_SIZE) % HISTORY_SIZE;
        int idx2 = (history_index - history_count + i + 1 + HISTORY_SIZE) % HISTORY_SIZE;
        
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

// ============================================================================
// 画面9: Pressure（天気図風の H/L と等圧線、3時間の気圧の傾向）
// ============================================================================
// 中央に H / L と今の海面気圧を出し、1013 hPa との間に 4hPa ごとに等圧線を引く
// 線が多いほど 1013 から離れた強い高気圧・低気圧。右下のバッジは3時間の変化

#define PRESSURE_STANDARD 1013   // H と L の境目（標準気圧 hPa）
#define ISOBAR_STEP 4            // 等圧線の間隔（hPa。天気図と同じ）
#define ISOBAR_CENTER_X 100
#define ISOBAR_CENTER_Y 92
#define ISOBAR_FIRST_R 62        // 一番内側の線の半径（px。H/L と気圧の数字を囲む）
#define ISOBAR_SPACING 14        // 線どうしの間隔（px）
#define ISOBAR_LAST_R 118        // 一番外側の線の半径の上限（超える本数なら間隔を詰める）
#define ISOBAR_MAX 12            // 描く線の上限（間隔 約5px。1013 から 48hPa 離れた台風並み）
#define ISOBAR_SEGMENTS 90       // 1周を何本の線分で描くか
#define ISOBAR_PHASE_STEP 0.7f   // 線ごとに揺らし方をずらす量（同じ形が重ならないように）
#define ISOBAR_LABEL_INNER_T -0.5f  // 一番内側の線の気圧を書く角度（rad、右上）
#define ISOBAR_LABEL_OUTER_T 2.24f  // 一番外側の線の気圧を書く角度（rad、左下の角の方向）
#define TREND_STEADY 1.0f        // 3時間の変化がこれ未満なら横ばい（→）
#define TREND_RAPID 3.0f         // これ以上なら急な変化（▲▲ / ▼▼）

// 等圧線の半径。角度 t によって少し揺らし、手描きの天気図のような形にする
static float isobarRadius(float base_r, float t, float phase) {
  return base_r * (1.0f + 0.06f * sinf(2 * t + phase) + 0.035f * sinf(3 * t + 2 * phase + 1));
}

// 等圧線を1本描く（太さを 2px にするため、半径を 1px ずらして2周描く）
static void drawIsobar(float base_r, float phase) {
  for (int w = 0; w < 2; w++) {
    int16_t prev_x = 0;
    int16_t prev_y = 0;
    for (int j = 0; j <= ISOBAR_SEGMENTS; j++) {
      float t = j * (float)TWO_PI / ISOBAR_SEGMENTS;
      float r = isobarRadius(base_r + w, t, phase);
      int16_t x = ISOBAR_CENTER_X + lroundf(r * cosf(t));
      int16_t y = ISOBAR_CENTER_Y + lroundf(r * sinf(t));
      if (j > 0) {
        display.drawLine(prev_x, prev_y, x, y, GxEPD_BLACK);
      }
      prev_x = x;
      prev_y = y;
    }
  }
}

// 等圧線の角度 t の位置に、その線の気圧を白い箱に載せて書く
// 箱が画面からはみ出すときは、画面の内側に寄せる
static void drawIsobarLabel(float base_r, float phase, float t, int value) {
  float r = isobarRadius(base_r, t, phase);
  int16_t x = ISOBAR_CENTER_X + lroundf(r * cosf(t));
  int16_t y = ISOBAR_CENTER_Y + lroundf(r * sinf(t));

  String label(value);
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(label, 0, 0, &bx, &by, &bw, &bh);
  int16_t box_w = bw + 4;
  int16_t box_h = bh + 4;
  int16_t left = constrain(x - box_w / 2, 0, display.width() - box_w);
  int16_t top = constrain(y - box_h / 2, 0, display.height() - box_h);
  display.fillRect(left, top, box_w, box_h, GxEPD_WHITE);
  display.setCursor(left + 2 - bx, top + 2 - by);
  display.print(label);
}

// 傾向の三角（幅 12px・高さ 10px。top は三角の上端）
static void drawTrendTriangle(int16_t cx, int16_t top, bool up) {
  if (up) {
    display.fillTriangle(cx, top, cx - 6, top + 10, cx + 6, top + 10, GxEPD_WHITE);
  } else {
    display.fillTriangle(cx - 6, top, cx + 6, top, cx, top + 10, GxEPD_WHITE);
  }
}

// 右下の3時間の傾向のバッジ（黒い角丸に白抜き）
// ▲ / ▼: 1〜3hPa、▲▲ / ▼▼: 3hPa 以上、→: 1hPa 未満。三角の大きさはどれも同じ
static void drawTrendBadge(float trend) {
  // 白い縁取りで等圧線から切り離してから、黒いバッジを描く
  display.fillRoundRect(116, 152, 82, 46, 11, GxEPD_WHITE);
  display.fillRoundRect(120, 156, 74, 38, 7, GxEPD_BLACK);

  // 表示する値（小数1桁）で判定する（「1.0」と出ているのに横ばい、とならないように）
  float change = roundf(fabsf(trend) * 10) / 10;
  const int16_t cx = 136;  // 三角の左右の中心（バッジの上下の中心は y=175）
  if (change < TREND_STEADY) {
    display.fillTriangle(cx - 5, 169, cx - 5, 181, cx + 5, 175, GxEPD_WHITE);
  } else if (change >= TREND_RAPID) {
    drawTrendTriangle(cx, 163, trend > 0);
    drawTrendTriangle(cx, 176, trend > 0);
  } else {
    drawTrendTriangle(cx, 170, trend > 0);
  }

  // 変化の大きさ（符号は三角の向きで表す）と、その上に「3h」
  u8g2Fonts.setFontMode(1);  // 背景を塗らない
  u8g2Fonts.setForegroundColor(GxEPD_WHITE);
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  u8g2Fonts.setCursor(190 - u8g2Fonts.getUTF8Width("3h"), 168);
  u8g2Fonts.print("3h");

  char value[8];
  snprintf(value, sizeof(value), "%.1f", change);
  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  u8g2Fonts.setCursor(190 - u8g2Fonts.getUTF8Width(value), 190);
  u8g2Fonts.print(value);
}

void displayPressure(const BMP280Data& bmp280_data, bool bmp280_enabled, float trend_3h) {
  if (!bmp280_enabled) {
    display.setCursor(2, 56);
    display.println("BMP280");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
    return;
  }

  int pressure = lroundf(bmp280_data.sea_level_pressure);
  bool high = pressure >= PRESSURE_STANDARD;

  // 等圧線: 今の気圧から 1013 に向かって 4hPa ごと（今の気圧ちょうどの線は引かない）
  // first は一番内側（今の気圧に一番近い）線の気圧
  int first;
  int count;
  if (high) {
    first = (pressure - 1) / ISOBAR_STEP * ISOBAR_STEP;  // 今より低い、一番近い 4 の倍数
    count = first > PRESSURE_STANDARD ? (first - PRESSURE_STANDARD - 1) / ISOBAR_STEP + 1 : 0;
  } else {
    first = (pressure / ISOBAR_STEP + 1) * ISOBAR_STEP;  // 今より高い、一番近い 4 の倍数
    count = first < PRESSURE_STANDARD ? (PRESSURE_STANDARD - 1 - first) / ISOBAR_STEP + 1 : 0;
  }
  int step = high ? -ISOBAR_STEP : ISOBAR_STEP;
  if (count > ISOBAR_MAX) {
    count = ISOBAR_MAX;
  }

  // 外側の線が ISOBAR_LAST_R を超える本数なら、間隔を詰めて収める（台風のように線が混み合う）
  float spacing = ISOBAR_SPACING;
  if (count > 1 && ISOBAR_FIRST_R + (count - 1) * ISOBAR_SPACING > ISOBAR_LAST_R) {
    spacing = (float)(ISOBAR_LAST_R - ISOBAR_FIRST_R) / (count - 1);
  }
  for (int i = 0; i < count; i++) {
    drawIsobar(ISOBAR_FIRST_R + i * spacing, i * ISOBAR_PHASE_STEP);
  }

  // 線の気圧（一番内側と一番外側の線だけ）。線を描き終えてから載せて、白い箱で線を切る
  if (count > 0) {
    drawIsobarLabel(ISOBAR_FIRST_R, 0, ISOBAR_LABEL_INNER_T, first);
  }
  if (count > 1) {
    int last = count - 1;
    drawIsobarLabel(ISOBAR_FIRST_R + last * spacing, last * ISOBAR_PHASE_STEP,
                    ISOBAR_LABEL_OUTER_T, first + last * step);
  }

  // H / L（月日画面と同じ FreeMonoBold24pt7b を2倍にして使う）
  const char* mark = high ? "H" : "L";
  display.setFont(&FreeMonoBold24pt7b);
  display.setTextSize(2);
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(mark, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor(ISOBAR_CENTER_X - bw / 2 - bx, 104);
  display.print(mark);
  display.setTextSize(1);
  display.setFont();

  // 今の海面気圧
  u8g2Fonts.begin(display);
  u8g2Fonts.setFontMode(1);  // 背景を塗らない
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  char value[8];
  snprintf(value, sizeof(value), "%d", pressure);
  u8g2Fonts.setCursor(ISOBAR_CENTER_X - u8g2Fonts.getUTF8Width(value) / 2, 138);
  u8g2Fonts.print(value);

  // 3時間分の履歴がたまるまでは傾向のバッジを出さない
  if (!isnan(trend_3h)) {
    drawTrendBadge(trend_3h);
  }
}
