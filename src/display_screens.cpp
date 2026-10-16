#include "display_screens.h"
// 全画面共通のフォント FreeMonoBold12pt7b は main.cpp が読み込んで設定する
// （ここでも使うとフォントのデータが2つ入ってしまう）
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <qrcode.h>
#include <U8g2_for_Adafruit_GFX.h>

// 関数プロトタイプ（main.cppで定義）
extern String getAQIStatusEnglish(uint8_t aqi);

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
// 画面5: 月日表示（4分割の大きな丸ゴシック風数字）
// ============================================================================
// 画面を 100×100 の4区画に分け、上段に月、下段に日を1桁ずつ描く（10月17日 → 1 0 / 1 7）
// 数字はフォントを使わず、中心線（直線と楕円弧）を丸いペンでなぞって描く。
// 線の太さが一定で端と継ぎ目が丸くなり、データも数百バイトで済む

#define MONTHDAY_CELL 100     // 1桁の区画（px）
#define MONTHDAY_DIGIT_W 84   // 数字の外形の幅（px）
#define MONTHDAY_DIGIT_H 96   // 数字の外形の高さ（px）
#define MONTHDAY_PEN_R 10     // ペンの半径（線の太さは 2r+1 = 21px）

// 数字の中心線を 幅60 × 高さ80 の設計座標で表す（y は下向き）
struct DigitStroke {
  char kind;              // 'L' = 直線, 'A' = 楕円弧
  int8_t p0, p1, p2, p3;  // 直線: 始点 x, y と終点 x, y / 弧: 中心 x, y と半径 x, y
  int16_t a0, a1;         // 弧の開始角と終了角（度。0 = 右、90 = 下で、a0 → a1 と時計回りに進む）
};

#define STROKE_LINE(x0, y0, x1, y1) {'L', x0, y0, x1, y1, 0, 0}
#define STROKE_ARC(cx, cy, rx, ry, a0, a1) {'A', cx, cy, rx, ry, a0, a1}

static const DigitStroke DIGIT_0[] = {STROKE_ARC(30, 40, 30, 40, 0, 360)};
static const DigitStroke DIGIT_1[] = {STROKE_LINE(35, 0, 35, 80), STROKE_LINE(35, 0, 13, 15)};
static const DigitStroke DIGIT_2[] = {STROKE_ARC(30, 26, 28, 26, 195, 400), STROKE_LINE(51, 43, 0, 80), STROKE_LINE(0, 80, 60, 80)};
static const DigitStroke DIGIT_3[] = {STROKE_ARC(30, 20, 26, 20, 200, 450), STROKE_ARC(30, 60, 30, 20, 270, 520), STROKE_LINE(20, 40, 30, 40)};
static const DigitStroke DIGIT_4[] = {STROKE_LINE(46, 0, 0, 58), STROKE_LINE(0, 58, 60, 58), STROKE_LINE(46, 0, 46, 80)};
static const DigitStroke DIGIT_5[] = {STROKE_LINE(56, 0, 10, 0), STROKE_LINE(10, 0, 10, 35), STROKE_ARC(30, 54, 30, 26, 228, 510)};
static const DigitStroke DIGIT_6[] = {STROKE_ARC(30, 52, 30, 28, 0, 360), STROKE_ARC(46, 52, 46, 52, 180, 258)};
static const DigitStroke DIGIT_7[] = {STROKE_LINE(0, 0, 60, 0), STROKE_LINE(60, 0, 22, 80)};
static const DigitStroke DIGIT_8[] = {STROKE_ARC(30, 20, 25, 20, 0, 360), STROKE_ARC(30, 59, 30, 21, 0, 360)};
static const DigitStroke DIGIT_9[] = {STROKE_ARC(30, 28, 30, 28, 0, 360), STROKE_ARC(14, 28, 46, 52, 0, 78)};

struct DigitGlyph {
  const DigitStroke* strokes;
  uint8_t count;
};

#define DIGIT_GLYPH(strokes) {strokes, sizeof(strokes) / sizeof(strokes[0])}
static const DigitGlyph DIGIT_GLYPHS[10] = {
  DIGIT_GLYPH(DIGIT_0), DIGIT_GLYPH(DIGIT_1), DIGIT_GLYPH(DIGIT_2), DIGIT_GLYPH(DIGIT_3), DIGIT_GLYPH(DIGIT_4),
  DIGIT_GLYPH(DIGIT_5), DIGIT_GLYPH(DIGIT_6), DIGIT_GLYPH(DIGIT_7), DIGIT_GLYPH(DIGIT_8), DIGIT_GLYPH(DIGIT_9),
};

// 半径 r の丸いペンで (x0, y0) → (x1, y1) をなぞる（両端に円、間を2つの三角形で埋める）
static void drawRoundLine(float x0, float y0, float x1, float y1, int16_t r) {
  float dx = x1 - x0, dy = y1 - y0;
  float len = sqrtf(dx * dx + dy * dy);
  if (len > 0.5f) {
    // 線に垂直な長さ r のベクトル
    float nx = -dy / len * r, ny = dx / len * r;
    int16_t ax = lroundf(x0 + nx), ay = lroundf(y0 + ny);
    int16_t bx = lroundf(x0 - nx), by = lroundf(y0 - ny);
    int16_t cx = lroundf(x1 - nx), cy = lroundf(y1 - ny);
    int16_t ex = lroundf(x1 + nx), ey = lroundf(y1 + ny);
    display.fillTriangle(ax, ay, bx, by, cx, cy, GxEPD_BLACK);
    display.fillTriangle(ax, ay, cx, cy, ex, ey, GxEPD_BLACK);
  }
  display.fillCircle(lroundf(x0), lroundf(y0), r, GxEPD_BLACK);
  display.fillCircle(lroundf(x1), lroundf(y1), r, GxEPD_BLACK);
}

// 区画 (cellX, cellY) の中央に数字を1つ描く
static void drawDigitInCell(int digit, int16_t cellX, int16_t cellY) {
  const int16_t r = MONTHDAY_PEN_R;
  // 設計座標 → 画面座標。外形が DIGIT_W × DIGIT_H になるよう、ペンの太さを除いた分に中心線を合わせる
  float sx = (MONTHDAY_DIGIT_W - 2 * r - 1) / 60.0f;
  float sy = (MONTHDAY_DIGIT_H - 2 * r - 1) / 80.0f;
  float ox = cellX + (MONTHDAY_CELL - MONTHDAY_DIGIT_W) / 2.0f + r;
  float oy = cellY + (MONTHDAY_CELL - MONTHDAY_DIGIT_H) / 2.0f + r;

  const DigitGlyph& glyph = DIGIT_GLYPHS[digit];
  for (uint8_t i = 0; i < glyph.count; i++) {
    const DigitStroke& s = glyph.strokes[i];
    if (s.kind == 'L') {
      drawRoundLine(ox + s.p0 * sx, oy + s.p1 * sy, ox + s.p2 * sx, oy + s.p3 * sy, r);
      continue;
    }

    // 楕円弧は約3pxごとの折れ線にする（継ぎ目もペンの円で丸くなる）
    float cx = ox + s.p0 * sx, cy = oy + s.p1 * sy;
    float rx = s.p2 * sx, ry = s.p3 * sy;
    float a0 = s.a0 * DEG_TO_RAD, a1 = s.a1 * DEG_TO_RAD;
    int n = max(2, (int)ceilf((a1 - a0) * max(rx, ry) / 3));
    float px = cx + rx * cosf(a0), py = cy + ry * sinf(a0);
    for (int k = 1; k <= n; k++) {
      float a = a0 + (a1 - a0) * k / n;
      float qx = cx + rx * cosf(a), qy = cy + ry * sinf(a);
      drawRoundLine(px, py, qx, qy, r);
      px = qx;
      py = qy;
    }
  }
}

void displayMonthDay(int month, int day) {
  drawDigitInCell(month / 10, 0, 0);
  drawDigitInCell(month % 10, MONTHDAY_CELL, 0);
  drawDigitInCell(day / 10, 0, MONTHDAY_CELL);
  drawDigitInCell(day % 10, MONTHDAY_CELL, MONTHDAY_CELL);
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

// U8g2 フォントで描くためのオブジェクト（画面7: Climate Modern、画面9: Weather、画面10: Pressure で共用）
static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

static void beginU8g2Fonts() {
  static bool initialized = false;
  if (!initialized) {
    u8g2Fonts.begin(display);
    initialized = true;
  }
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
  beginU8g2Fonts();

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
// 画面9: Weather（天気）
// ============================================================================
// アイコンは画像を持たずに図形で描く。ESP32-C3 には FPU がないので座標は整数で計算する

// 天気の種類（アイコンの描き分け。WEATHER_FOG 以降は雲の下に何か描く）
enum WeatherKind {
  WEATHER_CLEAR,
  WEATHER_PARTLY_CLOUDY,
  WEATHER_CLOUDY,
  WEATHER_FOG,
  WEATHER_RAIN,
  WEATHER_SNOW,
  WEATHER_THUNDER,
};

// WMO 天気コードを天気の種類と表示名にする
// （コードの一覧: https://open-meteo.com/en/docs の「WMO Weather interpretation codes」）
static WeatherKind weatherKind(int code, const char** label) {
  if (code <= 1) {
    *label = code == 0 ? "Clear" : "Mostly Clear";
    return WEATHER_CLEAR;
  }
  if (code == 2) {
    *label = "Partly Cloudy";
    return WEATHER_PARTLY_CLOUDY;
  }
  if (code == 3) {
    *label = "Cloudy";
    return WEATHER_CLOUDY;
  }
  if (code <= 48) {
    *label = "Fog";
    return WEATHER_FOG;
  }
  if (code <= 57) {
    *label = "Drizzle";
    return WEATHER_RAIN;
  }
  if (code <= 67) {
    *label = "Rain";
    return WEATHER_RAIN;
  }
  if (code <= 77) {
    *label = "Snow";
    return WEATHER_SNOW;
  }
  if (code <= 82) {
    *label = "Showers";
    return WEATHER_RAIN;
  }
  if (code <= 86) {
    *label = "Snow Showers";
    return WEATHER_SNOW;
  }
  *label = "Thunderstorm";
  return WEATHER_THUNDER;
}

// アイコンの寸法は大きさの単位 u の 10分の k で決める
#define U(k) ((int16_t)(u * (k) / 10))

// 2px の太さの線
static void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
  display.drawLine(x0 + 1, y0, x1 + 1, y1, GxEPD_BLACK);
  display.drawLine(x0, y0 + 1, x1, y1 + 1, GxEPD_BLACK);
}

// 45°ごとの8方向（×10）
static const int8_t DIR8[8][2] = {{10, 0}, {7, 7}, {0, 10}, {-7, 7}, {-10, 0}, {-7, -7}, {0, -10}, {7, -7}};

// 太陽（r = 光線の先までの半径）
static void drawSun(int16_t cx, int16_t cy, int16_t r) {
  display.fillCircle(cx, cy, r / 2, GxEPD_BLACK);
  for (int i = 0; i < 8; i++) {
    drawThickLine(cx + DIR8[i][0] * r * 7 / 100, cy + DIR8[i][1] * r * 7 / 100,
                  cx + DIR8[i][0] * r / 10, cy + DIR8[i][1] * r / 10);
  }
}

// 三日月（r = 半径）
static void drawMoon(int16_t cx, int16_t cy, int16_t r) {
  display.fillCircle(cx, cy, r, GxEPD_BLACK);
  display.fillCircle(cx + r / 2, cy - r * 35 / 100, r * 8 / 10, GxEPD_WHITE);
}

// 雲の形を塗る（grow だけ大きく）。(cx, by) は雲の底の中央（雲の幅は 5u、高さは 3.2u）
static void fillCloud(int16_t cx, int16_t by, int16_t u, int16_t grow, uint16_t color) {
  display.fillCircle(cx - U(14), by - U(10), U(10) + grow, color);
  display.fillCircle(cx - U(2), by - U(16), U(16) + grow, color);
  display.fillCircle(cx + U(15), by - U(11), U(11) + grow, color);
  display.fillRect(cx - U(14), by - U(11), U(29), U(11) + grow, color);
}

// 雲（白抜きで輪郭だけ。後ろの太陽や月は隠す）
static void drawCloud(int16_t cx, int16_t by, int16_t u) {
  fillCloud(cx, by, u, 2, GxEPD_BLACK);
  fillCloud(cx, by, u, 0, GxEPD_WHITE);
}

// 天気アイコンを (cx, cy) を中心に描く（約 5u 四方）
static void drawWeatherIcon(int16_t cx, int16_t cy, int16_t u, WeatherKind kind, bool is_day) {
  if (kind == WEATHER_CLEAR) {
    if (is_day) {
      drawSun(cx, cy, U(24));
    } else {
      drawMoon(cx, cy, U(17));
    }
    return;
  }

  if (kind == WEATHER_PARTLY_CLOUDY) {
    // 左上に太陽（夜は月）、右下に少し小さい雲
    if (is_day) {
      drawSun(cx - U(9), cy - U(6), U(18));
    } else {
      drawMoon(cx - U(9), cy - U(8), U(13));
    }
    drawCloud(cx + U(5), cy + U(21), u * 85 / 100);
    return;
  }

  // 雨や雪などは雲の下に描くので、そのぶん雲を上に置く
  int16_t by = kind >= WEATHER_FOG ? cy + U(7) : cy + U(16);
  drawCloud(cx, by, u);

  switch (kind) {
    case WEATHER_FOG:
      drawThickLine(cx - U(22), by + U(6), cx + U(18), by + U(6));
      drawThickLine(cx - U(16), by + U(13), cx + U(24), by + U(13));
      break;
    case WEATHER_RAIN:
      for (int i = -1; i <= 1; i++) {
        int16_t x = cx + i * U(13);
        drawThickLine(x + U(3), by + U(4), x - U(3), by + U(15));
      }
      break;
    case WEATHER_SNOW:
      // 雪の結晶（線を60°ずつ3本）
      for (int i = -1; i <= 1; i++) {
        int16_t x = cx + i * U(13);
        int16_t y = by + U(10);
        int16_t s = U(5);
        int16_t sy = s * 87 / 100;
        drawThickLine(x - s, y, x + s, y);
        drawThickLine(x - s / 2, y - sy, x + s / 2, y + sy);
        drawThickLine(x - s / 2, y + sy, x + s / 2, y - sy);
      }
      break;
    case WEATHER_THUNDER:
      // 稲妻（ジグザグ）
      drawThickLine(cx + U(5), by + U(3), cx - U(4), by + U(12));
      drawThickLine(cx - U(4), by + U(12), cx + U(4), by + U(12));
      drawThickLine(cx + U(4), by + U(12), cx - U(5), by + U(22));
      break;
    default:
      break;
  }
}

#undef U

// 曜日（0 = 日曜）。Sakamoto の方法（mktime より軽い）
static int dayOfWeek(int y, int m, int d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) {
    y--;
  }
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

// U8g2 フォントで text を cx を中心に描く（y はベースライン）
static void printCentered(const char* text, int16_t cx, int16_t y) {
  u8g2Fonts.setCursor(cx - u8g2Fonts.getUTF8Width(text) / 2, y);
  u8g2Fonts.print(text);
}

void displayWeather(const WeatherData& weather, bool has_location, bool wifi_connected) {
  if (!has_location || !weather.valid) {
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);
    display.setCursor(20, 80);
    display.print("Weather");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 110);
    if (!has_location) {
      display.print("Set the location");
      display.setCursor(20, 130);
      display.print("on the settings page");
    } else if (!wifi_connected) {
      display.print("WiFi: Not Connected");
    } else {
      display.print("Loading...");
    }
    return;
  }

  beginU8g2Fonts();
  u8g2Fonts.setFontMode(1);  // 文字の背景は塗らない
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

  // ===== 上半分：今の天気 =====
  // 1行目: 天気の名前（左）と取得した時刻（右）
  const char* label;
  WeatherKind kind = weatherKind(weather.code, &label);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  u8g2Fonts.setCursor(4, 15);
  u8g2Fonts.print(label);

  if (weather.fetched_at != 0) {
    struct tm t;
    localtime_r(&weather.fetched_at, &t);
    char updated[16];
    strftime(updated, sizeof(updated), "Updated %H:%M", &t);
    u8g2Fonts.setFont(u8g2_font_helvR08_tf);
    u8g2Fonts.setCursor(196 - u8g2Fonts.getUTF8Width(updated), 13);
    u8g2Fonts.print(updated);
  }

  drawWeatherIcon(48, 60, 13, kind, weather.is_day);

  // 気温（整数）と °C を右の列の中央に
  char text[20];
  snprintf(text, sizeof(text), "%ld", lroundf(weather.temp));
  u8g2Fonts.setFont(u8g2_font_logisoso42_tn);
  int16_t temp_w = u8g2Fonts.getUTF8Width(text);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  int16_t x = 144 - (temp_w + 2 + u8g2Fonts.getUTF8Width("°C")) / 2;
  u8g2Fonts.setFont(u8g2_font_logisoso42_tn);
  u8g2Fonts.setCursor(x, 84);
  u8g2Fonts.print(text);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  u8g2Fonts.setCursor(x + temp_w + 2, 56);
  u8g2Fonts.print("°C");

  display.drawFastHLine(6, 104, 188, GxEPD_BLACK);
  display.drawFastVLine(100, 110, 84, GxEPD_BLACK);

  // ===== 下半分：今日（左）と明日（右）の予報 =====
  static const char* const WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  for (int i = 0; i < 2; i++) {
    const WeatherDay& day = weather.days[i];
    int16_t cx = 50 + i * 100;

    // 日付（例: 10/17 Sat。date は YYYY-MM-DD）
    int month = atoi(day.date + 5);
    int mday = atoi(day.date + 8);
    if (month >= 1 && month <= 12) {
      snprintf(text, sizeof(text), "%d/%d %s", month, mday, WEEKDAYS[dayOfWeek(atoi(day.date), month, mday)]);
      u8g2Fonts.setFont(u8g2_font_helvB12_tf);
      printCentered(text, cx, 121);
    }

    const char* day_label;
    drawWeatherIcon(cx, 146, 8, weatherKind(day.code, &day_label), true);

    // 最高 / 最低気温
    snprintf(text, sizeof(text), "%ld° / %ld°", lroundf(day.temp_max), lroundf(day.temp_min));
    u8g2Fonts.setFont(u8g2_font_helvB12_tf);
    printCentered(text, cx, 184);

    if (day.precip_prob >= 0) {
      snprintf(text, sizeof(text), "Rain %d%%", day.precip_prob);
      u8g2Fonts.setFont(u8g2_font_helvR08_tf);
      printCentered(text, cx, 197);
    }
  }
}

// ============================================================================
// 画面10: Pressure（天気図風の H/L と等圧線、3時間の気圧の傾向）
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
#define ISOBAR_LABEL_INNER_T -0.4f  // 一番内側の線の気圧を書く角度（rad、右上。H の影に触れない位置）
#define ISOBAR_LABEL_OUTER_T 2.24f  // 一番外側の線の気圧を書く角度（rad、左下の角の方向）
#define TREND_STEADY 1.0f        // 3時間の変化がこれ未満なら横ばい（→）
#define TREND_RAPID 3.0f         // これ以上なら急な変化（▲▲ / ▼▼）

// H / L のブロック体（フォントを使わず長方形で描く。白抜きの太字に黒い縁と、右下に黒い影）
#define MARK_X 76         // 字の左上（縁と影を除く）
#define MARK_Y 40
#define MARK_W 46         // H の幅
#define MARK_L_W 40       // L の横棒の長さ
#define MARK_H 52         // 字の高さ
#define MARK_STROKE 14    // 線の太さ
#define MARK_OUTLINE 3    // 黒い縁の太さ
#define MARK_SHADOW 6     // 影を右下へずらす量

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

// H / L を形づくる長方形を、(dx, dy) ずらし、上下左右に grow ずつ太らせて塗る
static void fillMarkRects(bool high, int16_t dx, int16_t dy, int16_t grow, uint16_t color) {
  int16_t x = MARK_X + dx - grow;
  int16_t y = MARK_Y + dy - grow;
  int16_t stroke = MARK_STROKE + 2 * grow;
  int16_t height = MARK_H + 2 * grow;
  display.fillRect(x, y, stroke, height, color);  // 左の縦棒
  if (high) {
    display.fillRect(x + MARK_W - MARK_STROKE, y, stroke, height, color);                  // 右の縦棒
    display.fillRect(x, y + (MARK_H - MARK_STROKE) / 2, MARK_W + 2 * grow, stroke, color);  // 横棒
  } else {
    display.fillRect(x, y + MARK_H - MARK_STROKE, MARK_L_W + 2 * grow, stroke, color);  // 下の横棒
  }
}

// 影 → 黒い縁 → 中の白、の順に重ねて、影つきの白抜き文字にする
static void drawPressureMark(bool high) {
  fillMarkRects(high, MARK_SHADOW, MARK_SHADOW, MARK_OUTLINE, GxEPD_BLACK);
  fillMarkRects(high, 0, 0, MARK_OUTLINE, GxEPD_BLACK);
  fillMarkRects(high, 0, 0, 0, GxEPD_WHITE);
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

  drawPressureMark(high);

  // 今の海面気圧
  beginU8g2Fonts();
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
