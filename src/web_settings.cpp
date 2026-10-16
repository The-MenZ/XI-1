#include "web_settings.h"
#include <WebServer.h>
#include <WiFi.h>
#include "network.h"
#include "settings.h"

// main.cpp で定義（NTP サーバーとタイムゾーンを settings から反映する）
extern void applyTimeSettings();

bool mics5524_calibration_requested = false;

static WebServer server(80);
static const char* const* screen_names = nullptr;
static uint8_t num_screens = 0;

// 保存や再起動のあと、応答を返し終えてから再起動するための時刻（0 = 予定なし）
static unsigned long restart_requested_at = 0;
#define RESTART_DELAY_MS 1000

static const char PAGE_HEAD[] =
  "<!doctype html><html lang=ja><head><meta charset=utf-8>"
  "<meta name=viewport content='width=device-width,initial-scale=1'>"
  "<title>XI-1 設定</title><style>"
  "body{font-family:sans-serif;max-width:480px;margin:auto;padding:0 16px}"
  "fieldset{margin:12px 0}"
  "input,select,button{font-size:16px;margin:4px 0}"  // 16px 未満だと iOS が入力時に拡大する
  "input[type=text],input[type=password],input[type=number],select{width:100%;box-sizing:border-box}"
  "</style></head><body><h1>XI-1 設定</h1>";

static const char PAGE_TAIL[] = "</body></html>";

// 天気の地域の候補探し（市区町村名 → 候補 → 緯度・経度）。本体ではなくブラウザから
// CSIS シンプルジオコーディング（東京大学）に問い合わせるので、本体は検索の処理を持たない。
// 候補は自動では選ばない（同じ名前の別の場所を選んだまま気づかないのを防ぐ）。
// 候補を選ぶと、地名（hidden）と緯度・経度の入力欄に値が入る。探し直したときや
// 「選んでください」に戻したときは、ページを開いたときの値（保存済みの値）に戻す
// （hidden の input は value を変えると defaultValue も変わるので、最初に覚えておく）。iLvl は住所の細かさ
// （1 = 都道府県、3 = 市区町村、5 = 町・大字 …。0 は重複などなので除く）で、粗い順に並べる
static const char WEATHER_SCRIPT[] =
  "<script>"
  "var wl=[],$=i=>document.getElementById(i),wi=['wn','wla','wlo'],ws=wi.map(i=>$(i).value);"
  "function wreset(){wi.forEach((i,k)=>$(i).value=ws[k])}"
  "function geo(){"
    "wreset();$('wr').hidden=true;$('wm').textContent='検索中…';"
    "fetch('https://geocode.csis.u-tokyo.ac.jp/cgi-bin/simple_geocode.cgi?charset=UTF8&addr='"
      "+encodeURIComponent($('wq').value.trim()))"
    ".then(r=>r.text()).then(t=>{"
      "var x=new DOMParser().parseFromString(t,'text/xml');"
      "wl=[...x.getElementsByTagName('candidate')].map(c=>{"
        "var g=k=>c.getElementsByTagName(k)[0].textContent;"
        "return{n:g('address').split('/').join(' '),la:g('latitude'),lo:g('longitude'),l:+g('iLvl')}"
      "}).filter(c=>c.l>0).sort((a,b)=>a.l-b.l);"
      "$('wr').innerHTML='';$('wr').add(new Option(wl.length+'件の候補から選んでください',''));"
      "wl.forEach((c,i)=>$('wr').add(new Option(c.n,i)));"
      "$('wr').hidden=!wl.length;"
      "$('wm').textContent=wl.length?'':'見つかりませんでした（地域は変わっていません）'"
    "}).catch(()=>{$('wm').textContent='検索できません（インターネットに未接続）。地域は変わっていません'})}"
  "function wpick(){"
    "var c=wl[$('wr').value];if(!c){wreset();$('wm').textContent='';return}"
    "$('wn').value=c.n;$('wla').value=c.la;$('wlo').value=c.lo;"
    "$('wm').textContent='保存すると「'+c.n+'」に変わります'}"
  "function wman(){"
    "$('wn').value='';$('wm').textContent='保存すると緯度・経度で指定した地点に変わります'}"
  "</script>";

// HTML に埋め込む値（SSID など）のエスケープ
static String htmlEscape(const String& value) {
  String escaped;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value[i];
    switch (c) {
      case '&': escaped += "&amp;"; break;
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      case '"': escaped += "&quot;"; break;
      case '\'': escaped += "&#39;"; break;
      default: escaped += c;
    }
  }
  return escaped;
}

// 緯度・経度として受け付ける文字列か（10進数で、整数部の絶対値が limit 以下）
// 天気 API の URL にそのまま入れるので、数字・先頭の符号・小数点以外の文字は受け付けない
// （Google マップからコピーすると小数点以下が十数桁になることがあるので、長さは20文字まで）
static bool isCoordinate(const String& value, int limit) {
  if (value.length() == 0 || value.length() > 20) {
    return false;
  }
  bool has_digit = false;
  bool has_dot = false;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '-' && i == 0) {
      continue;
    }
    if (c == '.' && !has_dot) {
      has_dot = true;
      continue;
    }
    if (!isdigit(c)) {
      return false;
    }
    has_digit = true;
  }
  return has_digit && abs(value.toInt()) <= limit;
}

static void sendPage(const String& body) {
  server.send(200, "text/html; charset=utf-8", PAGE_HEAD + body + PAGE_TAIL);
}

static void redirectToRoot(const char* message) {
  server.sendHeader("Location", String("/?m=") + message);
  server.send(303);
}

static void handleRoot() {
  // WiFi 接続中に初めて開いたときは一覧が空なのでスキャンする
  // （AP 中は AP を立てる前にスキャン済み）
  if (getScannedSSIDs().empty() && !isAPMode()) {
    scanWiFiNetworks();
  }

  String html;
  html.reserve(4500);

  String message = server.arg("m");
  if (message == "saved") {
    html += "<p><b>保存しました</b></p>";
  } else if (message == "calib") {
    html += "<p><b>次の更新時に、今の空気を基準値として取り直します（ウォームアップ中なら完了後）</b></p>";
  }

  // 接続状態
  if (isAPMode()) {
    html += "<p>APモード（WiFi未接続）";
    if (settings.wifi_ssid.length() > 0) {
      html += "<br>保存済みの「" + htmlEscape(settings.wifi_ssid) + "」に5分ごとに再接続を試します";
    }
    html += "</p>";
  } else {
    html += "<p>接続中: " + htmlEscape(WiFi.SSID()) + "（" + WiFi.localIP().toString() + "）</p>";
  }

  html += "<form method=post action=/save>";

  // WiFi: 一覧から選んでパスワードを入力。一覧にない SSID は手入力
  html += "<fieldset><legend>WiFi</legend><select name=s>";
  bool current_listed = false;
  for (const String& ssid : getScannedSSIDs()) {
    if (ssid == settings.wifi_ssid) {
      current_listed = true;
    }
  }
  if (!current_listed && settings.wifi_ssid.length() > 0) {
    // 保存済みの SSID が見えていなくても選べるようにする
    html += "<option selected>" + htmlEscape(settings.wifi_ssid) + "</option>";
  }
  for (const String& ssid : getScannedSSIDs()) {
    html += "<option";
    if (ssid == settings.wifi_ssid) {
      html += " selected";
    }
    html += ">" + htmlEscape(ssid) + "</option>";
  }
  html += "</select> <a href=/scan>再スキャン</a>"
          "<input type=text name=s2 placeholder='一覧にない場合はSSIDを入力'>"
          "<input type=password name=p placeholder='パスワード（同じSSIDで変えないなら空欄）'>"
          "</fieldset>";

  // 画面
  html += "<fieldset><legend>画面</legend>";
  for (uint8_t i = 0; i < num_screens; i++) {
    html += "<label><input type=checkbox name=c" + String(i);
    if (settings.screen_mask & (1 << i)) {
      html += " checked";
    }
    html += "> " + String(i) + ": " + htmlEscape(screen_names[i]) + "</label><br>";
  }
  html += "切り替え間隔（秒）<input type=number name=iv min=" + String(INTERVAL_SEC_MIN) +
          " max=" + String(INTERVAL_SEC_MAX) + " value=" + String(settings.interval_sec) + ">"
          "</fieldset>";

  // 時刻
  html += "<fieldset><legend>時刻</legend>"
          "NTPサーバー<input type=text name=ntp value=\"" + htmlEscape(settings.ntp_server) + "\">"
          "タイムゾーン（例: JST-9）<input type=text name=tz value=\"" + htmlEscape(settings.timezone) + "\">"
          "</fieldset>";

  // 天気の地域：市区町村名で候補を探して選ぶ（Enter で送信せずに候補を探す）
  // 検索欄は送信しない。保存するのは選んだ候補の地名（hidden）と緯度・経度
  String current = settings.weather_place.length() > 0 ? htmlEscape(settings.weather_place)
                   : hasWeatherLocation()               ? String("緯度・経度で指定")
                                                        : String("未設定");
  html += "<fieldset><legend>天気の地域</legend>"
          "現在: <b>" + current + "</b>"
          "<input type=text id=wq placeholder='市区町村名（例: 府中市）' "
          "onkeydown=\"if(event.key=='Enter'){event.preventDefault();geo()}\">"
          "<button type=button onclick=geo()>候補を探す</button>"
          "<select id=wr onchange=wpick() hidden></select>"
          "<div id=wm></div>"
          "<input type=hidden id=wn name=wn value=\"" + htmlEscape(settings.weather_place) + "\">"
          "緯度<input type=number step=any min=-90 max=90 id=wla name=wla oninput=wman() value=\"" +
          settings.weather_lat + "\">"
          "経度<input type=number step=any min=-180 max=180 id=wlo name=wlo oninput=wman() value=\"" +
          settings.weather_lon + "\">"
          "<small>候補を探すには、スマホや PC がインターネットにつながっている必要があります"
          "（APモード中は緯度・経度を直接入力）。緯度・経度を空欄にすると未設定に戻ります。"
          "地名の検索: <a href=https://geocode.csis.u-tokyo.ac.jp/>CSISシンプルジオコーディング実験</a>を利用。"
          "天気データ: <a href=https://open-meteo.com/>Open-Meteo.com</a></small>"
          "</fieldset>";

  html += "<button>保存</button></form>";
  html += WEATHER_SCRIPT;

  html += "<fieldset><legend>ガスセンサー（MICS5524）</legend>"
          "<form method=post action=/calib><button>今の空気を基準にする</button></form>"
          "</fieldset>";

  html += "<fieldset><legend>本体</legend>"
          "<form method=post action=/reboot><button>再起動</button></form>"
          "<form method=post action=/reset onsubmit=\"return confirm('設定を初期化して再起動しますか？')\">"
          "<button>初期化</button></form>"
          "</fieldset>";

  sendPage(html);
}

static void handleSave() {
  // WiFi: 手入力があればそちらを優先
  String ssid = server.arg("s2");
  ssid.trim();
  if (ssid.length() == 0) {
    ssid = server.arg("s");
  }
  String password = server.arg("p");
  if (password.length() == 0 && ssid == settings.wifi_ssid) {
    password = settings.wifi_password;  // 同じ SSID でパスワード空欄なら変更しない
  }
  bool wifi_changed = (ssid != settings.wifi_ssid || password != settings.wifi_password);
  settings.wifi_ssid = ssid;
  settings.wifi_password = password;

  // 画面（全部オフにはできないようにする）
  uint16_t mask = 0;
  for (uint8_t i = 0; i < num_screens; i++) {
    if (server.hasArg("c" + String(i))) {
      mask |= 1 << i;
    }
  }
  if (mask != 0) {
    settings.screen_mask = mask;
  }
  settings.interval_sec = constrain(server.arg("iv").toInt(), INTERVAL_SEC_MIN, INTERVAL_SEC_MAX);

  // 時刻（空欄なら変更しない）
  String ntp = server.arg("ntp");
  ntp.trim();
  if (ntp.length() > 0) {
    settings.ntp_server = ntp;
  }
  String tz = server.arg("tz");
  tz.trim();
  if (tz.length() > 0) {
    settings.timezone = tz;
  }

  // 天気の地域（緯度・経度のどちらかが空欄なら未設定に戻す。数値でなければ変更しない）
  String place = server.arg("wn");
  place.trim();
  String lat = server.arg("wla");
  String lon = server.arg("wlo");
  lat.trim();
  lon.trim();
  if (lat.length() == 0 || lon.length() == 0) {
    settings.weather_place = "";
    settings.weather_lat = "";
    settings.weather_lon = "";
  } else if (isCoordinate(lat, 90) && isCoordinate(lon, 180)) {
    settings.weather_place = place;
    settings.weather_lat = lat;
    settings.weather_lon = lon;
  }

  saveSettings();
  applyTimeSettings();
  Serial.println("💾 設定を保存しました");

  if (wifi_changed) {
    // WiFi の変更は再起動して反映する
    sendPage("<p>保存しました。再起動して「" + htmlEscape(ssid) + "」に接続します。</p>"
             "<p>接続できると、本体の画面に新しい設定ページの QR コードが表示されます。"
             "接続できなければ、もう一度 AP モードになります。</p>");
    restart_requested_at = millis();
  } else {
    redirectToRoot("saved");
  }
}

static void handleScan() {
  scanWiFiNetworks();
  server.sendHeader("Location", "/");
  server.send(303);
}

static void handleCalibrate() {
  mics5524_calibration_requested = true;
  redirectToRoot("calib");
}

static void handleReboot() {
  sendPage("<p>再起動します。</p>");
  restart_requested_at = millis();
}

static void handleReset() {
  resetSettings();
  Serial.println("🗑 設定を初期化しました");
  sendPage("<p>設定を初期化しました。再起動します。</p>");
  restart_requested_at = millis();
}

void setupWebSettings(const char* const* names, uint8_t count) {
  screen_names = names;
  num_screens = count;

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/calib", HTTP_POST, handleCalibrate);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });
  server.begin();

  Serial.println("🌐 設定ページ: 起動しました");
}

void webSettingsLoop() {
  server.handleClient();

  if (restart_requested_at != 0 && millis() - restart_requested_at >= RESTART_DELAY_MS) {
    Serial.println("🔄 再起動します");
    ESP.restart();
  }
}
