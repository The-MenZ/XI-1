# 📱 WiFi QR / 設定ページ QR 画面

## 概要
QR コードを表示する画面は2つあります。

| 画面 | 内容 | 中央のアイコン |
|------|------|----------------|
| **6: WiFi QR** | 保存済みの WiFi（SSID・パスワード）に接続するための QR。AP モード中は本体の AP「XI-1-XXXX」に接続するための QR | WiFi マーク |
| **8: Settings QR** | 設定ページの URL（`http://<本体のIP>/`、AP モード中は `http://192.168.4.1/`） | 歯車 |

WiFi の SSID・パスワードは設定ページで変更し、NVS に保存されます（`include/wifi_config.h` は保存前の初期値。README の「WiFi 設定」を参照）。

---

## 🎨 表示例

QRコードを画面中央に表示し、その中央にアイコンのバッジを重ねます（SSID などの文字は描きません）。
バッジは外から白い枠・黒い枠・白い枠（各 2 px）の三重で、アイコンは内側の白い枠の中いっぱいに描きます。

```
┌──────────────────────────┐
│                          │ ← 余白（最低1モジュール）
│  ▓▓▓▓▓▓▓  ▓ ▓▓  ▓▓▓▓▓▓▓  │
│  ▓     ▓ ▓▓  ▓  ▓     ▓  │
│  ▓ ▓▓▓ ▓  ▓ ▓▓  ▓ ▓▓▓ ▓  │
│  ▓ ▓▓▓ ▓ ┏━━━━┓ ▓ ▓▓▓ ▓  │ ← 白い枠・黒い枠・白い枠
│  ▓     ▓ ┃ 📶 ┃ ▓     ▓  │ ← 枠の中いっぱいにアイコン（30〜37 px）
│  ▓▓▓▓▓▓▓ ┗━━━━┛ ▓▓▓▓▓▓▓  │
│   ▓ ▓▓  ▓▓ ▓ ▓▓  ▓ ▓ ▓   │
│  ...                     │
│                          │
└──────────────────────────┘
```

---

## ✨ 機能

| 項目 | 説明 |
|------|------|
| **QRコード生成** | ricmoo/QRCode、誤り訂正レベル Q（`ECC_QUARTILE`、約25%まで復元） |
| **フォーマット（WiFi）** | `WIFI:T:WPA;S:<SSID>;P:<PASSWORD>;;`（パスワードなしは `WIFI:T:nopass;S:<SSID>;;`） |
| **フォーマット（設定ページ）** | `http://<IP>/` |
| **エスケープ** | SSID・パスワード中の `\` `;` `,` `:` `"` の前に `\` を付ける |
| **バージョン自動選択** | v1〜v10 のうち、文字列が収まる最小のバージョン |
| **サイズ自動調整** | 1モジュール = 200 px ÷（QR一辺のモジュール数 + 2）（切り捨て）、中央に配置 |
| **中央アイコン** | 白・黒・白の三重枠のバッジ（42〜49 px）の中いっぱいに、コードで描いた WiFi マーク / 歯車。v6 以下のときだけ重ねる |
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
void displaySettingsQR(const String& url);
```

### 3. 実装 (`display_screens.cpp`)

2つの画面は共通の `drawQRCode(text, drawIcon)` で描きます。

```cpp
// 誤り訂正レベル Q・バイトモードで各バージョンに入る最大バイト数（v1〜v10）
static const uint16_t QR_BYTE_CAPACITY_Q[] = {11, 20, 32, 46, 60, 74, 86, 108, 130, 151};

// v7 以降は中央に位置合わせパターンが入るので、アイコンを重ねない
static const uint8_t QR_ICON_MAX_VERSION = 6;

// 中央のバッジ: 外から白い枠・黒い枠・白い枠（各 2 px）、その内側いっぱいにアイコン
#define QR_BADGE_MIN_PX 42
#define QR_BADGE_GAP 2     // 外側の白い枠
#define QR_BADGE_BORDER 2  // 黒い枠
#define QR_BADGE_PAD 2     // 内側の白い枠

// アイコンは s×s の正方形の大きさに合わせてコードで描く（ビットマップは持たない）
static void drawWiFiIcon(int16_t x0, int16_t y0, int16_t s);  // 点 + ±45° の弧3本（端は丸い）
static void drawGearIcon(int16_t x0, int16_t y0, int16_t s);  // 8枚の歯 + 中央の穴

static void drawQRCode(const String& text, QRIconDrawer drawIcon) {
  // 文字列が収まる最小のバージョンを選ぶ（収まらなければ "QR: data too long"）
  // qrcode_initText(..., version, ECC_QUARTILE, text.c_str());

  // 周りに最低1マスの白い余白を残して、1モジュールのピクセルサイズを決める
  int module_size = display.width() / (qrcode.size + 2);

  // 黒モジュールを fillRect で描画

  // QR_BADGE_MIN_PX 以上の奇数マスの白い正方形を中央に置き、黒い枠とアイコンを描く
  // （QR の一辺は必ず奇数マスなので、奇数マスの正方形はちょうど中央に収まる）
}

void displayWiFiQR(const String& ssid, const String& password) {
  // パスワードが空なら T:nopass
  drawQRCode(wifi_string, drawWiFiIcon);
}

void displaySettingsQR(const String& url) {
  // url が空（WiFi 切断中）なら "Settings / WiFi: Not Connected" を表示
  drawQRCode(url, drawGearIcon);
}
```

#### なぜバージョンを自動選択するのか
ricmoo/QRCode は文字列がバージョンの容量に収まるかをチェックしません。以前はバージョン1固定だったため、WiFi 文字列が容量を超えてスタック上のバッファを壊していました。現在はエスケープ後の文字列のバイト数から収まる最小のバージョンを選び、バッファも `qrcode_getBufferSize(version)` で確保しています。

#### なぜ誤り訂正レベル Q なのか
中央のバッジで隠れたモジュールは読み取れないので、誤り訂正で補います。L（約7%まで復元）では足りないため Q（約25%まで復元）にしています。バッジで隠れるのは QR 全体の 8% 以下です。

誤り訂正はブロックごとに「直せるコード語の数」が決まっていて、1つのブロックでも超えると読めません。下の表の「誤り訂正の使用率」は、バッジに1マスでもかかるコード語をすべて壊れたとみなしたときの、いちばん厳しいブロックの割合です。v2 は 91% と余裕が少ないですが、これ以上小さいバッジ（5マス = 35 px）ではアイコンが小さすぎるため、このサイズにしています。

#### なぜバッジをこれ以上大きくしないのか
アイコンを大きくするためにバッジを広げると、v3 は 9 マス（54 px）、v4 は 11 マス（55 px）になり、誤り訂正の使用率が 89〜92% まで上がります。PC で試すと v3 の 9 マスは、画像を小さく写したときに OpenCV で読めないものが半分以上ありました。そのためバッジの大きさは以前の白い正方形（v2〜v5）と同じにし、その中に枠を入れてアイコンを枠いっぱいに描いています（v6 だけ、アイコンが小さくなりすぎないよう 9 マス → 11 マスに広げています）。

#### バージョンと表示サイズ

| バージョン | モジュール数 | 最大バイト数 | 1モジュール | QRの大きさ | バッジ（アイコン） | 隠れる割合 | 誤り訂正の使用率 |
|---|---|---|---|---|---|---|---|
| v1 | 21×21 | 11 | 8 px | 168 px | 使わない（文字列が短すぎる） | | |
| v2 | 25×25 | 20 | 7 px | 175 px | 7マス 49 px（37 px） | 7.8% | 91% |
| v3 | 29×29 | 32 | 6 px | 174 px | 7マス 42 px（30 px） | 5.8% | 67% |
| v4 | 33×33 | 46 | 5 px | 165 px | 9マス 45 px（33 px） | 7.4% | 77% |
| v5 | 37×37 | 60 | 5 px | 185 px | 9マス 45 px（33 px） | 5.9% | 44% |
| v6 | 41×41 | 74 | 4 px | 164 px | 11マス 44 px（32 px） | 7.2% | 50% |
| v7 | 45×45 | 86 | 4 px | 180 px | アイコンなし | | |
| v8 | 49×49 | 108 | 3 px | 147 px | アイコンなし | | |
| v9 | 53×53 | 130 | 3 px | 159 px | アイコンなし | | |
| v10 | 57×57 | 151 | 3 px | 171 px | アイコンなし | | |

- 設定ページの URL（`http://192.168.4.1/` など 19〜23 バイト）は v2〜v3
- AP の WiFi QR（`WIFI:T:nopass;S:XI-1-XXXX;;`、27 バイト）は v3
- 家の WiFi の QR は SSID とパスワードの長さで決まります（エスケープ後の合計が 28 文字までなら v4、42 文字までなら v5）
- 151 バイトを超えると "QR: data too long" と表示します

#### シリアル出力（デバッグ）
```
DEBUG: QRCode gen err=0, version=3, size=29, bytes=<文字列のバイト数>
DEBUG: module_size=6, qr_width=174, start_x=13, start_y=13
DEBUG: black_pixels=<黒モジュールの数> (drawn with fillRect)
```

### 4. 呼び出し (`main.cpp`)
```cpp
case SCREEN_WIFI_QR: {
  // WiFi QRコード表示画面（AP モード中は自分の AP に接続するための QR）
  if (isAPMode()) {
    displayWiFiQR(getAPSSID(), "");
  } else {
    displayWiFiQR(settings.wifi_ssid, settings.wifi_password);
  }
  break;
}
case SCREEN_SETTINGS_QR:
  // 設定ページの URL の QRコード
  displaySettingsQR(getSettingsURL());
  break;
```

---

## 📊 画面順序

各画面の有効/無効は設定ページで変更します（既定値は `main.cpp` の `SCREEN_DEFAULT_ENABLED`）。

| 画面 | 画面名 | 内容 | 既定 |
|------|--------|------|------|
| 0 | Air Quality | eCO2, TVOC, AQI | 有効 |
| 1 | Climate | 温度, 湿度, 気圧, 高度 | 有効 |
| 2 | Ambience | 照度, 騒音レベル | 有効 |
| 3 | Gas Sensor | カナリアと HP ゲージ（起動後3分間は卵） | 有効 |
| 4 | Date & Time | 日時表示（フル） | **無効** |
| 5 | Month Day | 月・日のみ | 有効 |
| 6 | **WiFi QR** | WiFi の QRコード | 有効 |
| 7 | Climate Modern | 温湿度の円形ゲージ＋過去24時間グラフ | 有効 |
| 8 | **Settings QR** | 設定ページの QRコード | 有効 |
| 9 | Weather | 今の天気と今日・明日の予報 | 有効 |
| 10 | Pressure | 天気図風の H/L と等圧線＋3時間の傾向 | 有効 |

AP モード中は設定に関係なく、画面6（AP の WiFi QR）と画面8（設定ページ QR）だけを交互に表示します。

---

## 🔄 動作フロー

```
切り替え間隔（既定10秒）ごとに有効な画面だけが切り替わります（既定）：

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
Settings QR
    ↓ 10秒
Weather
    ↓ 10秒
Pressure
    ↓ 10秒
[リピート]
```

通常は部分リフレッシュで更新し、10分ごとにゴースト軽減のためフルリフレッシュ（約1.5秒）します。

---

## 📱 使用方法

### WiFi QR で接続する

1. **QRコードを読み取る**
   - スマートフォンのカメラアプリまたはQRコードスキャナーで読み取り
2. **WiFi接続確認**
   - 「このWiFiネットワークに接続しますか？」という通知が表示
   - 「接続」をタップ

### 設定ページを開く

1. スマホ（または PC）を本体と同じ WiFi に接続する（AP モード中は先に WiFi QR で AP に接続）
2. 歯車の QR を読み取ると、ブラウザで設定ページが開く

---

## ⚠️ セキュリティに関する注意

### パスワードがQRコードに平文で含まれる
- 画面6（WiFi QR）は家の WiFi のパスワードを平文で含みます
- 他人に画面を見られないよう注意してください
- 不要なら設定ページで画面6を OFF にしてください

### 設定ページと AP
- 設定ページにパスワードはありません。同じ WiFi（AP モード中は本体の AP）につながっている人は誰でも開けます
- AP はパスワードなしです
- 保存済みの WiFi パスワードは設定ページに表示しません

---

## 🎨 カスタマイズ例

### 認証方式を変更（WEP）
```cpp
wifi_string = "WIFI:T:WEP;S:" + escapeWiFiQRField(ssid) +
              ";P:" + escapeWiFiQRField(password) + ";;";
```

### アイコンを変える
アイコンは `drawWiFiIcon` / `drawGearIcon` のように「s×s の正方形の中いっぱいに描く関数」を作り、`drawQRCode` に渡します。大きさはバッジから決まるので、アイコン側で固定のサイズを持つ必要はありません。

枠の太さは `QR_BADGE_GAP` / `QR_BADGE_BORDER` / `QR_BADGE_PAD` で変えられます（太くするとそのぶんアイコンが小さくなります）。`QR_BADGE_MIN_PX` を大きくすると隠れるコード語が増えるので、上の表の「誤り訂正の使用率」が 100% を十分下回るか確かめてください。

### モジュールサイズ
モジュールサイズは QR のバージョンから自動で計算します（`module_size = display.width() / (qrcode.size + 2)`）。固定値にする場合は、`qrcode.size × module_size` が 200 px を超えないようにしてください。

---

## ✅ 検証済み

- ✅ 容量に応じたバージョン選択でバッファ溢れを解消（誤り訂正レベル Q の容量は ricmoo/QRCode のテーブルから計算して確認）
- ✅ アイコン入りの画面を PC 上で同じコードから画像にし、zbar と OpenCV で読み取れることを確認（v2〜v6、URL・AP の WiFi QR・家の WiFi QR）
- ✅ 三重枠のバッジにしたあとも、PC 上で v2〜v6 の約 160 通りの文字列を等倍・2倍・縮小・ぼかしで読み、以前のバッジと同じ程度に読めることを確認（zbar はすべて成功。OpenCV は 1 モジュールが 3 px 以下になるほど縮めると以前のバッジでも読めないものがあり、その数はほぼ同じ）
- ✅ 実機の e-Paper をスマホで読み取り、AP への接続（WiFi QR）と設定ページを開くこと（Settings QR）を確認
- ✅ 設定ページで WiFi を保存し、再起動後に接続できることを実機で確認

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
