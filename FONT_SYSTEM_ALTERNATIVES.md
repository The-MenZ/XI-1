# 🎨 フォントシステムの代替案

## ✅ 現在の状態（最新）

このドキュメントの後半（導入前の検討）では「U8g2 フォントは GxEPD2 で直接使えない／変換が必要」と結論していましたが、現在は **[U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)**（olikraus 作）を使い、**フォント変換なしで U8g2 フォントを GxEPD2 の画面に描画**しています。GxEPD2 の `display` は Adafruit GFX の派生クラスなので、そのまま渡せます。

- **ライブラリ**: `platformio.ini` の `lib_deps` に `https://github.com/olikraus/U8g2_for_Adafruit_GFX.git`（`olikraus/U8g2` も記載あり）
- **本番コード**: `src/display_screens.cpp` の `displayClimateModern()`（画面7: Climate Modern）で `u8g2_font_logisoso42_tn` / `u8g2_font_helvB18_tf` / `u8g2_font_helvR08_tf`、`displayWeather()`（画面9: Weather）で `u8g2_font_logisoso42_tn` / `u8g2_font_helvB12_tf` / `u8g2_font_helvR08_tf`、`displayPressure()`（画面10: Pressure）で `u8g2_font_helvB18_tf` / `u8g2_font_helvR08_tf` を使用
- **テストスケッチ**: `src/u8g2_gfx_font_test.cpp`（日本語フォント `u8g2_font_b16_t_japanese3` / `u8g2_font_b16_b_t_japanese3` の表示テストと、Climate Modern の試作画面）。専用環境 `esp32-c3-super-mini-k2-u8g2gfx` でビルドし、メイン環境 `esp32-c3-super-mini-k2` では `build_src_filter` で除外
- **その他の画面**: 従来どおり Adafruit GFX フォント（`FreeMonoBold12pt7b` など）

```cpp
#include <U8g2_for_Adafruit_GFX.h>

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
u8g2Fonts.begin(display);                  // GxEPD2 の display を渡す（一度だけ）

// display.firstPage() 〜 nextPage() のループ内で
u8g2Fonts.setFontMode(0);
u8g2Fonts.setForegroundColor(GxEPD_BLACK);
u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
u8g2Fonts.setFont(u8g2_font_helvB18_tf);   // U8g2 フォントをそのまま指定
u8g2Fonts.setCursor(80, 120);
u8g2Fonts.print("45%");
```

```bash
# テストスケッチの書き込み
pio run -e esp32-c3-super-mini-k2-u8g2gfx -t upload
```

> 📝 以下は U8g2_for_Adafruit_GFX 導入前の検討メモ（履歴）です。「変換が必要」「直接統合できない」という記述は、U8g2lib のディスプレイクラス（`U8G2_...`）を GxEPD2 と組み合わせる場合の話として読んでください。

---

## 📌 概要

導入前の時点では **Adafruit GFX Library** のフォント（FreeSans、FreeMono、FreeSerifなど）のみを使用しており、より多くのフォント選択肢が必要な場合の代替案をまとめました。

---

## 🔍 GxEPD2とU8g2の関係性について

### 重要な理解：これらは**異なるライブラリ**です

#### GxEPD2とは？

```
GxEPD2 (e-paperディスプレイドライバ)
    ↓ 継承
Adafruit GFX Library (グラフィック描画ライブラリ)
    ↓ 使用
Adafruit GFX Fonts (フォントシステム)
```

**GxEPD2の特徴:**
- **e-paperディスプレイ専用**のドライバライブラリ
- **Adafruit GFX Libraryを継承**している
- つまり、GxEPD2は`Adafruit_GFX`クラスの子クラス
- `display.setFont()`, `display.println()` などのメソッドは**Adafruit GFX**から継承
- 現在使用しているフォント（`FreeSans12pt7b`など）は**Adafruit GFX形式**

**現在のコード構造:**
```cpp
// GxEPD2はAdafruit GFXを継承しているため、
// Adafruit GFXのフォントを直接使用できる
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans12pt7b.h>  // Adafruit GFX形式のフォント

GxEPD2_BW<...> display(...);  // GxEPD2オブジェクト

display.setFont(&FreeSans12pt7b);  // Adafruit GFXのメソッド
display.println("Hello");           // Adafruit GFXのメソッド
```

---

#### U8g2とは？

```
U8g2lib (独立したグラフィックライブラリ)
    ↓ 独自の
U8g2 Font System (独自のフォントシステム)
```

**U8g2の特徴:**
- **独立したグラフィックライブラリ**（Adafruit GFXとは別物）
- OLED/LCD/e-paperなど**様々なディスプレイ**をサポート
- **独自のフォントシステム**を持っている
- 200種類以上のフォントが利用可能
- 日本語フォントもサポート

**U8g2の使用例:**
```cpp
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(...);  // U8g2オブジェクト

u8g2.setFont(u8g2_font_6x10_tf);  // U8g2形式のフォント
u8g2.drawStr(0, 20, "Hello");      // U8g2のメソッド
```

---

### ❌ 重要な問題：直接統合はできない

**GxEPD2とU8g2は異なるアーキテクチャのため、直接統合できません：**

1. **フォント形式が異なる**
   - GxEPD2: Adafruit GFX形式（`GFXfont`構造体）
   - U8g2: U8g2形式（`u8g2_font_t`構造体）

2. **APIが異なる**
   - GxEPD2: `display.setFont(&font)`, `display.println()`
   - U8g2: `u8g2.setFont(font)`, `u8g2.drawStr()`

3. **内部実装が異なる**
   - フォントデータの構造、レンダリング方法が全く違う

---

### ✅ U8g2フォントをGxEPD2で使用する方法

> ✅ **現在は U8g2_for_Adafruit_GFX を使う方法を採用**しており、変換は不要です（冒頭「現在の状態」参照）。以下の方法1・2は導入前に検討した案です。

当時は、U8g2のフォントをGxEPD2で使用するには **フォントデータの変換**が必要だと考えていました：

#### 方法1: フォント変換ツールを使用

1. **U8g2フォントをAdafruit GFX形式に変換**
   - オンラインツールやスクリプトを使用
   - フォントデータの構造を変換

2. **変換されたフォントをプロジェクトに追加**
   ```cpp
   #include "converted_font.h"  // 変換されたフォント
   
   display.setFont(&converted_font);  // 通常通り使用
   ```

#### 方法2: U8g2のレンダリング機能を間接的に使用

1. **U8g2でテキストをレンダリング**（メモリバッファに）
2. **レンダリング結果をビットマップとして取得**
3. **GxEPD2でビットマップを表示**

```cpp
// 概念的なコード（実装は複雑）
U8g2lib u8g2(...);
u8g2.setFont(u8g2_font_6x10_tf);
u8g2.drawStr(0, 20, "Hello");

// ビットマップを取得してGxEPD2に描画
// （実装は複雑で、メモリ効率も悪い）
```

**この方法のデメリット:**
- ⚠️ 実装が複雑
- ⚠️ メモリ使用量が増える
- ⚠️ パフォーマンスが低下する可能性

---

### 📊 まとめ：GxEPD2とU8g2の関係

| 項目 | GxEPD2 | U8g2 |
|------|--------|------|
| **目的** | e-paperディスプレイドライバ | 汎用グラフィックライブラリ |
| **継承元** | Adafruit GFX Library | 独立（独自実装） |
| **フォント形式** | Adafruit GFX形式 | U8g2形式 |
| **フォント数** | ~50種類 | 200+種類 |
| **直接統合** | ❌ 不可能 | ❌ 不可能 |
| **フォント変換** | ✅ 可能（ツール使用） | ✅ 可能（ツール使用） |

---

### 💡 実用的な推奨事項

**当時の状況:**（現在は U8g2_for_Adafruit_GFX 経由で U8g2 フォントを使用中）
- ✅ GxEPD2は既に動作している
- ✅ Adafruit GFXフォントも使用可能
- ⚠️ U8g2はインストールされているが未使用

**推奨アプローチ:**

1. **最も簡単**: Adafruit GFXフォントを継続使用
   - 既に動作している
   - 追加の作業不要

2. **より多くのフォントが必要**: フォント変換ツールを使用
   - U8g2フォントをAdafruit GFX形式に変換
   - 変換されたフォントをプロジェクトに追加

3. **完全にU8g2を使用**: ディスプレイライブラリをU8g2に変更
   - GxEPD2からU8g2に移行（大きな変更）
   - 既存コードの大幅な書き換えが必要

---

## 🎯 推奨フォントシステム候補

### 1. **U8g2lib** ⭐️ 現在は U8g2_for_Adafruit_GFX 経由で使用中（当時は「フォント変換で使用可能」と評価）

**特徴:**
- **200種類以上のフォント**が利用可能
- 既に `platformio.ini` にインストール済み（`olikraus/U8g2@^2.35.17`）
- 日本語フォントもサポート
- メモリ効率が良い
- **フォント変換ツールを使用すればGxEPD2で使用可能**

**利用可能なフォント例:**
- `u8g2_font_6x10_tf` - 小さなテキスト用
- `u8g2_font_helvB08_tf` - Helvetica Bold
- `u8g2_font_ncenB08_tr` - New Century Schoolbook
- `u8g2_font_timR08_tr` - Times Roman
- `u8g2_font_courB08_tr` - Courier Bold
- `u8g2_font_unifont_t_japanese1` - 日本語フォント
- その他多数...

**日本語フォント（japanese1/2/3）の違い:**
- **見た目のデザイン差ではなく、収録文字の範囲**が違います
- `japanese1`: 学習レベル 1-6 相当の文字集合
- `japanese2`: 学習レベル 1-7 相当の文字集合
- `japanese3`: 上記に加えて **半角形などのフォーム**を含む拡張セット

**⚠️ 重要な注意:**（当時の見解。現在は U8g2_for_Adafruit_GFX で変換なしに使用中）
- U8g2は**独立したライブラリ**で、GxEPD2とは**直接統合できません**
- U8g2フォントをGxEPD2で使用するには、**フォントデータの変換**が必要です
- 変換後は、Adafruit GFX形式のフォントとして使用できます

**実装方法（フォント変換後）:**
```cpp
// 1. U8g2フォントをAdafruit GFX形式に変換（ツール使用）
// 2. 変換されたフォントをインクルード
#include "fonts/converted_u8g2_font_6x10.h"

// 3. 通常通りGxEPD2で使用
display.setFont(&converted_u8g2_font_6x10);
display.println("Hello");
```

**メリット:**
- ✅ 既にインストール済み
- ✅ 豊富なフォント選択肢（200+種類）
- ✅ 日本語対応
- ✅ メモリ効率が良い
- ✅ 変換後は通常のフォントと同じように使用可能

**デメリット:**
- ⚠️ **フォントデータの変換作業が必要**（必須）
- ⚠️ 変換ツールの習得が必要
- ⚠️ 直接統合は不可能（変換が必須）

**参考リンク:**
- [U8g2lib GitHub](https://github.com/olikraus/u8g2)
- [U8g2 フォント一覧](https://github.com/olikraus/u8g2/wiki/fntlistall)

---

### 2. **カスタムフォント生成ツール**

**GFX Font Creator / Font Converter**

**特徴:**
- TrueType/OpenTypeフォントをAdafruit GFX形式に変換
- 任意のフォントをESP32で使用可能
- サイズや文字セットをカスタマイズ可能

**利用可能なツール:**
1. **GFX Font Creator** (オンラインツール)
   - [Adafruit GFX Font Creator](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts)
   - ブラウザベースのフォント変換ツール

2. **FontForge + スクリプト**
   - オープンソースのフォントエディタ
   - カスタムスクリプトで変換

3. **Hershey Fonts**
   - ベクターフォント形式
   - メモリ使用量が少ない

**実装方法:**
```cpp
// 1. フォントを変換してヘッダーファイルを生成
// 2. プロジェクトに追加
#include "custom_font.h"

display.setFont(&custom_font);
```

**メリット:**
- ✅ 任意のフォントを使用可能
- ✅ サイズを自由に設定
- ✅ 必要な文字のみを含めることでメモリ節約

**デメリット:**
- ⚠️ 変換作業が必要
- ⚠️ フォントファイルのサイズが大きくなる可能性

---

### 3. **LVGL (Light and Versatile Graphics Library)**

**特徴:**
- モダンなUIライブラリ
- 豊富なフォントサポート
- TrueTypeフォントの直接サポート（LVGL 8.0+）
- アンチエイリアス対応

**実装方法:**
```cpp
#include <lvgl.h>

// LVGLでフォントを設定
lv_font_t * font = &lv_font_montserrat_14;
lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
```

**メリット:**
- ✅ 非常に豊富なフォント選択肢
- ✅ TrueTypeフォントの直接サポート
- ✅ 高品質なレンダリング

**デメリット:**
- ⚠️ e-paperディスプレイには過剰（主にTFT/OLED向け）
- ⚠️ メモリ使用量が多い
- ⚠️ ESP32-C3には重い可能性

**参考リンク:**
- [LVGL GitHub](https://github.com/lvgl/lvgl)
- [LVGL Fonts](https://docs.lvgl.io/8.3/overview/font.html)

---

### 4. **ビットマップフォントシステム**

**特徴:**
- 事前にレンダリングされたビットマップフォント
- メモリ効率が良い
- カスタマイズが容易

**実装方法:**
```cpp
// カスタムビットマップフォント構造体
struct BitmapFont {
  uint8_t width;
  uint8_t height;
  const uint8_t *data;
};

// フォントデータを直接定義
const uint8_t font_data[] = { /* ... */ };
```

**メリット:**
- ✅ メモリ効率が良い
- ✅ 完全にカスタマイズ可能
- ✅ 軽量

**デメリット:**
- ⚠️ 手作業でフォントデータを作成する必要がある
- ⚠️ スケーリングが困難

---

### 5. **Hershey Fonts**

**特徴:**
- ベクターベースのフォント
- 非常に軽量
- スケーリング可能

**利用可能なフォント:**
- Hershey Simplex
- Hershey Duplex
- Hershey Complex
- Hershey Triplex
- Hershey Gothic
- Hershey Script

**実装方法:**
```cpp
#include "hershey_fonts.h"

// Hersheyフォントを使用
display.setFont(&hershey_simplex);
```

**メリット:**
- ✅ 非常に軽量
- ✅ スケーリング可能
- ✅ クラシックな見た目

**デメリット:**
- ⚠️ フォント選択肢が限定的
- ⚠️ モダンなデザインには不向き

**参考リンク:**
- [Hershey Fonts](https://en.wikipedia.org/wiki/Hershey_fonts)

---

## 🔄 GxEPD2でU8g2フォントを使用する方法

### ⚠️ 重要な前提

> ✅ 現在は **U8g2_for_Adafruit_GFX** で U8g2 フォントを直接描画しています（冒頭「現在の状態」参照）。以下は導入前に検討した方法です。

**GxEPD2とU8g2（U8g2lib のディスプレイクラス）は直接統合できません。** 当時はフォントデータの変換が必要だと考えていました。

### 方法1: フォント変換ツールを使用（推奨）

**手順:**

1. **U8g2フォントをAdafruit GFX形式に変換**
   - オンラインツールやスクリプトを使用
   - 例: [GFX Font Creator](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts)

2. **変換されたフォントをプロジェクトに追加**
   ```cpp
   // 変換されたフォントをインクルード
   #include "fonts/converted_u8g2_font.h"
   
   // 通常通りGxEPD2で使用
   display.setFont(&converted_font);
   display.println("Hello");
   ```

3. **メリット:**
   - ✅ GxEPD2の既存コードを変更不要
   - ✅ 通常のフォントと同じように使用可能
   - ✅ メモリ効率が良い

### 方法2: U8g2のレンダリング機能を間接的に使用（非推奨）

**手順:**

1. U8g2でテキストをレンダリング（オフスクリーンバッファ）
2. レンダリング結果をビットマップとして取得
3. GxEPD2でビットマップを表示

**実装例（概念）:**
```cpp
#include <U8g2lib.h>
#include <GxEPD2_BW.h>

// U8g2オブジェクト（レンダリング専用）
U8G2LIB u8g2_renderer(...);

void drawTextWithU8g2Font(const char* text, int x, int y) {
  // 1. U8g2でテキストをレンダリング
  u8g2_renderer.setFont(u8g2_font_6x10_tf);
  u8g2_renderer.firstPage();
  do {
    u8g2_renderer.drawStr(x, y, text);
  } while (u8g2_renderer.nextPage());
  
  // 2. ビットマップを取得
  // 3. GxEPD2でビットマップを描画
  // （実装は非常に複雑）
}
```

**デメリット:**
- ⚠️ 実装が非常に複雑
- ⚠️ メモリ使用量が大幅に増加
- ⚠️ パフォーマンスが低下
- ⚠️ 推奨されない方法

### 方法3: 完全にU8g2に移行（大きな変更）

**手順:**

1. GxEPD2の代わりにU8g2を使用
2. 既存のコードを全て書き換え

**実装例:**
```cpp
#include <U8g2lib.h>

// U8g2でe-paperを制御
U8G2_GDEH0154D67_1_4W_SW_SPI u8g2(...);

void setup() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 20, "Hello");
  u8g2.sendBuffer();
}
```

**デメリット:**
- ⚠️ 既存コードの大幅な書き換えが必要
- ⚠️ GxEPD2のe-paper専用機能が使えなくなる可能性
- ⚠️ 大きなリスクと作業量

---

## 📊 比較表

| フォントシステム | フォント数 | メモリ効率 | 日本語対応 | 実装の容易さ | 推奨度 |
|----------------|-----------|-----------|----------|------------|--------|
| **Adafruit GFX** (現在も大半の画面で使用) | ~50 | ⭐⭐⭐ | ❌ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **U8g2lib** | 200+ | ⭐⭐⭐⭐ | ✅ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **カスタムフォント生成** | 無制限 | ⭐⭐⭐ | 可能 | ⭐⭐ | ⭐⭐⭐⭐ |
| **LVGL** | 100+ | ⭐⭐ | ✅ | ⭐⭐ | ⭐⭐ |
| **ビットマップ** | カスタム | ⭐⭐⭐⭐⭐ | 可能 | ⭐ | ⭐⭐⭐ |
| **Hershey** | ~10 | ⭐⭐⭐⭐⭐ | ❌ | ⭐⭐⭐ | ⭐⭐ |

---

## 💡 推奨アプローチ

### 現実的な選択肢

#### 選択肢1: Adafruit GFXフォントを継続使用（最も簡単）

**メリット:**
- ✅ 既に動作している
- ✅ 追加の作業不要
- ✅ 安定している

**デメリット:**
- ⚠️ フォント選択肢が限定的（~50種類）

**推奨度:** ⭐⭐⭐⭐（現状維持が最適な場合も多い）

---

#### 選択肢2: フォント変換ツールでU8g2フォントを使用（推奨）

**メリット:**
- ✅ 200種類以上のフォントから選択可能
- ✅ 日本語フォントも利用可能
- ✅ GxEPD2の既存コードを変更不要
- ✅ 変換後は通常のフォントと同じように使用

**デメリット:**
- ⚠️ フォント変換の作業が必要
- ⚠️ 変換ツールの習得が必要

**手順:**
1. U8g2フォントをAdafruit GFX形式に変換
2. 変換されたフォントをプロジェクトに追加
3. 通常通り`display.setFont()`で使用

**推奨度:** ⭐⭐⭐⭐⭐（より多くのフォントが必要な場合）

---

#### 選択肢3: 完全にU8g2に移行（非推奨）

**メリット:**
- ✅ U8g2の全機能が使用可能
- ✅ 豊富なフォントが直接使用可能

**デメリット:**
- ⚠️ 既存コードの大幅な書き換えが必要
- ⚠️ GxEPD2のe-paper専用機能が使えなくなる可能性
- ⚠️ 大きなリスクと作業量

**推奨度:** ⭐（特別な理由がない限り推奨しない）

### 長期解決策: カスタムフォント生成

特定のフォントが必要な場合：
1. フォント変換ツールを使用
2. 必要なサイズと文字セットでカスタムフォントを生成
3. プロジェクトに追加

---

## 🛠️ 実装例: フォント変換ツールの使用

### ステップ1: フォント変換ツールを探す

**利用可能なツール:**

1. **Adafruit GFX Font Creator**
   - [GitHubリポジトリ](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts)
   - TrueType/OpenTypeフォントをAdafruit GFX形式に変換

2. **オンラインフォント変換ツール**
   - 検索: "Adafruit GFX font converter"
   - または: "U8g2 to Adafruit GFX font converter"

3. **カスタムスクリプト**
   - U8g2フォントデータを解析
   - Adafruit GFX形式に変換

### ステップ2: フォントを変換

1. 使用したいU8g2フォントを選択
2. 変換ツールでAdafruit GFX形式に変換
3. 変換された`.h`ファイルを取得

### ステップ3: プロジェクトに追加

```cpp
// 変換されたフォントをインクルード
#include "fonts/converted_u8g2_font_6x10.h"

// 通常通り使用
display.setFont(&converted_u8g2_font_6x10);
display.setCursor(10, 50);
display.println("Hello World");
```

### ステップ4: テスト

```cpp
void testConvertedFont() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    // 変換されたフォントを使用
    display.setFont(&converted_u8g2_font_6x10);
    display.setCursor(10, 30);
    display.println("Test Font");
    
  } while (display.nextPage());
}
```

---

## 📚 参考資料

- [U8g2lib 公式ドキュメント](https://github.com/olikraus/u8g2/wiki)
- [U8g2 フォント一覧](https://github.com/olikraus/u8g2/wiki/fntlistall)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [FontForge](https://fontforge.org/)

---

## ⚠️ 注意事項

1. **メモリ制約**: ESP32-C3のメモリは限られているため、大きなフォントは使用を避ける
2. **e-paper特性**: e-paperディスプレイは更新が遅いため、アニメーションや動的なフォント変更には不向き
3. **統合の複雑さ**: 異なるフォントシステムを統合するには追加の作業が必要

---

## 🎯 次のステップ

### 推奨される手順

1. **現状の評価**: 現在のAdafruit GFXフォントで十分か確認
   - もし十分なら、変更不要

2. **フォント変換ツールの調査**: より多くのフォントが必要な場合
   - Adafruit GFX Font Creatorなどのツールを調査
   - U8g2フォントをAdafruit GFX形式に変換する方法を確認

3. **テスト実装**: 小さなテストコードで動作確認
   - 変換されたフォントを1つ試す
   - メモリ使用量とパフォーマンスを確認

4. **本番実装**: 問題なければ本番コードに統合
   - 必要なフォントを変換
   - プロジェクトに追加して使用

---

## 📝 まとめ：GxEPD2とU8g2の関係

### 重要なポイント

1. **GxEPD2はAdafruit GFX Libraryを継承している**
   - GxEPD2 = e-paperドライバ + Adafruit GFX機能
   - Adafruit GFX形式のフォントを直接使用可能

2. **U8g2は独立したライブラリ**
   - GxEPD2とは直接統合できない
   - 独自のフォントシステムを持っている

3. **U8g2フォントをGxEPD2で使用するには変換が必要**
   - フォントデータをAdafruit GFX形式に変換
   - 変換後は通常のフォントと同じように使用可能

4. **推奨アプローチ**
   - 現状維持（Adafruit GFXフォント）: 最も簡単
   - フォント変換ツール使用: より多くのフォントが必要な場合
   - 完全移行（U8g2）: 非推奨（大きな変更が必要）

### 結論

**GxEPD2とU8g2は異なるライブラリで、直接統合はできません。** しかし、U8g2の豊富なフォントをGxEPD2で使用するには、フォント変換ツールを使用してAdafruit GFX形式に変換すれば可能です。

> ✅ **その後の結論（現在）**: フォント変換ではなく **U8g2_for_Adafruit_GFX** を採用し、U8g2 フォントを変換なしで GxEPD2 の画面に描画しています（`displayClimateModern()`・`displayWeather()` と `src/u8g2_gfx_font_test.cpp`）。詳細は冒頭の「現在の状態」を参照してください。

