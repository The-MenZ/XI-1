#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <vector>

// ============================================================================
// WiFi 接続と AP モード
// ============================================================================
// 保存済みの WiFi に接続できないときは、自分が AP（パスワードなし）になり、
// スマホや PC から設定ページ（http://192.168.4.1/）を開けるようにする

// 起動時の接続。未設定か、規定回数続けて失敗したら AP モードに入る
void setupNetwork();

// loop() から毎回呼ぶ（切断の監視と、AP 中の保存済み WiFi への再接続）
void networkLoop();

bool isWiFiConnected();
bool isAPMode();

// AP の SSID（XI-1-XXXX、XXXX は MAC アドレスの末尾）
String getAPSSID();

// 設定ページの URL（AP 中は AP の IP、接続中は自分の IP、どちらでもなければ空文字）
String getSettingsURL();

// 周囲の WiFi をスキャンし、SSID 一覧（信号の強い順・重複なし）を更新する
void scanWiFiNetworks();
const std::vector<String>& getScannedSSIDs();

#endif // NETWORK_H
