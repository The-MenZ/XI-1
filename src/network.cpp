#include "network.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <algorithm>
#include "settings.h"

// 起動時: CONNECT_ATTEMPT_MS 待つ接続を CONNECT_ATTEMPTS 回試し、だめなら AP モード
#define CONNECT_ATTEMPTS 3
#define CONNECT_ATTEMPT_MS (10 * 1000UL)

// 稼働中に切断されたら、この時間は自動再接続に任せ、それでも戻らなければ AP モード
#define LOST_TO_AP_MS (5 * 60 * 1000UL)

// AP 中に保存済み WiFi への再接続を試す間隔と、1回の待ち時間
// （停電後にルーターの起動が遅れたときなどに、自動で元に戻るため）
#define AP_RETRY_INTERVAL_MS (5 * 60 * 1000UL)
#define AP_RETRY_TIMEOUT_MS (15 * 1000UL)

static bool ap_mode = false;
static bool ap_retrying = false;
static unsigned long last_ap_retry = 0;
static unsigned long disconnected_since = 0;  // 切断に気づいた時刻（0 = 接続中）
static std::vector<String> scanned_ssids;

static void onConnected() {
  Serial.println("\n✓ WiFi接続成功");
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

  // TX Power設定（8.5dB減衰 = 11.5dBm出力）
  esp_wifi_set_max_tx_power(46);

  // TX Power設定の確認
  int8_t tx_power;
  esp_wifi_get_max_tx_power(&tx_power);
  Serial.printf("📡 TX Power Setting: %d (≈ %.1f dBm)\n", tx_power, tx_power / 4.0);
}

static void startAP() {
  Serial.println("\n=== APモード開始 ===");

  // AP 中に STA が勝手に再接続を試すと AP のチャンネルが変わり、
  // 設定中のスマホが切断されるので、再接続は networkLoop() で制御する
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);  // STA はスキャンと再接続の確認に使う

  // AP を立てる前にスキャンしておく（AP 中のスキャンはスマホとの通信を一瞬止める）
  scanWiFiNetworks();

  WiFi.softAP(getAPSSID().c_str());  // パスワードなし
  ap_mode = true;
  ap_retrying = false;
  last_ap_retry = millis();

  Serial.printf("SSID: %s（パスワードなし）\n", getAPSSID().c_str());
  Serial.printf("設定ページ: %s\n", getSettingsURL().c_str());
}

static void stopAP() {
  WiFi.softAPdisconnect(true);  // AP を止めて STA のみに戻す
  WiFi.setAutoReconnect(true);
  ap_mode = false;
  disconnected_since = 0;
  Serial.println("=== APモード終了 ===");
}

void setupNetwork() {
  Serial.println("\n=== WiFi接続開始 ===");

  if (settings.wifi_ssid.length() == 0) {
    Serial.println("WiFi未設定");
    startAP();
    return;
  }

  Serial.printf("SSID: %s\n", settings.wifi_ssid.c_str());
  WiFi.mode(WIFI_STA);

  for (int attempt = 1; attempt <= CONNECT_ATTEMPTS; attempt++) {
    WiFi.begin(settings.wifi_ssid.c_str(), settings.wifi_password.c_str());

    unsigned long start = millis();
    while (!isWiFiConnected() && millis() - start < CONNECT_ATTEMPT_MS) {
      delay(500);
      Serial.print(".");
    }

    if (isWiFiConnected()) {
      onConnected();
      return;
    }

    Serial.printf("\n✗ WiFi接続失敗（%d/%d）\n", attempt, CONNECT_ATTEMPTS);
    WiFi.disconnect();
    delay(100);
  }

  startAP();
}

void networkLoop() {
  unsigned long now = millis();

  if (!ap_mode) {
    if (isWiFiConnected()) {
      disconnected_since = 0;
    } else if (disconnected_since == 0) {
      disconnected_since = now;
      Serial.println("⚠ WiFi切断（自動で再接続を試みます）");
    } else if (now - disconnected_since >= LOST_TO_AP_MS) {
      Serial.println("⚠ WiFiに復帰できないため APモードに切り替えます");
      startAP();
    }
    return;
  }

  // ここから AP 中: 保存済み WiFi への再接続を試す
  if (ap_retrying) {
    if (isWiFiConnected()) {
      ap_retrying = false;
      stopAP();
      onConnected();
    } else if (now - last_ap_retry >= AP_RETRY_TIMEOUT_MS) {
      ap_retrying = false;
      last_ap_retry = now;
      WiFi.disconnect();
      Serial.println("✗ 再接続失敗（APモードを継続）");
    }
    return;
  }

  // スマホが AP につながっている間は試さない（チャンネルが変わって切れるため）
  if (settings.wifi_ssid.length() > 0 && WiFi.softAPgetStationNum() == 0 &&
      now - last_ap_retry >= AP_RETRY_INTERVAL_MS) {
    Serial.printf("\n🔄 保存済みWiFi（%s）への再接続を試行\n", settings.wifi_ssid.c_str());
    WiFi.begin(settings.wifi_ssid.c_str(), settings.wifi_password.c_str());
    ap_retrying = true;
    last_ap_retry = now;
  }
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool isAPMode() {
  return ap_mode;
}

String getAPSSID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[16];
  snprintf(ssid, sizeof(ssid), "XI-1-%02X%02X", mac[4], mac[5]);
  return String(ssid);
}

String getSettingsURL() {
  if (ap_mode) {
    return "http://" + WiFi.softAPIP().toString() + "/";
  }
  if (isWiFiConnected()) {
    return "http://" + WiFi.localIP().toString() + "/";
  }
  return "";
}

void scanWiFiNetworks() {
  Serial.print("📡 WiFiスキャン中...");
  int n = WiFi.scanNetworks();

  // 信号の強い順に並べる
  std::vector<int> order;
  for (int i = 0; i < n; i++) {
    order.push_back(i);
  }
  std::sort(order.begin(), order.end(), [](int a, int b) { return WiFi.RSSI(a) > WiFi.RSSI(b); });

  scanned_ssids.clear();
  for (int i : order) {
    String ssid = WiFi.SSID(i);
    // ステルス SSID と、同じ SSID の別のアクセスポイント（中継器など）は除く
    if (ssid.length() == 0 ||
        std::find(scanned_ssids.begin(), scanned_ssids.end(), ssid) != scanned_ssids.end()) {
      continue;
    }
    scanned_ssids.push_back(ssid);
  }
  WiFi.scanDelete();

  Serial.printf("✅ %u件\n", (unsigned)scanned_ssids.size());
}

const std::vector<String>& getScannedSSIDs() {
  return scanned_ssids;
}
