// 電腦版截圖用:NVS 換成什麼都不存的空殼,排行與進入次數一律從 0 開始
#pragma once
#include <cstddef>
#include <cstdint>
struct Preferences {
  bool begin(const char*, bool) { return true; }
  bool remove(const char*) { return true; }
  uint16_t getUShort(const char*, uint16_t d = 0) { return d; }
  size_t putUShort(const char*, uint16_t) { return 2; }
  size_t getBytes(const char*, void*, size_t) { return 0; }
  size_t putBytes(const char*, const void*, size_t n) { return n; }
};
