#include "open_meteo.h"
#include <WiFi.h>

#define OPEN_METEO_TIMEOUT_MS 8000  // 接続から受信し終えるまでの上限

bool openMeteoGet(const char* host, const String& path, size_t max_len, const char* label, String& body) {
  WiFiClient client;
  if (!client.connect(host, 80, OPEN_METEO_TIMEOUT_MS)) {
    Serial.printf("   ⚠️  %s: サーバーに接続できません\n", label);
    return false;
  }

  // HTTP/1.0 で頼むと、サーバーは送り終えたら切断する（chunked 形式にならない）
  client.printf("GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path.c_str(), host);

  String response;
  response.reserve(max_len);
  unsigned long start = millis();
  while ((client.connected() || client.available()) && millis() - start < OPEN_METEO_TIMEOUT_MS &&
         response.length() < max_len) {
    if (client.available()) {
      response += (char)client.read();
    } else {
      delay(10);
    }
  }
  client.stop();

  // 1行目は "HTTP/1.0 200 OK"
  if (!response.startsWith("HTTP/1.") || response.substring(9, 12) != "200") {
    Serial.printf("   ⚠️  %s: 応答エラー（%s）\n", label, response.substring(0, response.indexOf('\r')).c_str());
    return false;
  }
  int header_end = response.indexOf("\r\n\r\n");
  if (header_end < 0) {
    Serial.printf("   ⚠️  %s: 応答を読み取れません\n", label);
    return false;
  }
  body = response.substring(header_end + 4);
  return true;
}

// Open-Meteo の応答は形が決まっていて入れ子も浅いので、JSON ライブラリは使わず文字列を探す
const char* openMeteoFindValue(const char* json, const char* object, const char* key, int index) {
  char pattern[40];
  snprintf(pattern, sizeof(pattern), "\"%s\":{", object);
  const char* start = strstr(json, pattern);
  if (start == nullptr) {
    return nullptr;
  }
  const char* end = strchr(start, '}');
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);
  const char* p = strstr(start, pattern);
  if (end == nullptr || p == nullptr || p > end) {
    return nullptr;
  }
  p += strlen(pattern);
  if (*p == '[') {
    p++;
    for (int i = 0; i < index; i++) {
      p = strchr(p, ',');
      if (p == nullptr || p > end) {
        return nullptr;
      }
      p++;
    }
  }
  return p;
}

// 応答の数値は「-12.3」の形だけなので自前で読む（strtof は指数表記なども扱うぶん約5KB大きい）
bool openMeteoReadNumber(const char* json, const char* object, const char* key, int index, float* out) {
  const char* p = openMeteoFindValue(json, object, key, index);
  if (p == nullptr) {
    return false;
  }
  bool negative = *p == '-';
  if (negative) {
    p++;
  }
  if (!isdigit(*p)) {
    return false;
  }
  float value = 0;
  while (isdigit(*p)) {
    value = value * 10 + (*p++ - '0');
  }
  if (*p == '.') {
    p++;
    float scale = 0.1f;
    while (isdigit(*p)) {
      value += (*p++ - '0') * scale;
      scale *= 0.1f;
    }
  }
  *out = negative ? -value : value;
  return true;
}

bool openMeteoReadTime(const char* json, const char* object, const char* key, int index, time_t* out) {
  const char* p = openMeteoFindValue(json, object, key, index);
  if (p == nullptr || !isdigit(*p)) {
    return false;
  }
  time_t value = 0;
  while (isdigit(*p)) {
    value = value * 10 + (*p++ - '0');
  }
  *out = value;
  return true;
}
