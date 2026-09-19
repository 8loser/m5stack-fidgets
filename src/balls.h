#pragma once
#include "common.h"

// ================= 1. 彈珠 =================
// 傾斜給重力、手指排斥球、搖一下裝置球全部散開
namespace balls {
  constexpr int N = 18; constexpr float G = 900, REST = 0.85f;
  struct B { float x, y, vx, vy, r; uint32_t col; } b[N];
  void init() {
    for (int i = 0; i < N; i++) {
      b[i].r = 6 + frand() * 6; b[i].col = hsv(i / (float)N);
      b[i].x = b[i].r + frand() * (W - 2 * b[i].r); b[i].y = b[i].r + frand() * (H - 2 * b[i].r); b[i].vx = b[i].vy = 0;
    }
    cv.fillScreen(0);
  }
  static void hit(float v) { if (v > 200) { snd::note(200 + v); if (v > 500) buzz(120, 25); } }
  void step(const Ctx& c) {
    if (c.shake > 0.8f) { for (auto& o : b) { float a = frand() * 6.283f, v = 300 + c.shake * 300; o.vx += cosf(a) * v; o.vy += sinf(a) * v; } buzz(80, 30); }   // 搖一下:全部往隨機方向噴開
    for (auto& o : b) {
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt;
      if (c.touch) {  // 手指排斥
        float dx = o.x - c.tx, dy = o.y - c.ty, d2 = dx * dx + dy * dy + 400;
        if (d2 < 90 * 90) { float f = 4e6f * c.dt / d2; o.vx += dx / sqrtf(d2) * f; o.vy += dy / sqrtf(d2) * f; }
      }
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      if (o.x < o.r)     { o.x = o.r;     hit(fabsf(o.vx)); o.vx = -o.vx * REST; }
      if (o.x > W - o.r) { o.x = W - o.r; hit(fabsf(o.vx)); o.vx = -o.vx * REST; }
      if (o.y < o.r)     { o.y = o.r;     hit(fabsf(o.vy)); o.vy = -o.vy * REST; }
      if (o.y > H - o.r) { o.y = H - o.r; hit(fabsf(o.vy)); o.vy = -o.vy * REST; }
    }
    for (int i = 0; i < N; i++) for (int j = i + 1; j < N; j++) {  // ponytail: O(n^2),N=18 夠用
      auto& a = b[i]; auto& o = b[j];
      float dx = o.x - a.x, dy = o.y - a.y, d = sqrtf(dx * dx + dy * dy), pen = a.r + o.r - d;
      if (pen <= 0 || d < 1e-3f) continue;
      float nx = dx / d, ny = dy / d;
      a.x -= nx * pen / 2; a.y -= ny * pen / 2; o.x += nx * pen / 2; o.y += ny * pen / 2;
      float rv = (o.vx - a.vx) * nx + (o.vy - a.vy) * ny;
      if (rv < 0) { float j = -(1 + REST) * rv / 2; a.vx -= nx * j; a.vy -= ny * j; o.vx += nx * j; o.vy += ny * j; hit(-rv * 0.7f); }
    }
  }
  void draw() { fadeCanvas(); for (auto& o : b) cv.fillCircle((int)o.x, (int)o.y, (int)o.r, o.col); }
}
