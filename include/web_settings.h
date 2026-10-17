#ifndef WEB_SETTINGS_H
#define WEB_SETTINGS_H

#include <Arduino.h>

// ============================================================================
// 設定ページ（Web サーバー）
// ============================================================================
// AP 中は http://192.168.4.1/、WiFi 接続中は http://<自分のIP>/ で開ける

// screen_names: 画面ごとの表示 ON/OFF のチェックボックスに出す名前
void setupWebSettings(const char* const* screen_names, uint8_t num_screens);

// loop() から毎回呼ぶ（リクエスト処理と、保存後の再起動）
void webSettingsLoop();

// 設定ページから MICS5524 の再キャリブレーションが要求されたら true
// （main.cpp がウォームアップ後に実行して false に戻す）
extern bool mics5524_calibration_requested;

// 今の騒音レベル（MAX9814。本体が測った dB）。設定ページで静かなときの騒音レベルを決める目安に出す
// （main.cpp がマイクを読むたびに入れる。NAN はまだ測っていない）
extern float current_sound_db;

#endif // WEB_SETTINGS_H
