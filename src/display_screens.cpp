#include "display_screens.h"
// 全画面共通のフォント FreeMonoBold12pt7b は main.cpp が読み込んで設定する
// （ここでも使うとフォントのデータが2つ入ってしまう）
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <qrcode.h>
#include <U8g2_for_Adafruit_GFX.h>

// 他の画面で定義している描画の部品
// 丸いペン（半径 r）で線を引く（画面5 の月日表示で定義）
static void drawRoundLine(float x0, float y0, float x1, float y1, int16_t r, uint16_t color = GxEPD_BLACK);
// 2次ベジェ曲線を丸いペンでなぞる（画面3 のガスセンサーで定義。r はペンの半径）
static void drawCurve(float x0, float y0, float x1, float y1, float x2, float y2, int16_t r = 1, uint16_t color = GxEPD_BLACK);
// 丸ゴシック風の数字（0〜9）か「?」（GLYPH_QUESTION）を描く（画面5 の月日表示で定義）
#define GLYPH_QUESTION 10
static void drawGlyph(int glyph, int16_t left, int16_t top, int16_t w, int16_t h, int16_t r, int16_t edge,
                      uint16_t color = GxEPD_BLACK);
// 2px の線と、黒い太陽（画面9 の天気で定義）
static void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
static void drawSun(int16_t cx, int16_t cy, int16_t r);

// ============================================================================
// 画面0: Air Quality（大きな AQI の数字と、換気をうながす上げ下げ窓）
// ============================================================================
// 左に AQI（1〜5）の白抜きの数字と星（AQI 1 で★5つ）、右に上げ下げ窓を描く。
// 空気が悪いほど、窓ガラスも含めて部屋全体に点々のもやが濃くかかる。窓の外はいつも晴れ。
// AQI 4 で下の窓を少し上げ、AQI 5 で全開にする。開いたすき間と、そこから入る風の通り道だけもやが晴れる。
// ウォームアップ中は「?」と、10枚の羽根のブラインドを出し、10% 進むごとに羽根を1枚上げる。
// 絵は画像を持たずに、四角・円・三角と丸いペンの線で描く

// 窓（外枠の外側の位置と大きさ）。枠の内側がガラス
#define AQ_WIN_X 108
#define AQ_WIN_Y 16
#define AQ_WIN_W 82
#define AQ_WIN_H 136
#define AQ_FRAME 5                           // 枠の太さ
#define AQ_GLASS_X (AQ_WIN_X + AQ_FRAME)      // 113
#define AQ_GLASS_Y (AQ_WIN_Y + AQ_FRAME)      // 21
#define AQ_GLASS_W (AQ_WIN_W - 2 * AQ_FRAME)  // 72
#define AQ_GLASS_H (AQ_WIN_H - 2 * AQ_FRAME)  // 126
#define AQ_RAIL_Y 82        // 閉じているときの、上下の窓の合わせ目（太さ 5px）
#define AQ_ROOM_BOTTOM 160  // もやをかける範囲の下端（その下は文字の帯）
#define AQ_WIND_X 112       // 風の通り道が始まる x（窓の左の枠）

// 大きな数字の外形と、白抜きにする黒い縁の太さ
#define AQ_DIGIT_X 20
#define AQ_DIGIT_Y 38
#define AQ_DIGIT_W 64
#define AQ_DIGIT_H 88
#define AQ_DIGIT_PEN_R 9  // ペンの半径（線の太さは 2r+1 = 19px）
#define AQ_DIGIT_EDGE 4

// ブラインド（上の 6px がレールで、その下に 12px の羽根が10枚）
#define AQ_BLIND_SLATS 10
#define AQ_SLAT_H 12
#define AQ_BLIND_TOP (AQ_GLASS_Y + 6)

// AQI ごとのもやの点の間隔（px。0 は点なし）。点は 2×2px
static const uint8_t AQ_HAZE_STEP[5] = {0, 12, 8, 6, 5};

// AQI 1〜3 の下の帯の文字（4・5 は OPEN!）
static const char* const AQ_STATUS[3] = {"FRESH", "GOOD", "STUFFY"};

// 窓を開けたときの形（AQI 4 と 5）
struct AQOpening {
  int16_t pane_top;       // 上げた下の窓の上の桟の y（-1 は枠に隠れて見えない）
  int16_t pane_bottom;    // 上げた下の窓の下の桟の y。桟（4px）より下が開いたすき間
  int16_t band[4];        // 風の通り道の上端・下端の y（窓の側）と、上端・下端の y（左端）
  uint8_t lines;          // 風の線の数
  int16_t line_y[3][2];   // 風の線の始点（窓の側）と終点（左端）の y
};

static const AQOpening AQ_OPENINGS[2] = {
  {60, 126, {124, 150, 100, 132}, 2, {{134, 106}, {144, 118}, {0, 0}}},  // AQI 4: 少しだけ開ける
  {-1, 82, {82, 152, 66, 152}, 3, {{98, 82}, {116, 100}, {134, 120}}},   // AQI 5: 全開
};

// 星の10個の頂点（外と内が交互。外の半径を 100 としたとき。上から時計回り）
static const int8_t STAR_POINTS[10][2] = {
  {0, -100}, {26, -36}, {95, -31}, {43, 14}, {59, 81}, {0, 45}, {-59, 81}, {-43, 14}, {-95, -31}, {-26, -36},
};

// 今のフォントで text を cx を中心に描く（y はベースライン）
static void printCenteredText(const char* text, int16_t cx, int16_t y) {
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor(cx - bx - bw / 2, y);
  display.print(text);
}

// 太さ t の四角い枠（x, y, w, h は外側）
static void drawFrame(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t) {
  display.fillRect(x, y, w, t, GxEPD_BLACK);
  display.fillRect(x, y + h - t, w, t, GxEPD_BLACK);
  display.fillRect(x, y, t, h, GxEPD_BLACK);
  display.fillRect(x + w - t, y, t, h, GxEPD_BLACK);
}

// 星（filled なら塗り、そうでなければ輪郭だけ）
static void drawStar(int16_t cx, int16_t cy, int16_t r, bool filled) {
  for (int i = 0; i < 10; i++) {
    const int8_t* p = STAR_POINTS[i];
    const int8_t* q = STAR_POINTS[(i + 1) % 10];
    int16_t x0 = cx + lroundf(p[0] * r / 100.0f), y0 = cy + lroundf(p[1] * r / 100.0f);
    int16_t x1 = cx + lroundf(q[0] * r / 100.0f), y1 = cy + lroundf(q[1] * r / 100.0f);
    if (filled) {
      display.fillTriangle(cx, cy, x0, y0, x1, y1, GxEPD_BLACK);
    } else {
      drawThickLine(x0, y0, x1, y1);
    }
  }
}

// 4本の光のきらめき（AQI 1 のとき窓ガラスに出す）
static void drawSparkle(int16_t cx, int16_t cy, int16_t r) {
  int16_t q = r / 3;
  display.fillTriangle(cx - q, cy, cx + q, cy, cx, cy - r, GxEPD_BLACK);
  display.fillTriangle(cx - q, cy, cx + q, cy, cx, cy + r, GxEPD_BLACK);
  display.fillTriangle(cx, cy - q, cx, cy + q, cx - r, cy, GxEPD_BLACK);
  display.fillTriangle(cx, cy - q, cx, cy + q, cx + r, cy, GxEPD_BLACK);
}

// 鳥（翼2枚の曲線。hw は片方の翼の幅）
static void drawBird(int16_t x, int16_t y, int16_t hw) {
  drawCurve(x - hw, y - hw * 0.4f, x - hw * 0.5f, y - hw * 0.8f, x, y);
  drawCurve(x, y, x + hw * 0.5f, y - hw * 0.8f, x + hw, y - hw * 0.4f);
}

// 窓の外の景色（太陽と雪をかぶった山と鳥）。空気が悪くても、外はいつも晴れ
static void drawAQScenery() {
  const int16_t x = AQ_GLASS_X, y = AQ_GLASS_Y, w = AQ_GLASS_W, h = AQ_GLASS_H;
  // 山の稜線（左の高い山と右の低い山）と、左の山の雪の縁（ぎざぎざ）。ガラスの幅・高さに対する %
  static const int8_t RIDGE[5][2] = {{0, 100}, {34, 46}, {58, 74}, {80, 56}, {100, 82}};
  static const int8_t SNOW[5][2] = {{28, 56}, {31, 60}, {35, 55}, {38, 60}, {42, 56}};

  drawSun(x + w * 74 / 100, y + h * 20 / 100, 18);
  for (int i = 0; i < 4; i++) {
    drawRoundLine(x + w * RIDGE[i][0] / 100, y + h * RIDGE[i][1] / 100,
                  x + w * RIDGE[i + 1][0] / 100, y + h * RIDGE[i + 1][1] / 100, 1);
    drawRoundLine(x + w * SNOW[i][0] / 100, y + h * SNOW[i][1] / 100,
                  x + w * SNOW[i + 1][0] / 100, y + h * SNOW[i + 1][1] / 100, 1);
  }
  // 鳥は太陽の左の空に、右上がりに2羽
  drawBird(x + w * 15 / 100, y + h * 23 / 100, 6);
  drawBird(x + w * 33 / 100, y + h * 17 / 100, 5);
}

// 窓の外枠と窓台
static void drawAQWindowFrame() {
  drawFrame(AQ_WIN_X, AQ_WIN_Y, AQ_WIN_W, AQ_WIN_H, AQ_FRAME);
  display.fillRect(104, 150, 90, 6, GxEPD_BLACK);
}

// 左上の「AQI」（もやの上でも読めるよう、白い台にのせる）
static void drawAQLabel() {
  display.fillRoundRect(22, 6, 60, 24, 4, GxEPD_WHITE);
  printCenteredText("AQI", 52, 25);
}

// 風の通り道の上端と下端（x は 0〜AQ_WIND_X。窓から左へ、なめらかに広がりながら上がる）
static void windBand(const AQOpening& o, int16_t x, int16_t* top, int16_t* bottom) {
  float u = (float)(AQ_WIND_X - x) / AQ_WIND_X;
  float s = u * u * (3 - 2 * u);  // 両端でゆるやかになる 0〜1
  *top = o.band[0] + lroundf((o.band[2] - o.band[0]) * s);
  *bottom = o.band[1] + lroundf((o.band[3] - o.band[1]) * s);
}

// 部屋全体（窓ガラスも含む）に点々のもやをかける。窓を開けていれば、すき間と風の通り道には打たない
static void drawAQHaze(uint8_t aqi, const AQOpening* open) {
  uint8_t step = AQ_HAZE_STEP[aqi - 1];
  if (step == 0) {
    return;
  }
  for (int16_t x = step / 2; x < display.width(); x += step) {
    int16_t band_top = 0, band_bottom = -1;  // 通り道がない列は空にしておく
    if (open && x <= AQ_WIND_X) {
      windBand(*open, x, &band_top, &band_bottom);
    }
    bool gap_column = open && x >= AQ_GLASS_X && x < AQ_GLASS_X + AQ_GLASS_W;
    for (int16_t y = step / 2; y < AQ_ROOM_BOTTOM; y += step) {
      if (y >= band_top && y <= band_bottom) {
        continue;
      }
      if (gap_column && y >= open->pane_bottom + 4) {
        continue;
      }
      display.fillRect(x, y, 2, 2, GxEPD_BLACK);
    }
  }
}

// 風の線（窓の側 (104, y0) から左端 (10, y1) へ、山と谷を交互に3つ波打つ。左端に矢じり）
static void drawWindLine(int16_t y0, int16_t y1) {
  const float x0 = 104, x1 = 10;
  const int16_t r = 2;  // ペンの半径（線の太さは 5px）
  float gx = (x1 - x0) / 3, gy = (y1 - y0) / 3.0f;
  float px = x0, py = y0;
  float cx = x0 + gx / 2, cy = y0 + gy / 2 - 8;  // 最初の山の制御点
  for (int i = 0; i < 3; i++) {
    float qx = px + gx, qy = py + gy;
    drawCurve(px, py, cx, cy, qx, qy, r);
    // 次の制御点は、今の制御点を終点で折り返したところ（山の次は谷になる）
    cx = 2 * qx - cx;
    cy = 2 * qy - cy;
    px = qx;
    py = qy;
  }
  float a = atan2f(y1 - y0, x1 - x0);
  drawRoundLine(x1 - 10 * cosf(a - 0.6f), y1 - 10 * sinf(a - 0.6f), x1, y1, r);
  drawRoundLine(x1 - 10 * cosf(a + 0.6f), y1 - 10 * sinf(a + 0.6f), x1, y1, r);
}

// ウォームアップ中: 「?」と閉じたブラインド。10% 進むごとに羽根が1枚上がる（終わるまで最後の1枚は残す）
static void drawAQWarmup(float progress) {
  int raised = constrain((int)(progress * AQ_BLIND_SLATS), 0, AQ_BLIND_SLATS - 1);
  int slats = AQ_BLIND_SLATS - raised;
  int16_t edge = AQ_BLIND_TOP + slats * AQ_SLAT_H;  // ブラインドの下の端

  drawAQScenery();
  drawAQWindowFrame();
  // 羽根（白い板。上の 2px の線で隣の羽根と区切る）
  for (int i = 0; i < slats; i++) {
    int16_t y = AQ_BLIND_TOP + i * AQ_SLAT_H;
    display.fillRect(AQ_GLASS_X, y, AQ_GLASS_W, AQ_SLAT_H, GxEPD_WHITE);
    display.fillRect(AQ_GLASS_X, y, AQ_GLASS_W, 2, GxEPD_BLACK);
  }
  display.fillRect(AQ_GLASS_X, AQ_GLASS_Y, AQ_GLASS_W, 6, GxEPD_BLACK);  // 上のレール
  display.fillRect(AQ_GLASS_X, edge - 2, AQ_GLASS_W, 5, GxEPD_BLACK);    // 下の重り
  // 上げ下げのひも（左寄り。下の重りの少し下まで垂らす。右寄りだと上がりきる前に太陽と重なる）
  int16_t cord = constrain(edge + 8, 34, 140);
  display.fillRect(117, AQ_BLIND_TOP, 2, cord - AQ_BLIND_TOP, GxEPD_BLACK);
  display.fillCircle(118, cord + 2, 3, GxEPD_BLACK);

  drawGlyph(GLYPH_QUESTION, AQ_DIGIT_X, AQ_DIGIT_Y, AQ_DIGIT_W, AQ_DIGIT_H, AQ_DIGIT_PEN_R, AQ_DIGIT_EDGE);
  drawAQLabel();
  printCenteredText("WARMING UP", 100, 187);
}

void displayAirQuality(const ENS160Data& ens160_data, bool sensor_enabled) {
  if (!sensor_enabled) {
    display.setCursor(2, 24);
    display.println("Air Quality");
    display.setCursor(2, 56);
    display.println("ENS160");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
    return;
  }

  // ウォームアップ中や、まだ測れていない（AQI が 0）ときはブラインド
  uint8_t aqi = ens160_data.aqi;
  if (ens160_data.validity != ENS160_VALIDITY_NORMAL || aqi < 1 || aqi > 5) {
    drawAQWarmup(ens160_data.warmup_progress);
    return;
  }

  const AQOpening* open = aqi >= 4 ? &AQ_OPENINGS[aqi - 4] : nullptr;
  drawAQScenery();
  drawAQHaze(aqi, open);
  if (open) {
    for (int i = 0; i < open->lines; i++) {
      drawWindLine(open->line_y[i][0], open->line_y[i][1]);
    }
  }

  drawAQWindowFrame();
  if (!open) {
    display.fillRect(AQ_WIN_X, AQ_RAIL_Y, AQ_WIN_W, 5, GxEPD_BLACK);  // 上下の窓の合わせ目
    if (aqi == 1) {
      drawSparkle(AQ_GLASS_X + 10, AQ_GLASS_Y + 10, 6);
    }
  } else {
    if (open->pane_top >= 0) {
      // 少しだけ上げたときは、上の窓の合わせ目が、上げた窓のガラスごしに細く見える
      display.fillRect(AQ_GLASS_X, AQ_RAIL_Y + 1, AQ_GLASS_W, 3, GxEPD_BLACK);
      display.fillRect(AQ_WIN_X, open->pane_top, AQ_WIN_W, 4, GxEPD_BLACK);
    }
    display.fillRect(AQ_WIN_X, open->pane_bottom, AQ_WIN_W, 4, GxEPD_BLACK);
  }

  // 風の線より手前に、大きな数字と星を描く（星は白い台の上）
  drawGlyph(aqi, AQ_DIGIT_X, AQ_DIGIT_Y, AQ_DIGIT_W, AQ_DIGIT_H, AQ_DIGIT_PEN_R, AQ_DIGIT_EDGE);
  drawAQLabel();
  display.fillRoundRect(4, 136, 92, 20, 4, GxEPD_WHITE);
  for (int i = 0; i < 5; i++) {
    drawStar(16 + i * 17, 146, 7, i < 6 - aqi);
  }

  // 下の帯: AQI 1〜3 は空気の様子、4・5 は黒地に白で OPEN!
  if (!open) {
    printCenteredText(AQ_STATUS[aqi - 1], 100, 179);
  } else {
    display.fillRoundRect(30, 161, 140, 22, 4, GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    printCenteredText(aqi == 5 ? "OPEN!!" : "OPEN!", 100, 179);
    display.setTextColor(GxEPD_BLACK);
  }
  char text[32];
  snprintf(text, sizeof(text), "eCO2 %u  TVOC %u", (unsigned)ens160_data.eco2, (unsigned)ens160_data.tvoc);
  display.setFont(&FreeSans9pt7b);
  printCenteredText(text, 100, 197);
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
    
    // 行3: iPhone の騒音計アプリに合わせた騒音（dB。静かなときが 30）
    if (max9814_enabled) {
      display.setCursor(2, 120);
      display.print("Sound: ");
      display.print(lroundf(max9814_data.db_phone));
      display.println("dB");

      // 行4: 本体が測った騒音レベル（静かにしたときのこの値を、設定ページの「静かなときの騒音レベル」に入れる）
      display.setCursor(2, 152);
      display.print("Raw:   ");
      display.print(lroundf(max9814_data.db_level));
      display.println("dB");
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
// 画面3: Gas Sensor（炭鉱のカナリアと RPG 風の HP ゲージ）
// ============================================================================
// 昔の炭鉱で、有毒ガスに気づくために連れて入ったカナリアに見立てる。
// 上の枠の HP は Rs/Ro から計算し、HP が減るほどカナリアが弱る（歌う → しょんぼり → ひっくり返る）。
// ウォームアップ中（起動から3分間）は温められている卵で、ひびの長さが進み具合。
// 絵は画像を持たずに、円・楕円・三角と丸いペンの線で描く

#define CANARY_HP_SING 70    // HP がこれ以上なら歌う
#define CANARY_HP_FAINT 30   // HP がこれ未満ならひっくり返る（その間はしょんぼり）
#define CANARY_OUTLINE 3     // 体や頭の輪郭の太さ（px）
#define CANARY_PEN_R 1       // 線を引くペンの半径（線の太さは 2r+1 = 3px）
#define HP_SEGMENTS 10       // HP ゲージのマスの数（1マス = HP 10）

// 楕円を塗る。上半分と下半分で縦の半径を変えられる（卵の形に使う）
static void fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry_top, int16_t ry_bottom, uint16_t color) {
  for (int16_t dy = -ry_top; dy <= ry_bottom; dy++) {
    float ry = dy < 0 ? ry_top : ry_bottom;
    float t = ry > 0 ? dy / ry : 0;
    int16_t dx = lroundf(rx * sqrtf(1 - t * t));
    display.drawFastHLine(cx - dx, cy + dy, 2 * dx + 1, color);
  }
}

// 黒い輪郭つきの白い楕円（半径は白い部分の大きさ）
static void drawOutlinedEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry_top, int16_t ry_bottom) {
  fillEllipse(cx, cy, rx + CANARY_OUTLINE, ry_top + CANARY_OUTLINE, ry_bottom + CANARY_OUTLINE, GxEPD_BLACK);
  fillEllipse(cx, cy, rx, ry_top, ry_bottom, GxEPD_WHITE);
}

// 黒い輪郭つきの白い円（半径は白い部分の大きさ）
static void drawOutlinedCircle(int16_t cx, int16_t cy, int16_t r) {
  display.fillCircle(cx, cy, r + CANARY_OUTLINE, GxEPD_BLACK);
  display.fillCircle(cx, cy, r, GxEPD_WHITE);
}

// (x0, y0) から (x2, y2) への2次ベジェ曲線（制御点 (x1, y1)）を半径 r の丸いペンでなぞる
// r の既定値（ファイルの先頭で宣言）はカナリアの線と同じ 1（CANARY_PEN_R）。color の既定値は黒
static void drawCurve(float x0, float y0, float x1, float y1, float x2, float y2, int16_t r, uint16_t color) {
  const int n = 8;
  float px = x0, py = y0;
  for (int k = 1; k <= n; k++) {
    float t = (float)k / n;
    float u = 1 - t;
    float qx = u * u * x0 + 2 * u * t * x1 + t * t * x2;
    float qy = u * u * y0 + 2 * u * t * y1 + t * t * y2;
    drawRoundLine(px, py, qx, qy, r, color);
    px = qx;
    py = qy;
  }
}

// においや熱の、揺れながら上へのびる線（(x, y) は下の端。waves は揺れの数で、1つ 9px）
static void drawWavyLine(int16_t x, int16_t y, int waves) {
  for (int i = 0; i < waves; i++) {
    int16_t y0 = y - i * 9;
    drawCurve(x, y0, x + (i % 2 ? -6 : 6), y0 - 4, x, y0 - 9);
  }
}

// 音符（(x, y) は玉の中心）
static void drawNote(int16_t x, int16_t y) {
  fillEllipse(x, y, 5, 3, 3, GxEPD_BLACK);
  drawRoundLine(x + 4, y - 1, x + 4, y - 15, CANARY_PEN_R);
  drawCurve(x + 4, y - 15, x + 9, y - 12, x + 11, y - 7);
}

// 冷や汗（(x, y) はしずくの先。黒い縁の白いしずく）
static void drawSweatDrop(int16_t x, int16_t y) {
  display.fillTriangle(x, y, x - 5, y + 9, x + 5, y + 9, GxEPD_BLACK);
  display.fillCircle(x, y + 9, 5, GxEPD_BLACK);
  display.fillTriangle(x, y + 4, x - 3, y + 9, x + 3, y + 9, GxEPD_WHITE);
  display.fillCircle(x, y + 9, 3, GxEPD_WHITE);
}

// しっぽ（輪郭つきの白い四角形。corners は4つの角の x, y）
static void drawTail(const int16_t corners[8]) {
  const int16_t* p = corners;
  display.fillTriangle(p[0], p[1], p[2], p[3], p[4], p[5], GxEPD_WHITE);
  display.fillTriangle(p[0], p[1], p[4], p[5], p[6], p[7], GxEPD_WHITE);
  for (int i = 0; i < 4; i++) {
    int j = (i + 1) % 4;
    drawRoundLine(p[2 * i], p[2 * i + 1], p[2 * j], p[2 * j + 1], CANARY_PEN_R);
  }
}

// ブランコ（2本のひもと止まり木）。ひもは HP の枠の下から垂らす
static void drawSwing() {
  display.fillRect(71, 46, 2, 107, GxEPD_BLACK);
  display.fillRect(128, 46, 2, 107, GxEPD_BLACK);
  display.fillRoundRect(59, 153, 82, 7, 3, GxEPD_BLACK);
}

static const int16_t TAIL_SITTING[8] = {84, 134, 58, 143, 64, 152, 87, 143};
static const int16_t TAIL_FALLEN[8] = {110, 173, 122, 165, 124, 176, 112, 180};

// HP が多い: ブランコに乗って、くちばしを開けて歌う
static void drawCanarySinging() {
  drawSwing();
  drawTail(TAIL_SITTING);
  drawOutlinedEllipse(100, 135, 23, 17, 17);  // 体
  drawCurve(84, 131, 97, 150, 116, 134);      // 翼
  drawOutlinedCircle(115, 110, 14);           // 頭
  drawCurve(109, 97, 109, 88, 116, 85);       // 頭の羽
  drawCurve(114, 96, 118, 88, 125, 88);
  display.fillCircle(120, 107, 3, GxEPD_BLACK);                     // 目
  display.fillTriangle(129, 104, 143, 106, 130, 110, GxEPD_BLACK);  // くちばし（上）
  display.fillTriangle(130, 113, 141, 120, 129, 117, GxEPD_BLACK);  // くちばし（下）
  drawNote(145, 89);
  drawNote(158, 72);
}

// HP が半分くらい: うなだれて冷や汗。まわりににおいの線
static void drawCanaryDrooping() {
  drawSwing();
  drawTail(TAIL_SITTING);
  drawOutlinedEllipse(100, 137, 23, 17, 17);  // 体
  drawCurve(84, 133, 97, 152, 117, 136);      // 翼
  drawOutlinedCircle(116, 120, 14);           // 頭（下がっている）
  drawCurve(112, 106, 110, 99, 104, 97);      // しおれた頭の羽
  // 半分閉じた目（まぶたの線の下に黒目の下半分）
  display.fillCircle(122, 118, 3, GxEPD_BLACK);
  display.fillRect(118, 114, 9, 4, GxEPD_WHITE);
  drawRoundLine(118, 117, 126, 117, CANARY_PEN_R);
  display.fillTriangle(130, 120, 141, 127, 129, 129, GxEPD_BLACK);  // 閉じたくちばし
  drawSweatDrop(90, 100);
  drawWavyLine(40, 197, 4);
  drawWavyLine(52, 189, 3);
  drawWavyLine(152, 140, 3);
}

// HP が少ない: 止まり木から落ちて、床でひっくり返る（× 目で足が上）。ブランコだけが揺れている
static void drawCanaryFainted() {
  display.drawLine(67, 46, 62, 96, GxEPD_BLACK);  // 傾いたブランコのひも（2px）
  display.drawLine(68, 46, 63, 96, GxEPD_BLACK);
  display.drawLine(124, 46, 118, 102, GxEPD_BLACK);
  display.drawLine(125, 46, 119, 102, GxEPD_BLACK);
  drawRoundLine(50, 96, 130, 104, 3);    // 止まり木
  drawCurve(43, 83, 37, 93, 43, 104);    // 揺れの線
  drawCurve(161, 92, 167, 102, 161, 113);
  drawRoundLine(25, 191, 121, 191, CANARY_PEN_R);  // 床

  drawTail(TAIL_FALLEN);
  drawOutlinedEllipse(90, 176, 21, 12, 12);  // 体
  drawCurve(74, 179, 88, 167, 103, 177);     // 翼
  // 上を向いた足（足首から3本の指のうち2本だけ描く）
  for (int16_t x = 85; x <= 95; x += 10) {
    drawRoundLine(x, 162, x, 149, CANARY_PEN_R);
    drawRoundLine(x - 5, 146, x, 149, CANARY_PEN_R);
    drawRoundLine(x, 149, x + 5, 146, CANARY_PEN_R);
  }
  drawOutlinedCircle(64, 177, 11);  // 頭
  drawRoundLine(57, 172, 63, 178, CANARY_PEN_R);  // × 目
  drawRoundLine(63, 172, 57, 178, CANARY_PEN_R);
  display.fillTriangle(53, 180, 43, 177, 51, 186, GxEPD_BLACK);  // くちばし
  drawWavyLine(38, 155, 3);
  drawWavyLine(142, 159, 3);
  drawWavyLine(157, 155, 2);
}

// ウォームアップ中: ヒーターで温められている卵。ひびが左からのびて、のび切ると（3分で）かえる
static const int16_t EGG_CRACK[][2] = {
  {64, 128}, {74, 120}, {83, 132}, {93, 119}, {103, 132}, {112, 120}, {122, 132}, {131, 122}, {136, 127},
};
#define EGG_CRACK_SEGMENTS (sizeof(EGG_CRACK) / sizeof(EGG_CRACK[0]) - 1)

static void drawEgg(float progress) {
  drawWavyLine(70, 76, 2);  // 熱の線
  drawWavyLine(100, 70, 2);
  drawWavyLine(130, 76, 2);
  drawOutlinedEllipse(100, 132, 35, 49, 37);  // 上が細い卵の形
  drawRoundLine(52, 175, 148, 175, CANARY_PEN_R);  // 床

  float length = progress * EGG_CRACK_SEGMENTS;
  for (int i = 0; i < (int)EGG_CRACK_SEGMENTS && length > i; i++) {
    float f = min(1.0f, length - i);
    float x0 = EGG_CRACK[i][0], y0 = EGG_CRACK[i][1];
    drawRoundLine(x0, y0, x0 + (EGG_CRACK[i + 1][0] - x0) * f, y0 + (EGG_CRACK[i + 1][1] - y0) * f, CANARY_PEN_R);
  }
}

// RPG 風のウィンドウ（太い線と細い線の二重の枠）
static void drawHPFrame() {
  for (int i = 0; i < 3; i++) {
    display.drawRect(4 + i, 4 + i, 192 - 2 * i, 42 - 2 * i, GxEPD_BLACK);
  }
  display.drawRect(9, 9, 182, 32, GxEPD_BLACK);
}

// 枠の中に「HP」、10マスのゲージ、HP の数字（右寄せ）を並べる
// 文字は全画面共通のフォント FreeMonoBold12pt7b（main.cpp で設定済み）
static void drawHPWindow(int hp) {
  drawHPFrame();
  display.setCursor(13, 32);
  display.print("HP");

  int filled = (hp + 9) / 10;  // HP が 1 でも残っていれば1マス
  for (int i = 0; i < HP_SEGMENTS; i++) {
    int16_t x = 44 + i * 10;
    if (i < filled) {
      display.fillRect(x, 18, 8, 15, GxEPD_BLACK);
    } else {
      display.drawRect(x, 18, 8, 15, GxEPD_BLACK);
    }
  }

  char text[4];
  snprintf(text, sizeof(text), "%d", hp);
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor(186 - bx - bw, 32);
  display.print(text);
}

// HP（0〜100）。Rs/Ro が 1（キャリブレーションしたときの空気）で 100、
// 測れる下限（出力が ADC の上限に達したとき）で 0。基準よりきれいな空気でも 100 で止める
static int canaryHP(const MICS5524Data& data) {
  float range = 1.0f - data.rs_ro_min;
  if (range < 0.01f) {
    return data.rs_ro_ratio >= 1.0f ? 100 : 0;
  }
  long hp = lroundf((data.rs_ro_ratio - data.rs_ro_min) / range * 100);
  return constrain(hp, 0, 100);
}

void displayGasSensor(const MICS5524Data& mics5524_data, bool sensor_enabled) {
  if (!sensor_enabled) {
    display.setCursor(2, 24);
    display.println("Gas Sensor");
    display.setCursor(2, 56);
    display.println("MICS5524");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
    return;
  }

  if (!mics5524_data.calibrated) {
    // ヒーターのウォームアップ中（起動から3分間）
    drawEgg(mics5524_data.warmup_progress);
    drawHPFrame();
    int16_t bx, by;
    uint16_t bw, bh;
    display.getTextBounds("WARMING UP", 0, 0, &bx, &by, &bw, &bh);
    display.setCursor(100 - bx - bw / 2, 32);
    display.print("WARMING UP");
    return;
  }

  int hp = canaryHP(mics5524_data);
  if (hp >= CANARY_HP_SING) {
    drawCanarySinging();
  } else if (hp >= CANARY_HP_FAINT) {
    drawCanaryDrooping();
  } else {
    drawCanaryFainted();
  }
  drawHPWindow(hp);
}

// ============================================================================
// 画面4: Time（パタパタ時計）
// ============================================================================
// 黒い札2枚に、時と分を白い丸ゴシック風の数字で描く（15:42 → [15] [42]。24時間表示で0埋め）。
// 札の真ん中に白い切れ目、左右に蝶番、下にスタンドを描いてパタパタ時計に見せる。
// 数字は月日画面と同じ drawGlyph()。NTP で時刻が合う前は「??」を出す

#define FLIP_CARD_Y 30         // 札の上端
#define FLIP_CARD_W 91
#define FLIP_CARD_H 130
#define FLIP_CARD_R 10         // 札の角の丸み
#define FLIP_DIGIT_W 36        // 数字の外形の幅（px）
#define FLIP_DIGIT_H 84        // 数字の外形の高さ（px）
#define FLIP_DIGIT_PEN_R 6     // ペンの半径（線の太さは 2r+1 = 13px）
#define FLIP_DIGIT_LEFT 6      // 札の左端から1桁目の左端まで
#define FLIP_DIGIT_PITCH 43    // 1桁目の左端から2桁目の左端まで
#define FLIP_SPLIT_H 3         // 札の真ん中の切れ目の太さ
#define FLIP_HINGE_W 4         // 蝶番（札の左右に出る黒い四角）
#define FLIP_HINGE_H 16

// 札の左端（左が時、右が分。画面の真ん中をはさんで左右対称）
static const int16_t FLIP_CARD_X[2] = {6, 103};

// 札を1枚描く。value は 0〜99 の2桁の値（負なら「??」）
static void drawFlipCard(int16_t x, int value) {
  display.fillRoundRect(x, FLIP_CARD_Y, FLIP_CARD_W, FLIP_CARD_H, FLIP_CARD_R, GxEPD_BLACK);

  int16_t top = FLIP_CARD_Y + (FLIP_CARD_H - FLIP_DIGIT_H) / 2;
  int tens = value < 0 ? GLYPH_QUESTION : value / 10;
  int ones = value < 0 ? GLYPH_QUESTION : value % 10;
  drawGlyph(tens, x + FLIP_DIGIT_LEFT, top, FLIP_DIGIT_W, FLIP_DIGIT_H, FLIP_DIGIT_PEN_R, 0, GxEPD_WHITE);
  drawGlyph(ones, x + FLIP_DIGIT_LEFT + FLIP_DIGIT_PITCH, top, FLIP_DIGIT_W, FLIP_DIGIT_H, FLIP_DIGIT_PEN_R, 0,
            GxEPD_WHITE);

  // 真ん中の切れ目と、左右の蝶番
  int16_t mid = FLIP_CARD_Y + FLIP_CARD_H / 2;
  display.fillRect(x, mid - FLIP_SPLIT_H / 2, FLIP_CARD_W, FLIP_SPLIT_H, GxEPD_WHITE);
  display.fillRect(x - FLIP_HINGE_W, mid - FLIP_HINGE_H / 2, FLIP_HINGE_W, FLIP_HINGE_H, GxEPD_BLACK);
  display.fillRect(x + FLIP_CARD_W, mid - FLIP_HINGE_H / 2, FLIP_HINGE_W, FLIP_HINGE_H, GxEPD_BLACK);
}

void displayTime(int hour, int minute, bool synced) {
  drawFlipCard(FLIP_CARD_X[0], synced ? hour : -1);
  drawFlipCard(FLIP_CARD_X[1], synced ? minute : -1);

  // スタンド（2本の脚と横長の台）
  display.fillRect(26, 160, 10, 10, GxEPD_BLACK);
  display.fillRect(164, 160, 10, 10, GxEPD_BLACK);
  display.fillRoundRect(10, 168, 180, 6, 3, GxEPD_BLACK);
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

// 「?」（画面0 のウォームアップ中に使う）。上のかぎ、短い縦線、下の点
static const DigitStroke GLYPH_Q[] = {
  STROKE_ARC(30, 20, 24, 20, 190, 420), STROKE_LINE(42, 37, 30, 48), STROKE_LINE(30, 48, 30, 52), STROKE_LINE(30, 80, 30, 80),
};
static const DigitGlyph QUESTION_GLYPH = DIGIT_GLYPH(GLYPH_Q);

// 半径 r の丸いペンで (x0, y0) → (x1, y1) をなぞる（両端に円、間を2つの三角形で埋める）
static void drawRoundLine(float x0, float y0, float x1, float y1, int16_t r, uint16_t color) {
  float dx = x1 - x0, dy = y1 - y0;
  float len = sqrtf(dx * dx + dy * dy);
  if (len > 0.5f) {
    // 線に垂直な長さ r のベクトル
    float nx = -dy / len * r, ny = dx / len * r;
    int16_t ax = lroundf(x0 + nx), ay = lroundf(y0 + ny);
    int16_t bx = lroundf(x0 - nx), by = lroundf(y0 - ny);
    int16_t cx = lroundf(x1 - nx), cy = lroundf(y1 - ny);
    int16_t ex = lroundf(x1 + nx), ey = lroundf(y1 + ny);
    display.fillTriangle(ax, ay, bx, by, cx, cy, color);
    display.fillTriangle(ax, ay, cx, cy, ex, ey, color);
  }
  display.fillCircle(lroundf(x0), lroundf(y0), r, color);
  display.fillCircle(lroundf(x1), lroundf(y1), r, color);
}

// 字の中心線を、半径 r のペンでなぞる（設計座標 (x, y) → 画面座標 (ox + x * sx, oy + y * sy)）
static void drawGlyphStrokes(const DigitGlyph& glyph, float ox, float oy, float sx, float sy, int16_t r, uint16_t color) {
  for (uint8_t i = 0; i < glyph.count; i++) {
    const DigitStroke& s = glyph.strokes[i];
    if (s.kind == 'L') {
      drawRoundLine(ox + s.p0 * sx, oy + s.p1 * sy, ox + s.p2 * sx, oy + s.p3 * sy, r, color);
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
      drawRoundLine(px, py, qx, qy, r, color);
      px = qx;
      py = qy;
    }
  }
}

// 数字（0〜9）か「?」（GLYPH_QUESTION）を、外形が w × h になるよう (left, top) に描く
// edge が 0 なら塗り、1 以上なら太さ edge の縁だけ残した白抜きにする
// color は字の色（既定は黒。黒い背景に白で描くときは白にする。白抜きの中は反対の色になる）
static void drawGlyph(int glyph, int16_t left, int16_t top, int16_t w, int16_t h, int16_t r, int16_t edge, uint16_t color) {
  const DigitGlyph& g = glyph == GLYPH_QUESTION ? QUESTION_GLYPH : DIGIT_GLYPHS[glyph];
  // 設計座標 → 画面座標。外形が w × h になるよう、ペンの太さを除いた分に中心線を合わせる
  float sx = (w - 2 * r - 1) / 60.0f;
  float sy = (h - 2 * r - 1) / 80.0f;
  // 白抜きは、黒いペンで全部なぞってから、同じ中心線を細い白いペンでなぞる
  // （1本ずつ黒と白を重ねると、次の線の黒が前の線の白を消してしまう）
  drawGlyphStrokes(g, left + r, top + r, sx, sy, r, color);
  if (edge > 0) {
    drawGlyphStrokes(g, left + r, top + r, sx, sy, r - edge, color == GxEPD_BLACK ? GxEPD_WHITE : GxEPD_BLACK);
  }
}

// 区画 (cellX, cellY) の中央に数字を1つ描く
static void drawDigitInCell(int digit, int16_t cellX, int16_t cellY) {
  drawGlyph(digit, cellX + (MONTHDAY_CELL - MONTHDAY_DIGIT_W) / 2, cellY + (MONTHDAY_CELL - MONTHDAY_DIGIT_H) / 2,
            MONTHDAY_DIGIT_W, MONTHDAY_DIGIT_H, MONTHDAY_PEN_R, 0);
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

// QR 中央のバッジ: 外から白い枠・黒い枠・白い枠、その内側いっぱいにアイコン
// バッジは QR_BADGE_MIN_PX 以上の奇数マスの正方形（v2〜v6 で 42〜49 px）
// 隠れるのは QR 全体の 8% 以下で、どのブロックも誤り訂正で直せる数のコード語しか壊さない
// （大きくすると v3 で 9 マス = 54 px になり、小さく写したときに読めなくなる）
#define QR_BADGE_MIN_PX 42
#define QR_BADGE_GAP 2     // 外側の白い枠（px）
#define QR_BADGE_BORDER 2  // 黒い枠（px）
#define QR_BADGE_PAD 2     // 内側の白い枠（px）

// WiFi マーク（WiFi QR 用）: 点と、それを中心にした3本の弧（真上から ±45°、端は丸い）
// s×s の正方形の幅いっぱいに描く（幅 11.1u、高さ 8.8u なので上下は少し空く）
static void drawWiFiIcon(int16_t x0, int16_t y0, int16_t s) {
  float u = s / 11.1f;
  float half_t = 0.6f * u;                         // 弧の太さの半分
  float cx = x0 + s / 2.0f;
  float cy = y0 + (s - 8.8f * u) / 2 + 7.6f * u;  // 点の中心
  for (int16_t y = y0; y < y0 + s; y++) {
    for (int16_t x = x0; x < x0 + s; x++) {
      float dx = fabsf(x + 0.5f - cx);
      float dy = cy - (y + 0.5f);
      float r = sqrtf(dx * dx + dy * dy);
      bool on = r <= 1.2f * u;
      for (int k = 1; k <= 3 && !on; k++) {
        float mid = (2 * k + 1) * u;  // 弧の太さの真ん中の半径
        float end = mid * 0.7071f;    // 弧の端（45°）の x, y
        on = (dx <= dy && fabsf(r - mid) <= half_t) ||
             (dx - end) * (dx - end) + (dy - end) * (dy - end) <= half_t * half_t;
      }
      if (on) {
        display.drawPixel(x, y, GxEPD_BLACK);
      }
    }
  }
}

// 歯車（設定ページ QR 用）: 8枚の歯と中央の穴。s×s の正方形いっぱいに描く
static void drawGearIcon(int16_t x0, int16_t y0, int16_t s) {
  float ro = s / 2.0f;     // 歯の先の半径
  float hw = ro * 0.2f;    // 歯の幅の半分
  for (int16_t y = 0; y < s; y++) {
    for (int16_t x = 0; x < s; x++) {
      float dx = fabsf(x + 0.5f - ro);
      float dy = fabsf(y + 0.5f - ro);
      float r = sqrtf(dx * dx + dy * dy);
      if (r > ro || r < ro * 0.32f) {
        continue;
      }
      // 本体の円 + 中心を通る4本の帯（縦・横・斜め2本）が向かい合う2枚ずつの歯になる
      if (r <= ro * 0.72f || dx <= hw || dy <= hw || fabsf(dx - dy) <= hw * 1.4142f) {
        display.drawPixel(x0 + x, y0 + y, GxEPD_BLACK);
      }
    }
  }
}

// アイコンを s×s の正方形に描く関数
typedef void (*QRIconDrawer)(int16_t x0, int16_t y0, int16_t s);

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

// text を QR コードにして画面中央に描き、drawIcon があれば中央にアイコンのバッジを重ねる
static void drawQRCode(const String& text, QRIconDrawer drawIcon) {
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

  if (drawIcon == nullptr || version > QR_ICON_MAX_VERSION) {
    return;
  }

  // バッジは奇数マスの白い正方形にして、QR の中央のマスに合わせて置く
  // （QR の一辺は必ず奇数マスなので、奇数マスの正方形はちょうど中央に収まる）
  int box = (QR_BADGE_MIN_PX + module_size - 1) / module_size;
  if (box % 2 == 0) {
    box++;
  }
  int box_px = box * module_size;
  int box_x = start_x + (qrcode.size - box) / 2 * module_size;
  int box_y = start_y + (qrcode.size - box) / 2 * module_size;
  display.fillRect(box_x, box_y, box_px, box_px, GxEPD_WHITE);
  drawFrame(box_x + QR_BADGE_GAP, box_y + QR_BADGE_GAP,
            box_px - QR_BADGE_GAP * 2, box_px - QR_BADGE_GAP * 2, QR_BADGE_BORDER);
  // アイコンは内側の白い枠の中いっぱい（30〜37 px）
  int inset = QR_BADGE_GAP + QR_BADGE_BORDER + QR_BADGE_PAD;
  drawIcon(box_x + inset, box_y + inset, box_px - inset * 2);
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
  drawQRCode(wifi_string, drawWiFiIcon);
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
  drawQRCode(url, drawGearIcon);
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

// u8g2 のフォントを変えて、文字の背景を塗らない設定にする
// U8g2_for_Adafruit_GFX の setFont() は、フォントが変わるたびに「背景を塗る」（setFontMode(0)）に戻してしまう。
// 背景の色を決めていないと初期値の 0 で塗られ、e-Paper では 0 が黒なので、文字の枠が黒い四角になる
// （Sound 画面の「dB」が黒い四角2つになっていた）。なので setFontMode(1) は setFont() の後に呼ぶ
static void setU8g2FontTransparent(const uint8_t* font) {
  u8g2Fonts.setFont(font);
  u8g2Fonts.setFontMode(1);
}

// ============================================================================
// 画面7: Climate Modern (温度/湿度モダンデザイン)
// ============================================================================
// 上に温度と湿度の円形ゲージ（2重の輪）と、その中に大きな温度と湿度。下に24時間の折れ線グラフ

// 円形ゲージ。輪は左下（135°）から時計回りに右下（405°）まで
#define CM_CX 100
#define CM_CY 75
#define CM_ARC_START 135.0f
#define CM_ARC_SPAN 270.0f
#define CM_PEN_R 5     // 輪を描く丸いペンの半径（輪の太さは 2r+1 = 11px）
#define CM_TEMP_R 65   // 温度の輪（外側）の中心線の半径（60〜70px）
#define CM_HUMID_R 52  // 湿度の輪（内側）の中心線の半径（47〜57px。温度の輪との間を 2px あける）

// 中央の温度の数字（logisoso42）と、右上の「°」
#define CM_TEMP_BASELINE 90
#define CM_TEMP_DIGIT_H 42  // 数字の高さ（ベースラインから上端まで）
#define CM_DEGREE_R 5       // 「°」の外側の半径
#define CM_DEGREE_PEN 3     // 「°」の線の太さ
#define CM_DEGREE_GAP 3     // 数字と「°」の間

// 半径 radius の輪を、丸いペン（半径 pen_r）で 0〜ratio の割合のところまでなぞる（ratio = 1 で輪の全体）。
// 丸い端が輪の範囲（135°〜405°）からはみ出ないよう、ペンの中心は両端から CM_PEN_R だけ内側を通る
static void drawClimateArc(int16_t radius, float ratio, int16_t pen_r, uint16_t color) {
  float inset = (float)CM_PEN_R / radius * RAD_TO_DEG;
  float a0 = CM_ARC_START + inset;
  float a1 = a0 + (CM_ARC_SPAN - 2 * inset) * ratio;
  int steps = (int)ceilf((a1 - a0) / 2);  // 2° ごとの線分
  float px = CM_CX + radius * cosf(a0 * DEG_TO_RAD), py = CM_CY + radius * sinf(a0 * DEG_TO_RAD);
  drawRoundLine(px, py, px, py, pen_r, color);  // ratio = 0 でも端の丸だけは出す
  for (int i = 1; i <= steps; i++) {
    float a = (a0 + (a1 - a0) * i / steps) * DEG_TO_RAD;
    float x = CM_CX + radius * cosf(a), y = CM_CY + radius * sinf(a);
    drawRoundLine(px, py, x, y, pen_r, color);
    px = x;
    py = y;
  }
}

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
  
  // ===== 円形ゲージ（外側の輪が温度 0〜50°C、内側の輪が湿度 0〜100%） =====
  // 輪は両端の丸い帯。細い枠の中を、今の値のところまで黒く塗る
  float temp_ratio = constrain(temperature / 50.0f, 0.0f, 1.0f);
  float humid_ratio = constrain(humidity / 100.0f, 0.0f, 1.0f);

  drawClimateArc(CM_TEMP_R, 1.0f, CM_PEN_R, GxEPD_BLACK);
  drawClimateArc(CM_TEMP_R, 1.0f, CM_PEN_R - 1, GxEPD_WHITE);
  drawClimateArc(CM_TEMP_R, temp_ratio, CM_PEN_R, GxEPD_BLACK);

  drawClimateArc(CM_HUMID_R, 1.0f, CM_PEN_R, GxEPD_BLACK);
  drawClimateArc(CM_HUMID_R, 1.0f, CM_PEN_R - 1, GxEPD_WHITE);
  if (aht21_enabled) {
    drawClimateArc(CM_HUMID_R, humid_ratio, CM_PEN_R, GxEPD_BLACK);
  }

  // ===== 中央の大きな温度表示 =====
  u8g2Fonts.setFontMode(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

  u8g2Fonts.setFont(u8g2_font_logisoso42_tn);  // 大きな数字フォント

  // 温度を整数で表示（小数点なし）。右上の「°」と合わせて輪の中央に置く
  char temp_text[8];
  snprintf(temp_text, sizeof(temp_text), "%ld", lroundf(temperature));
  int16_t temp_w = u8g2Fonts.getUTF8Width(temp_text);
  int16_t temp_x = CM_CX - (temp_w + CM_DEGREE_GAP + 2 * CM_DEGREE_R + 1) / 2;
  u8g2Fonts.setCursor(temp_x, CM_TEMP_BASELINE);
  u8g2Fonts.print(temp_text);

  // 「°」はフォントに無いので、白い穴のあいた黒い丸で描く。上端を数字の上端にそろえる
  int16_t degree_x = temp_x + temp_w + CM_DEGREE_GAP + CM_DEGREE_R;
  int16_t degree_y = CM_TEMP_BASELINE - CM_TEMP_DIGIT_H + CM_DEGREE_R;
  display.fillCircle(degree_x, degree_y, CM_DEGREE_R, GxEPD_BLACK);
  display.fillCircle(degree_x, degree_y, CM_DEGREE_R - CM_DEGREE_PEN, GxEPD_WHITE);

  // 湿度を温度の下の中央に表示（100% でも輪の両端にかからない）
  if (aht21_enabled) {
    char humid_text[8];
    snprintf(humid_text, sizeof(humid_text), "%d%%", humidity);
    u8g2Fonts.setFont(u8g2_font_helvB18_tf);
    u8g2Fonts.setCursor(CM_CX - u8g2Fonts.getUTF8Width(humid_text) / 2, 120);
    u8g2Fonts.print(humid_text);
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
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

  // ===== 上半分：今の天気 =====
  // 1行目: 天気の名前（左）と取得した時刻（右）
  const char* label;
  WeatherKind kind = weatherKind(weather.code, &label);
  setU8g2FontTransparent(u8g2_font_helvB12_tf);
  u8g2Fonts.setCursor(4, 15);
  u8g2Fonts.print(label);

  if (weather.fetched_at != 0) {
    struct tm t;
    localtime_r(&weather.fetched_at, &t);
    char updated[16];
    strftime(updated, sizeof(updated), "Updated %H:%M", &t);
    setU8g2FontTransparent(u8g2_font_helvR08_tf);
    u8g2Fonts.setCursor(196 - u8g2Fonts.getUTF8Width(updated), 13);
    u8g2Fonts.print(updated);
  }

  drawWeatherIcon(48, 60, 13, kind, weather.is_day);

  // 気温（整数）と °C を右の列の中央に
  char text[20];
  snprintf(text, sizeof(text), "%ld", lroundf(weather.temp));
  setU8g2FontTransparent(u8g2_font_logisoso42_tn);
  int16_t temp_w = u8g2Fonts.getUTF8Width(text);
  setU8g2FontTransparent(u8g2_font_helvB12_tf);
  int16_t x = 144 - (temp_w + 2 + u8g2Fonts.getUTF8Width("°C")) / 2;
  setU8g2FontTransparent(u8g2_font_logisoso42_tn);
  u8g2Fonts.setCursor(x, 84);
  u8g2Fonts.print(text);
  setU8g2FontTransparent(u8g2_font_helvB12_tf);
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
      setU8g2FontTransparent(u8g2_font_helvB12_tf);
      printCentered(text, cx, 121);
    }

    const char* day_label;
    drawWeatherIcon(cx, 146, 8, weatherKind(day.code, &day_label), true);

    // 最高 / 最低気温
    snprintf(text, sizeof(text), "%ld° / %ld°", lroundf(day.temp_max), lroundf(day.temp_min));
    setU8g2FontTransparent(u8g2_font_helvB12_tf);
    printCentered(text, cx, 184);

    if (day.precip_prob >= 0) {
      snprintf(text, sizeof(text), "Rain %d%%", day.precip_prob);
      setU8g2FontTransparent(u8g2_font_helvR08_tf);
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
  u8g2Fonts.setForegroundColor(GxEPD_WHITE);
  setU8g2FontTransparent(u8g2_font_helvR08_tf);
  u8g2Fonts.setCursor(190 - u8g2Fonts.getUTF8Width("3h"), 168);
  u8g2Fonts.print("3h");

  char value[8];
  snprintf(value, sizeof(value), "%.1f", change);
  setU8g2FontTransparent(u8g2_font_helvB18_tf);
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
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  setU8g2FontTransparent(u8g2_font_helvB18_tf);
  char value[8];
  snprintf(value, sizeof(value), "%d", pressure);
  u8g2Fonts.setCursor(ISOBAR_CENTER_X - u8g2Fonts.getUTF8Width(value) / 2, 138);
  u8g2Fonts.print(value);

  // 3時間分の履歴がたまるまでは傾向のバッジを出さない
  if (!isnan(trend_3h)) {
    drawTrendBadge(trend_3h);
  }
}

// ============================================================================
// 画面11: Sound（レトロな VU メーター）
// ============================================================================
// 針が指すのは iPhone の騒音計アプリに合わせた騒音（dB。静かなときが 30。変換表は Custom_MAX9814.h）。
// 目盛りは 30〜80dB で 5dB ごと。70dB から右は黒い帯（うるさい）。
// 80dB を超えても針は 80 で止め、下の数字はそのまま出す。
// 下の帯には同じ数字を出す。絵は画像を持たずに、線と円で描く

#define VU_CX 100           // 針の軸の位置
#define VU_CY 148
#define VU_R 92             // 目盛りの弧の半径
#define VU_DB_MIN 30        // 目盛りの左端（dB。マイク基板のノイズに埋もれて、これより静かな音は測れない）
#define VU_DB_MAX 80        // 目盛りの右端（dB）
#define VU_LOUD_DB 70       // ここから右が黒い帯（うるさい）
#define VU_ANGLE_LEFT 140   // 30dB の向き（度。右が 0 で反時計回り）
#define VU_ANGLE_SPAN 100   // 30〜80dB の角度の幅（度）
#define VU_NEEDLE_R 84      // 針の長さ

// 軸から db の向きに r 離れた位置
static void vuPoint(float db, float r, float* x, float* y) {
  float a = (VU_ANGLE_LEFT - (db - VU_DB_MIN) * VU_ANGLE_SPAN / (VU_DB_MAX - VU_DB_MIN)) * DEG_TO_RAD;
  *x = VU_CX + r * cosf(a);
  *y = VU_CY - r * sinf(a);
}

// db0〜db1 の弧を、半径 pen_r の丸いペンでなぞる（1dB ごとの線分）
static void drawVUArc(int db0, int db1, int16_t pen_r) {
  float px, py;
  vuPoint(db0, VU_R, &px, &py);
  for (int db = db0 + 1; db <= db1; db++) {
    float x, y;
    vuPoint(db, VU_R, &x, &y);
    drawRoundLine(px, py, x, y, pen_r);
    px = x;
    py = y;
  }
}

void displaySound(const MAX9814Data& max9814_data, bool sensor_enabled) {
  if (!sensor_enabled) {
    display.setCursor(2, 56);
    display.println("MAX9814");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
    return;
  }

  // メーターの窓（角の丸い 3px の枠）
  for (int i = 0; i < 3; i++) {
    display.drawRoundRect(6 + i, 6 + i, 188 - 2 * i, 156 - 2 * i, 10 - i, GxEPD_BLACK);
  }

  // 目盛りの弧と、うるさい帯（太さ 9px）
  drawVUArc(VU_DB_MIN, VU_DB_MAX, 1);
  drawVUArc(VU_LOUD_DB, VU_DB_MAX, 4);

  // 目盛りの線（5dB ごと。10dB ごとは長くして dB の数字を添える）
  beginU8g2Fonts();
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  setU8g2FontTransparent(u8g2_font_helvR08_tf);
  for (int db = VU_DB_MIN; db <= VU_DB_MAX; db += 5) {
    bool major = db % 10 == 0;
    float x0, y0, x1, y1;
    vuPoint(db, VU_R, &x0, &y0);
    vuPoint(db, VU_R - (major ? 12 : 7), &x1, &y1);
    drawRoundLine(x0, y0, x1, y1, 1);
    if (major) {
      char label[4];
      snprintf(label, sizeof(label), "%d", db);
      float lx, ly;
      vuPoint(db, VU_R + 14, &lx, &ly);
      printCentered(label, lroundf(lx), lroundf(ly) + 4);  // 字の高さの半分下げて、中心を合わせる
    }
  }

  // 文字盤の単位（本物の VU メーターの「VU」の位置）。針はこの上を通る
  setU8g2FontTransparent(u8g2_font_helvB18_tf);
  printCentered("dB", VU_CX, 110);

  // 針と軸
  float db = max9814_data.db_phone;
  float needle_db = constrain(db, (float)VU_DB_MIN, (float)VU_DB_MAX);
  float nx, ny;
  vuPoint(needle_db, VU_NEEDLE_R, &nx, &ny);
  drawRoundLine(VU_CX, VU_CY, nx, ny, 1);
  display.fillCircle(VU_CX, VU_CY, 8, GxEPD_BLACK);

  // 下の帯: 数字（「60」と「dB」を合わせて真ん中に寄せる）
  char value[8];
  snprintf(value, sizeof(value), "%ld", lroundf(db));
  int16_t bx, by;
  uint16_t value_w, unit_w, bh;
  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(value, 0, 0, &bx, &by, &value_w, &bh);
  display.setFont(&FreeSans12pt7b);
  display.getTextBounds("dB", 0, 0, &bx, &by, &unit_w, &bh);
  display.setFont(&FreeSans18pt7b);
  display.setCursor(VU_CX - (value_w + 2 + unit_w) / 2, 194);
  display.print(value);
  display.setFont(&FreeSans12pt7b);
  display.setCursor(display.getCursorX() + 2, 194);
  display.print("dB");
}

// ============================================================================
// 画面12: Light（猫の目）
// ============================================================================
// 猫の瞳は、明るいと縦に細く、暗いとまんまるになる。その瞳の幅で照度を見せる。
// 瞳は 1〜10,000 lx の間で、桁ごとに同じだけ細くする。10,000 lx から上は、まぶしそうに目を細める。
// 10 lx 未満は画面を黒くして、目だけを白く出す。下に照度の数字。
// 絵は画像を持たずに、レンズ形（2つの円が重なった形）・円・楕円と線で描く

#define CAT_EYE_Y 62            // 目の中心の y（x は CAT_EYE_X）
#define CAT_EYE_A 40            // 目（白い部分）の幅の半分
#define CAT_EYE_B 32            // 目の高さの半分
#define CAT_EYE_EDGE 5          // 目の縁の太さ
#define CAT_IRIS_R 28           // 虹彩の輪（太さ 3px）の半径
#define CAT_PUPIL_R 24          // 瞳の縦の半径。いちばん暗いときは幅の半分もこれ（まんまる）
#define CAT_PUPIL_MIN 3         // いちばん明るいときの瞳の幅の半分
#define CAT_DARK_LUX 10         // これ未満は画面を黒くする
#define CAT_SLIT_LUX 10000      // 瞳がいちばん細くなる照度。ここから上は目を細める
#define CAT_SQUINT_LUX 30000    // 目をいちばん細める照度（今のセンサーの設定で測れる上限の目安）
#define CAT_SQUINT_MAX 0.55f    // 目をいちばん細めたときに閉じる割合
#define LUX_DIGIT_TOP 144       // 照度の数字の位置と大きさ
#define LUX_DIGIT_H 46
#define LUX_DIGIT_W 30          // 1桁の幅（桁が多くて収まらないときは細くする）
#define LUX_DIGIT_GAP 5

static const int16_t CAT_EYE_X[2] = {50, 150};

// 横の半径 a・縦の半径 b のレンズ形（上の弧と下の弧が、左右の端で交わる形）
// 上の弧は (0, d) を中心とする円、下の弧は (0, -d) を中心とする円で、どちらも半径の2乗は r2
// （半径を 0.5px 広げて、上下の端に 1px だけ飛び出す点ができないようにしている）
struct Lens {
  float d, r2;
};

static Lens makeLens(float a, float b) {
  float d = (a * a - b * b) / (2 * b);
  return {d, (d + b + 0.5f) * (d + b + 0.5f)};
}

static bool inLens(const Lens& lens, float dx, float dy) {
  return dx * dx + (dy - lens.d) * (dy - lens.d) <= lens.r2 && dx * dx + (dy + lens.d) * (dy + lens.d) <= lens.r2;
}

// 猫の目を1つ描く。pupil_w は瞳の幅の半分、open は目の開き具合（1 で全開）
// 目を細めたときに、まぶたの外へ虹彩や瞳がはみ出さないよう、1px ずつ色を決めて描く
static void drawCatEye(int16_t cx, float pupil_w, float open) {
  const int16_t a = CAT_EYE_A + CAT_EYE_EDGE, b = CAT_EYE_B + CAT_EYE_EDGE;
  const Lens outer = makeLens(a, b);
  const Lens inner = makeLens(CAT_EYE_A, CAT_EYE_B * open);
  // 瞳に映った光。瞳の右上に、はみ出さない大きさで置く（瞳が細いときは出さない）
  float shine_x = pupil_w * 0.45f, shine_r = fminf(5, pupil_w * 0.3f);
  if (shine_r < 2) {
    shine_r = 0;
  }
  for (int16_t dy = -b; dy <= b; dy++) {
    for (int16_t dx = -a; dx <= a; dx++) {
      if (!inLens(outer, dx, dy)) {
        continue;
      }
      bool white = inLens(inner, dx, dy);
      if (white) {
        // 瞳の楕円も 0.5px 広げて、細いときに真ん中の左右へ 1px 飛び出す点ができないようにする
        float px = dx / (pupil_w + 0.5f), py = dy / (CAT_PUPIL_R + 0.5f);
        bool pupil = px * px + py * py <= 1;
        bool shine = (dx - shine_x) * (dx - shine_x) + (dy + 10) * (dy + 10) <= shine_r * shine_r;
        bool ring = fabsf(sqrtf(dx * dx + dy * dy) - CAT_IRIS_R) <= 1.5f;
        white = !ring && (!pupil || shine);
      }
      display.drawPixel(cx + dx, CAT_EYE_Y + dy, white ? GxEPD_WHITE : GxEPD_BLACK);
    }
  }
}

// 鼻と ω の口と、左右2本ずつのひげ
static void drawCatMouth(uint16_t color) {
  display.fillTriangle(92, 104, 108, 104, 100, 112, color);
  drawRoundLine(100, 112, 100, 116, 1, color);
  drawCurve(100, 116, 95, 124, 88, 117, 2, color);
  drawCurve(100, 116, 105, 124, 112, 117, 2, color);
  drawRoundLine(80, 110, 36, 104, 1, color);
  drawRoundLine(80, 118, 38, 124, 1, color);
  drawRoundLine(120, 110, 164, 104, 1, color);
  drawRoundLine(120, 118, 162, 124, 1, color);
}

// 照度の数字（丸ゴシック風）と「lx」を合わせて、下の真ん中に寄せる
static void drawLuxValue(long lux, uint16_t color) {
  char digits[8];
  int n = snprintf(digits, sizeof(digits), "%ld", lux);
  int16_t bx, by;
  uint16_t unit_w, bh;
  display.setFont(&FreeSans12pt7b);
  display.getTextBounds("lx", 0, 0, &bx, &by, &unit_w, &bh);
  // 6桁（100,000 lx 以上）でも収まるよう、画面の幅（左右 4px あける）に入らなければ数字を細くする
  int16_t w = min(LUX_DIGIT_W, (192 - unit_w - 4 - (n - 1) * LUX_DIGIT_GAP) / n);
  int16_t r = max(2, (int)lroundf(w * 0.14f));  // ペンの半径（線の太さは 2r+1）
  int16_t x = 100 - (n * w + (n - 1) * LUX_DIGIT_GAP + 4 + unit_w) / 2;
  for (int i = 0; i < n; i++) {
    drawGlyph(digits[i] - '0', x, LUX_DIGIT_TOP, w, LUX_DIGIT_H, r, 0, color);
    x += w + LUX_DIGIT_GAP;
  }
  display.setTextColor(color);
  display.setCursor(x - LUX_DIGIT_GAP + 4 - bx, LUX_DIGIT_TOP + LUX_DIGIT_H);
  display.print("lx");
  display.setTextColor(GxEPD_BLACK);
}

void displayLight(const VEML7700Data& veml7700_data, bool sensor_enabled) {
  if (!sensor_enabled) {
    display.setCursor(2, 56);
    display.println("VEML7700");
    display.setCursor(2, 88);
    display.println("Not");
    display.setCursor(2, 120);
    display.println("Connected");
    return;
  }

  float lux = fminf(fmaxf(veml7700_data.lux, 0.0f), 999999.0f);  // 読めなかったとき（NaN）は 0
  long shown = lroundf(lux);  // 出す数字。黒い画面にするかも、この数字で決める（9.6 lx が黒い画面で 10 と出ないように）
  bool dark = shown < CAT_DARK_LUX;
  uint16_t color = dark ? GxEPD_WHITE : GxEPD_BLACK;
  if (dark) {
    display.fillScreen(GxEPD_BLACK);
  }

  // 瞳の幅: 1〜10,000 lx で、桁ごとに同じだけ細くする
  float lit = log10f(fmaxf(lux, 1.0f));
  float t = constrain(lit / log10f(CAT_SLIT_LUX), 0.0f, 1.0f);
  float pupil_w = CAT_PUPIL_R - (CAT_PUPIL_R - CAT_PUPIL_MIN) * t;
  // まぶしさ: 10,000〜30,000 lx で、目を細める
  float s = constrain((lit - log10f(CAT_SLIT_LUX)) / log10f((float)CAT_SQUINT_LUX / CAT_SLIT_LUX), 0.0f, 1.0f);
  float open = 1 - CAT_SQUINT_MAX * s;
  for (int i = 0; i < 2; i++) {
    drawCatEye(CAT_EYE_X[i], pupil_w, open);
  }

  drawCatMouth(color);
  drawLuxValue(shown, color);
}
