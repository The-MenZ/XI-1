# 🗓️ 月日表示画面（シンプル & 洒落たデザイン）

## 📋 概要
月と日だけを大きく表示する、シンプルで洒落た画面です（画面5: Month Day）。

---

## 🎨 デザイン

現在時刻が `2025-11-29 15:30:45` の場合：

```
┌──────────────────────────┐
│                          │
│          11              │ ← 月（2桁、ベースライン y=85）
│                          │
│                          │
│          29              │ ← 日（2桁、ベースライン y=175）
│                          │
└──────────────────────────┘
```

1桁の月日はゼロ埋めします（例: 1月5日 → `01` / `05`）。

---

## ✨ 特徴

| 項目 | 説明 |
|------|------|
| **フォント** | FreeMonoBold24pt7b（等幅太字）を `setTextSize(2)` で2倍表示（1文字の送り幅 56 px、数字の高さ 約62 px） |
| **ゼロ埋め** | 月・日とも常に2桁 |
| **レイアウト** | 月を上、日を下に置き、それぞれ横方向にセンターアライン |
| **装飾** | なし（区切り線やラベルは描かない） |
| **年** | 表示なし（シンプル性重視） |

---

## 🔧 技術実装

### ヘッダー宣言 (`display_screens.h`)
```cpp
// 画面5: 月日表示（シンプル & 洒落たデザイン）
void displayMonthDay(int month, int day);
```

※ 画面番号は `main.cpp` の `SCREEN_ENABLED` / `switch` の添字（0始まり）と同じです。

### 実装 (`display_screens.cpp`)
```cpp
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
```

センタリングは1文字 60 px として計算しています（2桁なので x = (200 − 120) / 2 = 40）。実際の送り幅は 56 px ですが、グリフの左オフセットと合わせて数字はほぼ中央に描かれます（`00` の場合 約 48〜154 px）。

### 呼び出し (`main.cpp`)
```cpp
case 5: {
  // 月日表示画面
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  displayMonthDay(timeinfo.tm_mon + 1, timeinfo.tm_mday);
  break;
}
```

> NTP の同期状態はチェックしていません。同期前の時計は 1970年1月1日 から始まるので、起動直後に同期できていないと `01` / `01` と表示されます。

---

## 📊 画面順序

`main.cpp` の `SCREEN_ENABLED` で各画面の有効/無効を設定します。

| 画面 | 画面名 | 内容 | 既定 |
|------|--------|------|------|
| 0 | Air Quality | eCO2, TVOC, AQI | 有効 |
| 1 | Climate | 温度, 湿度, 気圧, 高度 | 有効 |
| 2 | Ambience | 照度, 騒音レベル | 有効 |
| 3 | Gas Sensor | Rs/Ro（起動後3分間は Warming up） | 有効 |
| 4 | Date & Time | 日時表示（フル） | **無効** |
| 5 | **Month Day** | 月・日のみ | 有効 |
| 6 | WiFi QR | WiFi 接続用 QR コード | 有効 |
| 7 | Climate Modern | 温湿度の円形ゲージ＋過去24時間グラフ | 有効 |

---

## 🔄 動作フロー

```
10秒ごとに有効な画面だけが切り替わります（既定）：

Air Quality
    ↓ 10秒
Climate
    ↓ 10秒
Ambience
    ↓ 10秒
Gas Sensor
    ↓ 10秒
Month Day          ← Date & Time は既定で無効なのでスキップ
    ↓ 10秒
WiFi QR
    ↓ 10秒
Climate Modern
    ↓ 10秒
[リピート]
```

通常は部分リフレッシュで更新し、10分ごとにゴースト軽減のためフルリフレッシュ（約1.5秒）します。

---

## 🎯 カスタマイズのポイント

### 1. 月と日の位置を調整
```cpp
display.setCursor(x_month, 85);   // ← ベースライン y=85 に月を表示
display.setCursor(x_day, 175);    // ← ベースライン y=175 に日を表示
```

### 2. フォントサイズを変更
```cpp
display.setFont(&FreeMonoBold24pt7b);
display.setTextSize(2);   // ← 2倍。数字1つが約 56×62 px になる
```
フォントや倍率を変えたら、センタリング計算の `* 60`（1文字の幅）も合わせて変更してください。200x200 の画面に2行収まる大きさにする必要があります。

---

## ✅ 検証済み

- ✅ 日付が正しく取得される（NTP 同期後）
- ✅ 月・日が2桁ゼロ埋めで表示される
- ✅ センターアライン機能が動作
- ✅ 画面切り替えが10秒ごとに動作

---

## 💡 設計の考慮事項

### なぜシンプルなのか？
- 200x200 e-paper での表示制限
- 一目で月日を把握する必要性

### なぜ等幅フォント＋ゼロ埋めなのか？
- 等幅フォントなので、2桁にそろえれば日付が変わっても数字の位置と幅が変わらない
- 太字の大きな数字だけで画面が成り立つ

---

## 🔮 将来の拡張案

- 曜日の表示（例：Friday）
- 季節情報の表示（🍂 Autumn など）
- 記念日マーク（🎂 Birthday など）
- 月の満ち欠け表示（🌕 Full Moon など）
