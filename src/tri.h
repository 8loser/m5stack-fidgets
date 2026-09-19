#pragma once
#include "ringlib.h"

// ================= 7. 三角形溢出 =================
// 大三角形場地的右邊開了一道缺口,小三角形在裡面彈跳;每逃出一個就在場內生 3 個,直到塞滿溢出。
// 傾斜給重力,點螢幕把手指附近的三角形推開
namespace tri {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int MAXT = 90, TR = 5; constexpr float G = 220, REST = 0.9f, GAP0 = 0.62f, GAP1 = 0.80f;   // 缺口在邊 CA(從 C 往 A)的 62% 到 80% 處,靠近右上
  static const float VX[3] = { 160, 60, 260 }, VY[3] = { 32, 206, 206 };   // A 頂、B 左下、C 右下
  struct T { float x, y, vx, vy, rot, vrot; uint32_t col; bool live, out; } static t[MAXT];
  static int escaped; static bool over; static float endT;
  static void spawn(float x, float y) {
    for (auto& o : t) if (!o.live) { float a = frand() * 6.283f; o = { x, y, cosf(a) * 120, sinf(a) * 120, frand() * 6.283f, (frand() - 0.5f) * 6, hsv(frand()), true, false }; return; }
  }
  void init() { memset(t, 0, sizeof t); escaped = 0; over = false; endT = 0; ringlib::reset(); spawn(160, 120); cv.fillScreen(0); }
  void step(const Ctx& c) {
    int live = 0;
    for (auto& o : t) if (o.live) {
      live++;
      if (c.tap) { float dx = o.x - c.tx, dy = o.y - c.ty, d2 = dx * dx + dy * dy + 100; if (d2 < 70 * 70) { float f = 2.5e5f / d2; o.vx += dx / sqrtf(d2) * f; o.vy += dy / sqrtf(d2) * f; } }
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt;
      float v = sqrtf(o.vx * o.vx + o.vy * o.vy); if (!o.out && v < 60 && v > 1e-3f) { o.vx *= 60 / v; o.vy *= 60 / v; }
      o.x += o.vx * c.dt; o.y += o.vy * c.dt; o.rot += o.vrot * c.dt;
      if (o.out || over) { if (o.x < -20 || o.x > W + 20 || o.y < -20 || o.y > H + 20) o.live = false; continue; }
      for (int e = 0; e < 3; e++) {   // 三條邊各是一個半平面,法向朝內
        int i = e, j = (e + 1) % 3; float ex = VX[j] - VX[i], ey = VY[j] - VY[i], el = sqrtf(ex * ex + ey * ey), nx = ey / el, ny = -ex / el;   // 頂點順時針(螢幕座標)時法向朝內
        float s = (o.x - VX[i]) * nx + (o.y - VY[i]) * ny;
        if (s >= TR) continue;
        float u = ((o.x - VX[i]) * ex + (o.y - VY[i]) * ey) / (el * el);
        if (e == 2 && u > GAP0 && u < GAP1) {   // 邊 CA 的缺口:放它出去
          if (s < -TR) { o.out = true; escaped++; spark(o.x, o.y, o.col); snd::note(400 + frand() * 200); buzz(50, 15); for (int k = 0; k < 3; k++) spawn(160 + (frand() - 0.5f) * 80, 120 + (frand() - 0.5f) * 60); }
          continue;
        }
        float vn = o.vx * nx + o.vy * ny; o.x += nx * (TR - s); o.y += ny * (TR - s);
        if (vn < 0) { o.vx -= (1 + REST) * vn * nx; o.vy -= (1 + REST) * vn * ny; o.vrot = (frand() - 0.5f) * 8; if (live < 12 && -vn > 80) snd::click(); }
      }
    }
    if (!over && live >= MAXT - 3) { over = true; buzz(200, 150); for (auto& o : t) if (o.live) { o.out = true; o.vy = 200 + frand() * 100; } }
    if ((over && (endT += c.dt) > 3) || (!live && escaped)) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    uint32_t wc = hsv(0.95f + escaped * 0.01f);
    cv.drawLine((int)VX[0], (int)VY[0], (int)VX[1], (int)VY[1], wc); cv.drawLine((int)VX[1], (int)VY[1], (int)VX[2], (int)VY[2], wc);
    if (!over) { cv.drawLine((int)VX[2], (int)VY[2], (int)(VX[2] + (VX[0] - VX[2]) * GAP0), (int)(VY[2] + (VY[0] - VY[2]) * GAP0), wc);
                 cv.drawLine((int)(VX[2] + (VX[0] - VX[2]) * GAP1), (int)(VY[2] + (VY[0] - VY[2]) * GAP1), (int)VX[0], (int)VY[0], wc); }
    for (auto& o : t) if (o.live) {
      int px[3], py[3]; for (int k = 0; k < 3; k++) { float a = o.rot + k * 2.0944f; px[k] = (int)(o.x + TR * cosf(a)); py[k] = (int)(o.y + TR * sinf(a)); }
      cv.fillTriangle(px[0], py[0], px[1], py[1], px[2], py[2], o.col);
    }
    drawSparks();
    char s[20]; snprintf(s, sizeof s, "escaped %d", escaped);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
