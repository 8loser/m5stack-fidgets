// 電腦版截圖用:補上遊戲用到的 ESP32 / Arduino 函式(編譯時 -include)
#pragma once
#include <M5Unified.h>
#include <cstdlib>
#define MALLOC_CAP_SPIRAM 0
static inline void* heap_caps_malloc(size_t n, int) { return malloc(n); }
using m5gfx::millis;
#define DEG_TO_RAD 0.017453292519943295f
#define RAD_TO_DEG 57.29577951308232f
#define PI 3.1415926535897932384626433832795
#define sq(x) ((x) * (x))
#include <algorithm>
using std::min; using std::max;
