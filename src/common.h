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
// 圓形場地共用:球超出內壁就推回並反彈,回傳撞擊的法向速度(沒撞回 0)
static float wallCircle(float& x, float& y, float& vx, float& vy, float r, float cx, float cy, float R, float rest = 1) {
  float dx = x - cx, dy = y - cy, d = sqrtf(dx * dx + dy * dy) + 1e-3f, lim = R - r;
  if (d < lim) return 0;
  float nx = dx / d, ny = dy / d, vn = vx * nx + vy * ny;
  x = cx + nx * lim; y = cy + ny * lim;
  if (vn <= 0) return 0;
  vx -= (1 + rest) * vn * nx; vy -= (1 + rest) * vn * ny; return vn;
}
// 點螢幕:把物體朝手指方向以速度 K 踢出去
static void tapKick(const Ctx& c, float x, float y, float& vx, float& vy, float K) {
  float dx = c.tx - x, dy = c.ty - y, d = sqrtf(dx * dx + dy * dy) + 1e-3f; vx = dx / d * K; vy = dy / d * K;
}
// 速度長度固定成 s(重力只改方向,不改快慢)
static void setSpeed(float& vx, float& vy, float s) { float v = sqrtf(vx * vx + vy * vy); if (v > 1e-3f) { vx *= s / v; vy *= s / v; } }
static void buzz(uint8_t level, uint32_t ms) { if (muted) return; M5.Power.setVibration(level); vibUntil = millis() + ms; }

// 音效。note():鋼琴感的音(6 個諧波、快起音、約 1 秒衰減,高次諧波衰得快),音高吸到 C 大調五聲音階
//       click():撞擊的低沉「咚」聲(500 Hz 主體 + 150 Hz 底 + 750 Hz 泛音,約 35 ms 衰 20 dB)。
//               量自參考影片井字段(43 到 48 秒)的牆壁撞擊:99% 能量在 300 到 900 Hz,2 kHz 以上幾乎沒有
namespace snd {
  constexpr int SR = 12000, LEN = SR, CSR = 12000, CLEN = CSR * 8 / 100, NBUF = 6;   // click 80 ms
  static int16_t* buf[NBUF]; static int16_t cbuf[CLEN]; static int16_t lut[256];   // cbuf 才 2 KB,放內部 RAM 不依賴 PSRAM
  static const float PENTA[5] = { 261.63f, 293.66f, 329.63f, 392.00f, 440.00f };
  void init() {
    for (int i = 0; i < 256; i++) lut[i] = (int16_t)(sinf(i * 6.2831853f / 256) * 32767);
    for (auto& b : buf) b = (int16_t*)heap_caps_malloc(LEN * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < CLEN; i++) {   // 三個阻尼正弦;Core2 喇叭低頻弱,振幅 25000 已接近上限(三項合計 1.26 倍),還嫌小聲就提高 500 Hz 或 main.cpp 的 setVolume
      float t = i / (float)CSR, w = 6.2831853f;
      cbuf[i] = (int16_t)(25000 * (sinf(w * 500 * t) * expf(-65 * t) + 0.18f * sinf(w * 150 * t) * expf(-50 * t) + 0.08f * sinf(w * 750 * t) * expf(-90 * t)));
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
    static const int AMP[6] = { 3400, 1900, 1200, 830, 560, 375 };   // 原 9000..1000 乘 48/128,讓 note 在 setVolume(128) 時等於原音量
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
  void click() { if (!muted) M5.Speaker.playRaw(cbuf, CLEN, CSR, false, 1, 7, true); }
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

