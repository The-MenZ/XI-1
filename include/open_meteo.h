#ifndef OPEN_METEO_H
#define OPEN_METEO_H

#include <Arduino.h>

// ============================================================================
// Open-Meteo（https://open-meteo.com/）からの取得（天気と潮位で共用）
// ============================================================================
// API キーが不要で、HTTP（TLS なし）で取れるので TLS のライブラリを使わずに済む

// host の path を HTTP で取得し、応答の本文を body に入れる（失敗したら false）。
// label はシリアルに出すログの名前（例: "天気"）。max_len を超える応答は途中で打ち切る
bool openMeteoGet(const char* host, const String& path, size_t max_len, const char* label, String& body);

// json の "object":{...} の中から "key": の値の先頭を返す（値が配列なら index 番目）
const char* openMeteoFindValue(const char* json, const char* object, const char* key, int index);

// 小数の値を読む（null などで読めなければ false）
bool openMeteoReadNumber(const char* json, const char* object, const char* key, int index, float* out);

// 整数の値を読む（timeformat=unixtime の時刻など。float では桁が足りないので別にする）
bool openMeteoReadTime(const char* json, const char* object, const char* key, int index, time_t* out);

#endif // OPEN_METEO_H
