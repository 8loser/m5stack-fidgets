// 所有遊戲共用:校正旋鈕、畫布、Ctx、亂數/顏色、震動、音效、殘影
#pragma once
#include <M5Unified.h>
#include <cmath>
#include <cstring>

// ---- 校正旋鈕(實機上調)----
// 期望:裝置往右傾 gx>0,往自己這側傾 gy>0。方向反了就把號翻過來。
static constexpr float TILT_X    = -1.0f;
static constexpr float TILT_Y    =  1.0f;
static constexpr float TILT_GAIN =  1.0f;   // getAccel 若回 m/s^2 而非 g,改成 1/9.8

constexpr int W = 320, H = 240;
static M5Canvas cv(&M5.Display);
static uint32_t vibUntil = 0;
static bool muted = false;   // 選單預覽時關掉聲音與震動

struct Ctx {
  float gx, gy;   // 重力方向(單位 g);IMU 關閉時固定 (0, 1)
  bool  touch;    // 有手指在螢幕區
  int   tx, ty;
  bool  tap;      // 這一幀剛按下
  float dt;
};

static uint32_t rng = 0x9E3779B9;
static inline float frand() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (rng >> 8) * (1.0f / 16777216.0f); }
static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { return cv.color888(r, g, b); }
static uint32_t hsv(float h) {  // h 0..1, 全飽和
  h = h - floorf(h); float s = h * 6; int i = (int)s; float f = s - i;
  uint8_t q = (uint8_t)(255 * (1 - f)), t = (uint8_t)(255 * f);
  switch (i % 6) { case 0: return rgb(255, t, 0); case 1: return rgb(q, 255, 0); case 2: return rgb(0, 255, t);
                   case 3: return rgb(0, q, 255); case 4: return rgb(t, 0, 255); default: return rgb(255, 0, q); }
}
static void buzz(uint8_t level, uint32_t ms) { if (muted) return; M5.Power.setVibration(level); vibUntil = millis() + ms; }

// 音效。note():鋼琴感的音(6 個諧波、快起音、約 1 秒衰減,高次諧波衰得快),音高吸到 C 大調五聲音階
//       click():撞擊的短促「嗒」聲(3.4 kHz + 1 kHz 阻尼正弦,30 ms),參考影片井字段的頻譜
namespace snd {
  constexpr int SR = 12000, LEN = SR, CSR = 24000, CLEN = CSR * 3 / 100, NBUF = 6;   // click 用較高取樣率才夠脆
  static int16_t* buf[NBUF]; static int16_t* cbuf; static int16_t lut[256];
  static const float PENTA[5] = { 261.63f, 293.66f, 329.63f, 392.00f, 440.00f };
  void init() {
    for (int i = 0; i < 256; i++) lut[i] = (int16_t)(sinf(i * 6.2831853f / 256) * 32767);
    for (auto& b : buf) b = (int16_t*)heap_caps_malloc(LEN * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    cbuf = (int16_t*)heap_caps_malloc(CLEN * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (cbuf) for (int i = 0; i < CLEN; i++) {   // 兩個阻尼正弦(3.4 kHz 主體 + 1 kHz 木質底),像敲擊的「嗒」
      float t = i / (float)CSR;
      cbuf[i] = (int16_t)(12000 * (0.7f * sinf(6.2831853f * 3400 * t) * expf(-150 * t) + 0.3f * sinf(6.2831853f * 1000 * t) * expf(-80 * t)));
    }
  }
  static float snap(float f) {
    float best = f, bd = 1e9f;
    for (int o = -2; o <= 3; o++) for (float n : PENTA) { float q = n * powf(2, o); if (fabsf(q - f) < bd) { bd = fabsf(q - f); best = q; } }
    return best;
  }
  // ponytail: 6 個緩衝 / 聲道輪替,同時第 7 個音會截斷最舊的;50 ms 內的連續撞擊只發第一個音
  void note(float f) {
    static int k = 0; static uint32_t lastMs = 0;
    uint32_t now = millis(); if (muted || now - lastMs < 50) return; lastMs = now;
    k = (k + 1) % NBUF;
    if (!buf[k]) { M5.Speaker.tone(f, 30); return; }
    M5.Speaker.stop(k);
    f = snap(f);
    static const int AMP[6] = { 9000, 5000, 3200, 2200, 1500, 1000 };
    static const float DECAY[6] = { 3.0f, 3.6f, 4.4f, 5.4f, 6.6f, 8.0f };
    uint32_t ph = 0, dph = (uint32_t)(f / SR * 4294967296.0f);
    for (int i = 0; i < LEN; i += 32) {
      float t = i / (float)SR; int e[6]; for (int h = 0; h < 6; h++) e[h] = (int)(AMP[h] * expf(-DECAY[h] * t));
      for (int j = i; j < i + 32 && j < LEN; j++, ph += dph) {
        int32_t v = 0; for (int h = 0; h < 6; h++) v += lut[(ph * (h + 1)) >> 24] * e[h];
        buf[k][j] = v >> 15;
      }
    }
    M5.Speaker.playRaw(buf[k], LEN, SR, false, 1, k, true);
  }
  void click() { if (muted) return; if (cbuf) M5.Speaker.playRaw(cbuf, CLEN, CSR, false, 1, 7, true); else M5.Speaker.tone(3400, 8); }
}

// 每幀把 8-bit(RGB332)畫布整體調暗一級,產生殘影
static uint8_t fadeLut[256];
static void buildFade() {
  for (int c = 0; c < 256; c++) {
    int r = c >> 5, g = (c >> 2) & 7, b = c & 3;
    if (r) r--; if (g) g--; if (b) b--;
    fadeLut[c] = (r << 5) | (g << 2) | b;
  }
}
static void fadeCanvas() { uint8_t* p = (uint8_t*)cv.getBuffer(); for (int i = 0; i < W * H; i++) p[i] = fadeLut[p[i]]; }

