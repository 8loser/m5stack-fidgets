#pragma once
#include "common.h"

// ================= 2. 彈珠台(高爾頓板)=================
namespace plinko {
  constexpr int ROWS = 8, DX = 28, DY = 20, TOP = 36, R = 3, PR = 3, BINS = 11, BW = W / BINS, BIN_TOP = TOP + ROWS * DY + 8;
  constexpr int MAXB = 60; constexpr float G = 600;
  struct B { float x, y, vx, vy; bool live; } b[MAXB];
  static int bin[BINS]; static uint32_t lastSpawn = 0;
  void init() { memset(b, 0, sizeof b); memset(bin, 0, sizeof bin); cv.fillScreen(0); }
  static void spawn(float x) {
    for (auto& o : b) if (!o.live) { o.live = true; o.x = x + frand() * 4 - 2; o.y = TOP - 16; o.vx = 0; o.vy = 0; return; }
  }
  void step(const Ctx& c) {
    uint32_t now = millis();
    if (now - lastSpawn > 220) { lastSpawn = now; spawn(W / 2); }
    if (c.tap && c.ty < BIN_TOP) for (int i = 0; i < 5; i++) spawn(c.tx);
    float gx = c.gy > 0.05f ? c.gx * 0.6f : 0;   // 只拿傾斜當左右偏壓,下落永遠向下
    for (auto& o : b) {
      if (!o.live) continue;
      o.vx += gx * G * c.dt; o.vy += G * c.dt;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      for (int r = 0; r < ROWS; r++) {  // 只查上下相鄰兩排
        float py = TOP + r * DY; if (fabsf(o.y - py) > R + PR + 1) continue;
        int n = r + 3;
        for (int j = 0; j < n; j++) {
          float px = W / 2.0f + (j - (n - 1) / 2.0f) * DX, dx = o.x - px, dy = o.y - py, d = sqrtf(dx * dx + dy * dy);
          if (d >= R + PR || d < 1e-3f) continue;
          float nx = dx / d, ny = dy / d, vn = o.vx * nx + o.vy * ny;
          o.x = px + nx * (R + PR); o.y = py + ny * (R + PR);
          if (vn < 0) { o.vx -= 1.5f * vn * nx; o.vy -= 1.5f * vn * ny; o.vx += (frand() - 0.5f) * 30; }
        }
      }
      if (o.x < R) { o.x = R; o.vx = -o.vx * 0.5f; } if (o.x > W - R) { o.x = W - R; o.vx = -o.vx * 0.5f; }
      if (o.y > BIN_TOP) {
        int k = (int)(o.x / BW); if (k >= BINS) k = BINS - 1;
        o.live = false; bin[k]++; snd::note(400 + k * 60);
        if (bin[k] * 2 >= H - BIN_TOP) { memset(bin, 0, sizeof bin); buzz(150, 60); }
      }
    }
  }
  void draw() {
    cv.fillScreen(0);
    for (int r = 0; r < ROWS; r++) { int n = r + 3; for (int j = 0; j < n; j++) cv.fillCircle((int)(W / 2.0f + (j - (n - 1) / 2.0f) * DX), TOP + r * DY, PR, rgb(140, 140, 160)); }
    for (int k = 0; k < BINS; k++) {
      cv.drawFastVLine(k * BW, BIN_TOP, H - BIN_TOP, rgb(60, 60, 80));
      int h = bin[k] * 2; if (h) cv.fillRect(k * BW + 2, H - h, BW - 3, h, hsv(k / (float)BINS));
    }
    for (auto& o : b) if (o.live) cv.fillCircle((int)o.x, (int)o.y, R, rgb(255, 230, 90));
  }
}
