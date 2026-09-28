#pragma once
#include "ringlib.h"

// ================= 6. 圓球溢出 =================
// 圓形場地開了一道缺口,小球在裡面彈跳;每逃出一個就在場內生 3 個,直到塞滿溢出。
// 傾斜給重力,A/C 轉動缺口,搖一下全部往隨機方向噴開,點螢幕把手指附近的球推開
namespace overflow {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int MAXT = 300, TR = 5, CX = 160, CY = 120, RAD = 106; constexpr float G = 220, REST = 0.5f, GAP = 15, SPIN = 120;   // 缺口寬 15 度,A/C 每秒轉 120 度
  struct T { float x, y, vx, vy; uint32_t col; bool live, out; } static t[MAXT];
  static int escaped; static bool over; static float endT, rot;   // rot = 缺口中心角(度)
  static void spawn(float x, float y) {
    for (auto& o : t) if (!o.live) { float a = frand() * 6.283f; o = { x, y, cosf(a) * 120, sinf(a) * 120, hsv(frand()), true, false }; return; }
  }
  void init() { memset(t, 0, sizeof t); escaped = 0; over = false; endT = 0; rot = -45; ringlib::reset(); spawn(160, 120); cv.fillScreen(0); }
  void step(const Ctx& c) {
    int live = 0; rot += (c.btnC - c.btnA) * SPIN * c.dt;
    if (c.shake > 0.8f) { for (auto& o : t) if (o.live && !o.out) { float a = frand() * 6.283f, v = 300 + c.shake * 300; o.vx += cosf(a) * v; o.vy += sinf(a) * v; } buzz(80, 30); }   // 同遊戲 1
    for (auto& o : t) if (o.live) {
      live++;
      if (c.tap) { float dx = o.x - c.tx, dy = o.y - c.ty, d2 = dx * dx + dy * dy + 100; if (d2 < 70 * 70) { float f = 2.5e5f / d2; o.vx += dx / sqrtf(d2) * f; o.vy += dy / sqrtf(d2) * f; } }
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      if (o.out || over) { if (o.x < -20 || o.x > W + 20 || o.y < -20 || o.y > H + 20) o.live = false; continue; }
      float dx = o.x - CX, dy = o.y - CY, d = sqrtf(dx * dx + dy * dy);
      if (d < RAD - TR) continue;
      if (fabsf(fmodf(atan2f(dy, dx) * RAD_TO_DEG - rot + 540, 360) - 180) < GAP / 2) {   // 在缺口的角度內:放它出去
        if (d > RAD + TR) { o.out = true; escaped++; spark(o.x, o.y, o.col); snd::note(400 + frand() * 200); buzz(50, 15); for (int k = 0; k < 3; k++) spawn(CX + (frand() - 0.5f) * 80, CY + (frand() - 0.5f) * 60); }
        continue;
      }
      float vn = wallCircle(o.x, o.y, o.vx, o.vy, TR, CX, CY, RAD, REST);
      if (live < 12 && vn > 80) snd::click();
    }
    if (!over && live >= MAXT - 3) { over = true; buzz(200, 150); for (auto& o : t) if (o.live) { o.out = true; o.vy = 200 + frand() * 100; } }
    if ((over && (endT += c.dt) > 3) || (!live && escaped)) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    uint32_t wc = hsv(0.95f + escaped * 0.01f);
    if (!over) ringlib::arc(CX, CY, RAD, rot + GAP / 2, rot + 360 - GAP / 2, wc);
    for (auto& o : t) if (o.live) cv.fillCircle((int)o.x, (int)o.y, TR, o.col);
    drawSparks();
    char s[20]; snprintf(s, sizeof s, "escaped %d", escaped);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
