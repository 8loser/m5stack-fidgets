#pragma once
#include "ringlib.h"

// ================= 16. 收縮的五角形 =================
// 旋轉的五角形牆一直往內縮,兩顆球在裡面彈跳、拖著長尾巴;縮到太小就炸開重來。
// 傾斜給重力,點螢幕把兩顆球都朝手指踢
namespace penta {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int NB = 2, BALL_R = 5; constexpr float CX = 160, CY = 120, RMAX0 = 100, RMIN = 22, SHRINK = 3.0f, OMEGA = 0.7f, G = 260, KICK = 240, VMIN = 140;
  struct B { float x, y, vx, vy; uint32_t col; } static b[NB];
  static float R, th, endT; static int bounces; static bool over;
  void init() {
    R = RMAX0; th = 0; bounces = 0; over = false; endT = 0;
    for (int i = 0; i < NB; i++) { float a = frand() * 6.283f; b[i] = { CX + (frand() - 0.5f) * 40, CY + (frand() - 0.5f) * 40, cosf(a) * 200, sinf(a) * 200, i ? rgb(255, 90, 60) : rgb(60, 220, 255) }; }
    ringlib::reset(); cv.fillScreen(0);
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); stepSparks(c.dt); return; }
    th += OMEGA * c.dt; R -= SHRINK * c.dt;
    for (auto& o : b) {
      if (c.tap) tapKick(c, o.x, o.y, o.vx, o.vy, KICK);
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt;
      float v = sqrtf(o.vx * o.vx + o.vy * o.vy); if (v < VMIN && v > 1e-3f) { o.vx *= VMIN / v; o.vy *= VMIN / v; }
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      float apo = R * cosf(0.6283f);   // 五角形內切圓半徑
      for (int e = 0; e < 5; e++) {   // 每條邊一個半平面,法向朝內
        float a = th + e * 1.2566f + 0.6283f, nx = -cosf(a), ny = -sinf(a);   // 邊中點方向 a,朝內的法向是反方向
        float s = (o.x - CX) * nx + (o.y - CY) * ny + apo;   // 到邊的距離(內側為正)
        if (s >= BALL_R) continue;
        float vn = o.vx * nx + o.vy * ny; o.x += nx * (BALL_R - s); o.y += ny * (BALL_R - s);
        if (vn < 0) { o.vx -= 2 * vn * nx; o.vy -= 2 * vn * ny; bounces++; spark(o.x, o.y, o.col); snd::note(200 + (RMAX0 - R) * 6); }
      }
    }
    if (R < RMIN) { over = true; buzz(200, 150); for (int k = 0; k < 6; k++) spark(CX, CY, hsv(frand())); }
    stepSparks(c.dt);
  }
  void draw() {
    fadeCanvas();
    uint32_t wc = hsv(0.05f + (RMAX0 - R) / RMAX0 * 0.6f);
    if (!over) for (int e = 0; e < 5; e++) {
      float a0 = th + e * 1.2566f, a1 = a0 + 1.2566f;
      cv.drawLine((int)(CX + R * cosf(a0)), (int)(CY + R * sinf(a0)), (int)(CX + R * cosf(a1)), (int)(CY + R * sinf(a1)), wc);
    }
    if (!over) for (auto& o : b) cv.fillCircle((int)o.x, (int)o.y, BALL_R, o.col);
    drawSparks();
    cv.fillRect(0, 0, 120, 12, 0);
    char s[20]; snprintf(s, sizeof s, "bounces %d", bounces);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
