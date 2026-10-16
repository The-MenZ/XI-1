# 📱 WiFi QRコード表示画面

## 概要
`include/wifi_config.h` に設定した WiFi 情報（`WIFI_SSID` / `WIFI_PASSWORD`）をQRコードで表示し、他のデバイスから簡単に接続できるようにする画面です。

---

## 🎨 表示例

QRコードだけを画面いっぱいに中央表示します（SSID などの文字は描きません）。

```
┌──────────────────────────┐
│                          │ ← 余白（現在の設定では 13 px）
│  ▓▓▓▓▓▓▓  ▓ ▓▓  ▓▓▓▓▓▓▓  │
│  ▓     ▓ ▓▓  ▓  ▓     ▓  │
│  ▓ ▓▓▓ ▓  ▓ ▓▓  ▓ ▓▓▓ ▓  │
│  ▓ ▓▓▓ ▓ ▓▓ ▓   ▓ ▓▓▓ ▓  │ ← WiFi情報を含むQRコード
│  ▓     ▓  ▓  ▓  ▓     ▓  │
│  ▓▓▓▓▓▓▓ ▓ ▓ ▓  ▓▓▓▓▓▓▓  │
│   ▓ ▓▓  ▓▓ ▓ ▓▓  ▓ ▓ ▓   │
│  ...                     │
│                          │
└──────────────────────────┘
```

---

## ✨ 機能

| 項目 | 説明 |
|------|------|
| **QRコード生成** | ricmoo/QRCode、誤り訂正レベル L（`ECC_LOW`） |
| **フォーマット** | `WIFI:T:WPA;S:<SSID>;P:<PASSWORD>;;` |
| **エスケープ** | SSID・パスワード中の `\` `;` `,` `:` `"` の前に `\` を付ける |
| **バージョン自動選択** | v1〜v10 のうち、文字列が収まる最小のバージョン |
| **サイズ自動調整** | 1モジュール = 200 px ÷ QR一辺のモジュール数（切り捨て）、中央に配置 |
| **接続方法** | スマートフォンのカメラ / QRコードスキャナーで読み取り |

---

## 🔧 技術実装

### 1. ライブラリ追加 (`platformio.ini`)
```ini
lib_deps = 
    ...
    ricmoo/QRCode@^0.0.1
```

### 2. 関数宣言 (`display_screens.h`)
```cpp
void displayWiFiQR(const String& ssid, const String& password);
```

### 3. 実装 (`display_screens.cpp`)

```cpp
// 誤り訂正レベル L・バイトモードで各バージョンに入る最大バイト数（v1〜v10）
static const uint16_t QR_BYTE_CAPACITY_L[] = {17, 32, 53, 78, 106, 134, 154, 192, 230, 271};
static const uint8_t QR_MAX_VERSION = sizeof(QR_BYTE_CAPACITY_L) / sizeof(QR_BYTE_CAPACITY_L[0]);

// \ ; , : " をバックスラッシュでエスケープ
static String escapeWiFiQRField(const String& value);

void displayWiFiQR(const String& ssid, const String& password) {
  String wifi_string = "WIFI:T:WPA;S:" + escapeWiFiQRField(ssid) +
                       ";P:" + escapeWiFiQRField(password) + ";;";

  // 文字列が収まる最小のバージョンを選ぶ
  uint8_t version = 0;
  for (uint8_t v = 1; v <= QR_MAX_VERSION; v++) {
    if (wifi_string.length() <= QR_BYTE_CAPACITY_L[v - 1]) {
      version = v;
      break;
    }
  }
  if (version == 0) {
    // "QR: data too long" を表示して終了
  }

  // バージョンに応じたサイズのバッファを確保
  uint8_t qrcodedata[qrcode_getBufferSize(version)];
  struct QRCode qrcode;
  int err = qrcode_initText(&qrcode, qrcodedata, version, ECC_LOW, wifi_string.c_str());
  if (err != 0) {
    // "QR Error: <err>" を表示して終了
  }

  // 1モジュールのピクセルサイズ（画面いっぱい）と中央寄せ
  int module_size = display.width() / qrcode.size;
  if ((qrcode.size * module_size) > display.height()) {
    module_size = display.height() / qrcode.size;
  }
  int qr_width = qrcode.size * module_size;
  int start_x = (display.width() - qr_width) / 2;
  int start_y = (display.height() - qr_width) / 2;

  // 黒モジュールを fillRect で描画
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        display.fillRect(start_x + x * module_size, start_y + y * module_size,
                         module_size, module_size, GxEPD_BLACK);
      }
    }
  }
}
```

#### なぜバージョンを自動選択するのか
ricmoo/QRCode は文字列がバージョンの容量に収まるかをチェックしません。以前はバージョン1固定だったため、WiFi 文字列が容量を超えてスタック上のバッファを壊していました。現在はエスケープ後の文字列のバイト数から収まる最小のバージョンを選び、バッファも `qrcode_getBufferSize(version)` で確保しています。

#### バージョンと表示サイズ

| バージョン | モジュール数 | 最大バイト数 | 1モジュール | QRの大きさ |
|---|---|---|---|---|
| v1 | 21×21 | 17 | 9 px | 189 px |
| v2 | 25×25 | 32 | 8 px | 200 px |
| **v3** | **29×29** | **53** | **6 px** | **174 px** |
| v4 | 33×33 | 78 | 6 px | 198 px |
| v5 | 37×37 | 106 | 5 px | 185 px |
| v6 | 41×41 | 134 | 4 px | 164 px |
| v7 | 45×45 | 154 | 4 px | 180 px |
| v8 | 49×49 | 192 | 4 px | 196 px |
| v9 | 53×53 | 230 | 3 px | 159 px |
| v10 | 57×57 | 271 | 3 px | 171 px |

現在の設定では **v3**（29×29、1モジュール 6 px、174 px 四方）になります。271 バイトを超えると "QR: data too long" と表示します。

#### シリアル出力（デバッグ）
```
DEBUG: QRCode gen err=0, version=3, size=29, bytes=<文字列のバイト数>
DEBUG: module_size=6, qr_width=174, start_x=13, start_y=13
DEBUG: black_pixels=<黒モジュールの数> (drawn with fillRect)
```

### 4. 呼び出し (`main.cpp`)
```cpp
case 6: {
  // WiFi QRコード表示画面
  displayWiFiQR(WIFI_SSID, WIFI_PASSWORD);
  break;
}
```

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
| 5 | Month Day | 月・日のみ | 有効 |
| 6 | **WiFi QR** | QRコード表示 | 有効 |
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

## 📱 使用方法

### スマートフォンでの接続手順

1. **QRコードを読み取る**
   - スマートフォンのカメラアプリまたはQRコードスキャナーで読み取り

2. **WiFi接続確認**
   - 「このWiFiネットワークに接続しますか？」という通知が表示
   - 「接続」をタップ

3. **自動入力**
   - SSID と パスワード が自動で入力される
   - すぐに接続完了

---

## ⚠️ セキュリティに関する注意

### パスワードがQRコードに平文で含まれる
- QRコードはパスワードを平文で含みます
- 他人に画面を見られないよう注意してください
- 信頼できるデバイスのみで接続させることを推奨

### QR画面が不要な場合
- `main.cpp` の `SCREEN_ENABLED` で画面6（WiFi QR）を `false` にすると表示されなくなります
- `include/wifi_config.h` は `.gitignore` に入っています。リポジトリにコミットしないでください

---

## 🎨 カスタマイズ例

### 認証方式を変更（WEP）
```cpp
String wifi_string = "WIFI:T:WEP;S:" + escapeWiFiQRField(ssid) +
                     ";P:" + escapeWiFiQRField(password) + ";;";
```

### パスワードなしのネットワーク
```cpp
String wifi_string = "WIFI:T:nopass;S:" + escapeWiFiQRField(ssid) + ";;";
```

### モジュールサイズ
モジュールサイズは QR のバージョンから自動で計算します（`module_size = display.width() / qrcode.size`）。固定値にする場合は、`qrcode.size × module_size` が 200 px を超えないようにしてください。

---

## ✅ 検証済み

- ✅ 実機で表示を確認（現在の設定では v3・29×29・1モジュール 6 px）
- ✅ 容量に応じたバージョン選択でバッファ溢れを解消
- ✅ 200x200ディスプレイいっぱいに中央表示

---

## 🔮 将来の拡張案

- ゲストネットワーク用QRコードの表示
- QRコード有効期限の設定
- 複数SSID対応
- QR画面のセキュリティ制限（PIN入力など）
- WiFi情報の暗号化表示

---

## 📝 参考資料

- [QRCode Library by ricmoo](https://github.com/ricmoo/qrcode)
- [WiFi設定フォーマット仕様](https://github.com/zxing/zxing/wiki/Barcode-Contents#wi-fi-network-config)
- [GxEPD2 ドライバ](https://github.com/ZinggJM/GxEPD2)
