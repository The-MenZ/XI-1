# 📝 利用可能なフォント一覧

## 📌 概要

このプロジェクトでは 2 種類のフォントが使えます。

- **Adafruit GFX Library のフォント**（`Fonts/` フォルダの 52 種類）: `display.setFont(&...)` で使用。文字は ASCII（0x20～0x7E）のみ
- **U8g2 のフォント**: `U8g2_for_Adafruit_GFX` ライブラリ経由で GxEPD2 の `display` に描画できます（Climate Modern・Pressure 画面で使用中）。詳しくは下の「🔤 U8g2 フォント」を参照

---

## ✅ このプロジェクトで実際に使っているフォント

| フォント | 種類 | 使用箇所 |
|---------|------|---------|
| `FreeMonoBold12pt7b` | Adafruit GFX | 全画面共通の既定フォント（`main.cpp` の `loop()` で設定）。Air Quality / Climate / Ambience / Gas Sensor 画面、Sound・Light の未接続時の表示、起動時のテスト表示（`main.cpp` の `setup()`） |
| `FreeSans12pt7b` | Adafruit GFX | Settings QR・Weather の未接続／未取得時の表示、Sound の「dB」、Light の「lx」 |
| `FreeSans18pt7b` | Adafruit GFX | Sound の補正後の数字 |
| `FreeSans9pt7b` | Adafruit GFX | WiFi QR のエラー表示、Weather の未取得時の表示、Pressure の等圧線の気圧 |
| `u8g2_font_logisoso42_tn` | U8g2 | Climate Modern の温度、Weather の今の気温（大きな数字） |
| `u8g2_font_helvB18_tf` | U8g2 | Climate Modern の湿度、Pressure の気圧と3時間の変化、Sound の文字盤の「dB」 |
| `u8g2_font_helvB12_tf` | U8g2 | Weather の天気の名前・°C・日付・最高/最低気温 |
| `u8g2_font_helvR08_tf` | U8g2 | Weather の取得時刻・降水確率、Pressure の「3h」、Sound の目盛りの数字 |

※ Month Day 画面の数字はフォントを使わず、`display_screens.cpp` の中で直線と楕円弧を丸いペンでなぞって描いています（`MONTHDAY_SCREEN.md` 参照）。Air Quality の AQI、Time のパタパタ時計、Light の照度の数字も、同じ描き方です。
※ `main.cpp` は `FreeMonoBold12pt7b` だけを読み込み、ほかのフォントは `display_screens.cpp` で読み込みます（下の注意事項を参照）。

---

## 🎨 フォント一覧

### FreeSans フォントファミリー

| フォント | サイズ | スタイル | 用途 |
|---------|--------|---------|------|
| `FreeSans9pt7b` | 9pt | 通常 | 小さいテキスト |
| `FreeSans12pt7b` | 12pt | 通常 | **標準サイズ** |
| `FreeSans18pt7b` | 18pt | 通常 | 中程度の見出し |
| `FreeSans24pt7b` | 24pt | 通常 | 大きい見出し |
| `FreeSansBold9pt7b` | 9pt | **太字** | 強調小サイズ |
| `FreeSansBold12pt7b` | 12pt | **太字** | 強調標準 |
| `FreeSansBold18pt7b` | 18pt | **太字** | 強調中見出し |
| `FreeSansBold24pt7b` | 24pt | **太字** | 強調大見出し |
| `FreeSansOblique9pt7b` | 9pt | *斜体* | 小斜体 |
| `FreeSansOblique12pt7b` | 12pt | *斜体* | 標準斜体 |
| `FreeSansOblique18pt7b` | 18pt | *斜体* | 中斜体 |
| `FreeSansOblique24pt7b` | 24pt | *斜体* | 大斜体 |
| `FreeSansBoldOblique9pt7b` | 9pt | ***太字斜体*** | 小太字斜体 |
| `FreeSansBoldOblique12pt7b` | 12pt | ***太字斜体*** | 標準太字斜体 |
| `FreeSansBoldOblique18pt7b` | 18pt | ***太字斜体*** | 中太字斜体 |
| `FreeSansBoldOblique24pt7b` | 24pt | ***太字斜体*** | 大太字斜体 |

---

### FreeMono フォントファミリー（等幅）

| フォント | サイズ | スタイル | 用途 |
|---------|--------|---------|------|
| `FreeMono9pt7b` | 9pt | 通常 | コード・数値（小） |
| `FreeMono12pt7b` | 12pt | 通常 | **コード・数値標準** |
| `FreeMono18pt7b` | 18pt | 通常 | コード・数値（中） |
| `FreeMono24pt7b` | 24pt | 通常 | コード・数値（大） |
| `FreeMonoBold9pt7b` | 9pt | **太字** | 強調コード小 |
| `FreeMonoBold12pt7b` | 12pt | **太字** | 強調コード標準 |
| `FreeMonoBold18pt7b` | 18pt | **太字** | 強調コード中 |
| `FreeMonoBold24pt7b` | 24pt | **太字** | 強調コード大 |
| `FreeMonoOblique9pt7b` | 9pt | *斜体* | コード小斜体 |
| `FreeMonoOblique12pt7b` | 12pt | *斜体* | コード標準斜体 |
| `FreeMonoOblique18pt7b` | 18pt | *斜体* | コード中斜体 |
| `FreeMonoOblique24pt7b` | 24pt | *斜体* | コード大斜体 |
| `FreeMonoBoldOblique9pt7b` | 9pt | ***太字斜体*** | コード太字斜体小 |
| `FreeMonoBoldOblique12pt7b` | 12pt | ***太字斜体*** | コード太字斜体標準 |
| `FreeMonoBoldOblique18pt7b` | 18pt | ***太字斜体*** | コード太字斜体中 |
| `FreeMonoBoldOblique24pt7b` | 24pt | ***太字斜体*** | コード太字斜体大 |

---

### FreeSerif フォントファミリー（セリフ体）

| フォント | サイズ | スタイル | 用途 |
|---------|--------|---------|------|
| `FreeSerif9pt7b` | 9pt | 通常 | 本文テキスト（小） |
| `FreeSerif12pt7b` | 12pt | 通常 | **本文テキスト標準** |
| `FreeSerif18pt7b` | 18pt | 通常 | 本文テキスト（大） |
| `FreeSerif24pt7b` | 24pt | 通常 | 本文テキスト（特大） |
| `FreeSerifBold9pt7b` | 9pt | **太字** | 太字本文小 |
| `FreeSerifBold12pt7b` | 12pt | **太字** | 太字本文標準 |
| `FreeSerifBold18pt7b` | 18pt | **太字** | 太字本文中 |
| `FreeSerifBold24pt7b` | 24pt | **太字** | 太字本文大 |
| `FreeSerifItalic9pt7b` | 9pt | *斜体* | イタリック本文小 |
| `FreeSerifItalic12pt7b` | 12pt | *斜体* | イタリック本文標準 |
| `FreeSerifItalic18pt7b` | 18pt | *斜体* | イタリック本文中 |
| `FreeSerifItalic24pt7b` | 24pt | *斜体* | イタリック本文大 |
| `FreeSerifBoldItalic9pt7b` | 9pt | ***太字斜体*** | 太字イタリック小 |
| `FreeSerifBoldItalic12pt7b` | 12pt | ***太字斜体*** | 太字イタリック標準 |
| `FreeSerifBoldItalic18pt7b` | 18pt | ***太字斜体*** | 太字イタリック中 |
| `FreeSerifBoldItalic24pt7b` | 24pt | ***太字斜体*** | 太字イタリック大 |

---

### スペシャルフォント

| フォント | 説明 | 用途 |
|---------|------|------|
| `Picopixel` | 超小型ピクセルフォント | 極小テキスト、ミニマルデザイン |
| `TomThumb` | 3x5 ピクセルミニフォント | ラベル、計器表示 |
| `Tiny3x3a2pt7b` | 3x3 ピクセルフォント | 最小限のテキスト |
| `Org_01` | オリジナルスタイルフォント | ユニークなデザイン |

---

## 💡 使用方法

### ヘッダーファイルのインクルード

```cpp
// 使いたいフォントをインクルード
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
```

### フォントの設定と使用

```cpp
// フォントを設定
display.setFont(&FreeSans12pt7b);

// テキストを表示
display.setCursor(10, 50);
display.println("Hello World");

// 別のフォントに切り替え
display.setFont(&FreeMonoBold18pt7b);
display.setCursor(10, 100);
display.println("Bold Text");

// フォントをリセット（デフォルトに戻す）
display.setFont(NULL);
display.println("Default Font");
```

- GFX フォントを設定すると、`setCursor(x, y)` の y は**文字のベースライン**になります（既定フォントでは左上）
- 表示できるのは ASCII（0x20～0x7E）のみです。`°` や日本語は表示できないので、必要なら U8g2 フォントを使います

---

## 🔤 U8g2 フォント（U8g2_for_Adafruit_GFX 経由）

`U8g2_for_Adafruit_GFX`（`platformio.ini` の `lib_deps` に追加済み）を使うと、GxEPD2 の `display` に U8g2 のフォントで描画できます。フォントデータはこのライブラリに同梱されています。Climate Modern 画面（`display_screens.cpp` の `displayClimateModern()`）での使い方:

```cpp
#include <U8g2_for_Adafruit_GFX.h>

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
u8g2Fonts.begin(display);                   // GxEPD2 の display に接続（初回のみ）

u8g2Fonts.setFontMode(0);                   // 0: 背景色も塗る / 1: 透過
u8g2Fonts.setForegroundColor(GxEPD_BLACK);
u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

u8g2Fonts.setFont(u8g2_font_logisoso42_tn); // 大きな数字
u8g2Fonts.setCursor(73, 90);                // y はベースライン
u8g2Fonts.print(lroundf(temperature));      // 四捨五入して整数で表示

u8g2Fonts.setFont(u8g2_font_helvB18_tf);
u8g2Fonts.setCursor(80, 120);
u8g2Fonts.print(humidity);
u8g2Fonts.print("%");
```

| U8g2 フォント | 内容 | 用途（このプロジェクト） |
|--------------|------|----------------------|
| `u8g2_font_logisoso42_tn` | 高さ 42px の数字（`_tn` = 数字と一部記号のみ） | 温度の大きな数字 |
| `u8g2_font_helvB18_tf` | Helvetica Bold 18（`_tf` = ASCII＋Latin-1） | 湿度 |
| `u8g2_font_helvB12_tf` | Helvetica Bold 12（`_tf`） | 天気画面の文字（`°` を含む） |
| `u8g2_font_helvR08_tf` | Helvetica Regular 8（`_tf`） | 天気・気圧・騒音画面の小さい文字 |

- **`setFont()` はフォントが変わるたびに `setFontMode(0)`（背景を塗る）に戻します。** 背景を塗らない画面では、`display_screens.cpp` の `setU8g2FontTransparent()` でフォントを変えます（`setFont()` の後に `setFontMode(1)` を呼ぶ）。`setFontMode(1)` を `setFont()` より前に呼ぶと効かず、背景の色を決めていなければ初期値の 0 で塗られます。e-Paper では 0 が黒なので、文字の枠が黒い四角になります（Sound 画面の「dB」が黒い四角2つになっていた。Weather と Pressure も同じ書き方だった）
- 同じ名前のフォントは U8g2 ライブラリ（`olikraus/U8g2`）にもあり、実際にリンクされるのは U8g2 側のデータです（`firmware.map` で確認）。どちらのデータでも表示は同じです
- `print()` は UTF-8 を解釈するので、`_tf` フォントなら `°` なども表示できます
- 日本語フォント（`u8g2_font_b16_t_japanese3` など）もあります。表示テストは `src/u8g2_gfx_font_test.cpp`（`pio run -e esp32-c3-super-mini-k2-u8g2gfx -t upload`）
- フォント一覧: [U8g2 Font List](https://github.com/olikraus/u8g2/wiki/fntlistall)、方式の比較は [FONT_SYSTEM_ALTERNATIVES.md](./FONT_SYSTEM_ALTERNATIVES.md)

---

## 📊 推奨される組み合わせ

### 📱 最小限のデザイン
```cpp
display.setFont(&Picopixel);      // 超小型テキスト
display.println("Minimal");
```

### 📊 センサー表示画面
```cpp
display.setFont(&FreeMonoBold12pt7b);  // 数値は等幅太字
display.println("25.5C");              // ° は GFX フォントに無いので使わない
```

### 📰 見出しと本文
```cpp
display.setFont(&FreeSans24pt7b);      // 見出し
display.println("TITLE");

display.setFont(&FreeSans9pt7b);       // 本文
display.println("This is body text");
```

### 🎨 洒落たデザイン
```cpp
display.setFont(&FreeSerif18pt7b);     // セリフ体見出し
display.println("Elegant");

display.setFont(&FreeSansOblique12pt7b);  // 斜体本文
display.println("Stylish");
```

---

## 📐 フォントサイズ比較

フォントの行の高さ（`yAdvance`、`println()` で改行したときに進む高さ）です。

| フォント | 行の高さ | 200px に入る行数の目安 |
|---------|---------|---------------------|
| `TomThumb` | 6px | 33 行 |
| `Picopixel` | 7px | 28 行 |
| `FreeSans9pt7b` | 22px | 9 行 |
| `FreeMonoBold12pt7b` | 24px（1文字の幅 14px → 1行 約14文字） | 8 行 |
| `FreeSans12pt7b` | 29px | 6 行 |
| `FreeSans18pt7b` | 42px | 4 行 |
| `FreeMonoBold24pt7b` | 47px（1文字の幅 28px） | 4 行 |
| `FreeSans24pt7b` | 56px | 3 行 |

---

## 🎯 200x200 e-paperでの推奨設定

| 用途 | フォント | 理由 |
|------|---------|------|
| タイトル | `FreeSans18pt7b` / `FreeSansBold18pt7b` | 見出しとしてちょうどいいサイズ |
| サブタイトル | `FreeSans12pt7b` / `FreeSansBold12pt7b` | 標準サイズで読みやすい |
| ラベル | `FreeSans9pt7b` | コンパクトで情報量多い |
| 数値 | `FreeMonoBold12pt7b` | 等幅で桁揃え、太字で目立つ |
| 強調 | `FreeSansBold24pt7b` | 大きく目立つ |
| 大きな数字 | `u8g2_font_logisoso42_tn`（U8g2） | Climate Modern の温度表示で使用 |
| ミニマル | `Picopixel` / `TomThumb` | スペース節約 |

---

## ✅ 実装例

### 例1: エレガントな見出し
```cpp
#include <Fonts/FreeSerif18pt7b.h>

display.setFont(&FreeSerif18pt7b);
display.setCursor(20, 50);
display.println("11");

display.setFont(&FreeSerif18pt7b);
display.setCursor(20, 120);
display.println("29");
```

### 例2: センサー表示
```cpp
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

display.setFont(&FreeSansBold12pt7b);
display.println("Temperature:");

display.setFont(&FreeMonoBold12pt7b);
display.println("25.5C");
```

### 例3: 複合デザイン
```cpp
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

display.setFont(&FreeSans24pt7b);
display.println("Data");

display.setFont(&FreeSans12pt7b);
display.println("Current Reading:");

display.setFont(&FreeMono9pt7b);
display.println("Temp: 22.3C");
display.println("Humidity: 65%");
```

---

## 📝 フォント選択ガイド

### どのフォントを選ぶ？

1. **読みやすさ重視**
   - → `FreeSans12pt7b` または `FreeSans18pt7b`

2. **数値・コード表示**
   - → `FreeMonoBold12pt7b` または `FreeMonoBold18pt7b`

3. **上品・エレガント**
   - → `FreeSerif18pt7b` または `FreeSerifItalic12pt7b`

4. **スペース節約**
   - → `FreeSans9pt7b` または `Picopixel`

5. **目立たせたい**
   - → `FreeSansBold24pt7b` または `FreeSansBoldOblique24pt7b`

6. **ミニマルなデザイン**
   - → `TomThumb` または `Tiny3x3a2pt7b`

---

## ⚠️ 注意事項

- フォントデータは **フラッシュ（PROGMEM）** に置かれ、RAM はほとんど使いません
- ファームウェアに入るのは `setFont()` で実際に参照したフォントだけです。インクルードしただけで使っていないフォントはビルド時に取り除かれます
- 大きいフォント（24pt など）ほどデータが大きくなります
- 同じ GFX フォントを複数の `.cpp` で使うと、ファイルごとにコピーが入ります。そのため `main.cpp` では全画面共通の `FreeMonoBold12pt7b` だけを使い、`display_screens.cpp` ではこのフォントを読み込みません（Gas Sensor 画面のように使う画面は、`main.cpp` で設定済みのものをそのまま使う。以前は `FreeSans12pt7b` が両方に入っていて、約 2.7KB 余分でした）
- U8g2 の日本語フォントは大きい（例: `u8g2_font_unifont_t_japanese1` で約 47KB）ので、必要なものだけ使ってください

---

## 🔮 フォントメモリ最適化のコツ

```cpp
// ✅ 使うフォントだけインクルードする（読みやすさのため）
#include <Fonts/FreeSans18pt7b.h>      // 見出し用
#include <Fonts/FreeMonoBold12pt7b.h>  // 数値用

// ✅ フォントを使う処理は 1 つの .cpp にまとめる（コピーの重複を防ぐ）
```

---

## 📚 参考資料

- [Adafruit GFX Library Documentation](https://learn.adafruit.com/adafruit-gfx-graphics-library)
- [GxEPD2 Display Library](https://github.com/ZinggJM/GxEPD2)
- [Font Reference](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts)
- [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)
- [U8g2 Font List](https://github.com/olikraus/u8g2/wiki/fntlistall)

