#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// ESP32-C3 Super Mini (K2) pinout for WeAct 1.54" e-paper (200x200 SSD1681)
#define EPD_CLK   4   // GPIO4  (SPI Clock / SCL)
#define EPD_MOSI  6   // GPIO6  (SPI Data / SDA)
#define EPD_CS    7   // GPIO7  (Chip Select / CS)
#define EPD_DC    1   // GPIO1  (Data/Command / DC)
#define EPD_RST   2   // GPIO2  (Reset / RES)
#define EPD_BUSY  5   // GPIO5  (Busy Signal / BUSY)

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
  GxEPD2_154_D67(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY)
);

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// 画面切り替え用変数
static unsigned long last_update = 0;
static uint8_t display_mode = 0;  // 0: 日本語テスト, 1: 温度/湿度画面
static bool first_display = true;  // 初回表示フラグ

void setup() {
  Serial.begin(115200);
  delay(200);

  SPI.begin(EPD_CLK, /*miso=*/ -1, EPD_MOSI, EPD_CS);
  display.init(115200, true, 2, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);

  u8g2Fonts.begin(display);
}

void loop() {
  unsigned long current_time = millis();
  
  // 10秒ごとに画面更新 & 切り替え（初回は即座に表示）
  if (current_time - last_update >= 10000 || first_display) {
    last_update = current_time;
    
    Serial.print("\n📺 画面: Screen ");
    Serial.print(display_mode + 1);
    Serial.println("/2");
    
    // ===================================================================
    // 初回はフルリフレッシュ、その後は部分リフレッシュで高速化
    // ===================================================================
    if (first_display) {
      display.setFullWindow();
      first_display = false;
      Serial.println("🔄 初回フルリフレッシュ実行");
    } else {
      // 通常は部分リフレッシュ（高速）
      display.setPartialWindow(0, 0, display.width(), display.height());
    }
    
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      // Ensure normal (non-inverted) text rendering
      u8g2Fonts.setFontMode(0);
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

      if (display_mode == 0) {
        // ===== 画面1: 日本語フォントテスト =====
        u8g2Fonts.setFont(u8g2_font_helvB14_tf);
        u8g2Fonts.setCursor(2, 18);
        u8g2Fonts.print("U8g2 JP b/f 16px");

        display.drawLine(0, 26, 199, 26, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_b16_t_japanese3);
        u8g2Fonts.setCursor(2, 48);
        u8g2Fonts.print("日本語テストです b16");

        u8g2Fonts.setFont(u8g2_font_b16_b_t_japanese3);
        u8g2Fonts.setCursor(2, 72);
        u8g2Fonts.print("日本語テストです b16_b");

        u8g2Fonts.setFont(u8g2_font_b16_b_t_japanese3);
        u8g2Fonts.setCursor(2, 96);
        u8g2Fonts.print("日本語テストです f16");

        u8g2Fonts.setFont(u8g2_font_b16_b_t_japanese3);
        u8g2Fonts.setCursor(2, 120);
        u8g2Fonts.print("日本語テストです f16_b");
      } else {
        // ===== 画面2: 温度/湿度表示（モダンデザイン） =====
        // ダミーデータ（後でセンサー値に置き換え）
        int temperature = 25;
        int humidity = 60;
        
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
        
        // ===== 中央の大きな温度表示 =====
        u8g2Fonts.setFont(u8g2_font_logisoso42_tn);  // 大きな数字フォント
        
        // 温度を中央に表示
        u8g2Fonts.setCursor(73, 90);
        u8g2Fonts.print(temperature);
                
        // 湿度を右下に表示
        u8g2Fonts.setFont(u8g2_font_helvB18_tf);
        u8g2Fonts.setCursor(80, 120);
        u8g2Fonts.print(humidity);
        u8g2Fonts.print("%");
        
        // ===== 下部：過去12時間のグラフ =====
        int16_t graphY = 135;
        int16_t graphHeight = 25;
        int16_t graphWidth = 180;
        int16_t graphX = 10;
        
        // "PAST 12H TEMP" ラベル
        u8g2Fonts.setFont(u8g2_font_helvR08_tf);
        u8g2Fonts.setCursor(graphX, graphY - 5);
        u8g2Fonts.print("TEMP");
        
        // グリッド線（横）
        for (int i = 0; i <= 2; i++) {
          int16_t y = graphY + i * (graphHeight / 2);
          for (int x = graphX; x < graphX + graphWidth; x += 4) {
            display.drawPixel(x, y, GxEPD_BLACK);
          }
        }
        
        // ダミーの温度グラフ（波形）
        for (int x = 0; x < graphWidth - 1; x++) {
          float angle = (x / (float)graphWidth) * 2 * PI;
          int16_t y1 = graphY + graphHeight/2 - sin(angle) * (graphHeight / 3);
          int16_t y2 = graphY + graphHeight/2 - sin((x+1) / (float)graphWidth * 2 * PI) * (graphHeight / 3);
          display.drawLine(graphX + x, y1, graphX + x + 1, y2, GxEPD_BLACK);
        }
        
        // "PAST 12H HUMIDITY" ラベル
        graphY = 175;
        u8g2Fonts.setCursor(graphX, graphY - 5);
        u8g2Fonts.print("HUMIDITY");
        
        // グリッド線（横）
        for (int i = 0; i <= 2; i++) {
          int16_t y = graphY + i * (graphHeight / 2);
          for (int x = graphX; x < graphX + graphWidth; x += 4) {
            display.drawPixel(x, y, GxEPD_BLACK);
          }
        }
        
        // ダミーの湿度グラフ（波形）
        for (int x = 0; x < graphWidth - 1; x++) {
          float angle = (x / (float)graphWidth) * PI + PI;
          int16_t y1 = graphY + graphHeight/2 - sin(angle) * (graphHeight / 4);
          int16_t y2 = graphY + graphHeight/2 - sin((x+1) / (float)graphWidth * PI + PI) * (graphHeight / 4);
          display.drawLine(graphX + x, y1, graphX + x + 1, y2, GxEPD_BLACK);
        }
      }
    } while (display.nextPage());
    
    // 画面モードを切り替え（0 ⇔ 1）
    display_mode = (display_mode + 1) % 2;
    
    // リフレッシュ完了待機
    delay(100);
  }
  
  // CPU過負荷防止
  delay(100);
}

