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
  html.reserve(3000);

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

  // 気圧（Pressure 画面の H/L は海面の高さに換算した気圧で判定する）
  html += "<fieldset><legend>気圧</legend>"
          "設置場所の標高（m。建物の上の階なら階の高さも足す）<input type=number name=alt min=" +
          String(ALTITUDE_M_MIN) + " max=" + String(ALTITUDE_M_MAX) + " value=" + String(settings.altitude_m) + ">"
          "</fieldset>";

  html += "<button>保存</button></form>";

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
  settings.altitude_m = constrain(server.arg("alt").toInt(), ALTITUDE_M_MIN, ALTITUDE_M_MAX);

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
