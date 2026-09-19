#pragma once
#include "ringlib.h"

// ================= 9. 方塊填滿場地就變大 =================
// 方形場地裡一個小方塊彈跳,每撞一次牆就長大一點;長到填滿場地,場地就放大一級、方塊重新變小。
// 撞擊點之間連細線當軌跡。傾斜給重力,點螢幕把方塊朝手指踢。場地長到最大後炸開重來
namespace grow {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, NTRAIL = 24, STAGES = 4; constexpr float G = 280, KICK = 220, S0 = 70, SSTEP = 45, GROW = 1.05f;
  static float S, s, x, y, vx, vy, endT; static int stage, bounces, trailN; static bool over;
  static int16_t trail[NTRAIL][2];
  static uint32_t col() { return hsv(0.75f + stage * 0.2f); }
  static void startStage() { s = 12; x = CX; y = CY; float a = frand() * 6.283f; vx = cosf(a) * 150; vy = sinf(a) * 150; trailN = 0; }
  void init() { S = S0; stage = bounces = 0; over = false; endT = 0; startStage(); ringlib::reset(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); stepSparks(c.dt); return; }
    if (c.tap) tapKick(c, x, y, vx, vy, KICK);
    vx += c.gx * G * c.dt; vy += c.gy * G * c.dt;
    float v = sqrtf(vx * vx + vy * vy); if (v < 100 && v > 1e-3f) { vx *= 100 / v; vy *= 100 / v; }
    x += vx * c.dt; y += vy * c.dt;
    float lim = (S - s) / 2; bool hit = false;
    if (x < CX - lim) { x = CX - lim; vx = fabsf(vx); hit = true; } if (x > CX + lim) { x = CX + lim; vx = -fabsf(vx); hit = true; }
    if (y < CY - lim) { y = CY - lim; vy = fabsf(vy); hit = true; } if (y > CY + lim) { y = CY + lim; vy = -fabsf(vy); hit = true; }
    if (hit) {
      bounces++; s *= GROW; trail[trailN++ % NTRAIL][0] = (int16_t)x; trail[(trailN - 1) % NTRAIL][1] = (int16_t)y;
      spark(x, y, col()); snd::note(200 + s * 4); buzz(30, 10);
      if (s >= S - 3) {   // 填滿:場地放大
        if (++stage >= STAGES) { over = true; buzz(200, 150); for (int k = 0; k < 4; k++) spark(CX + (frand() - 0.5f) * S, CY + (frand() - 0.5f) * S, hsv(frand())); }
        else { S += SSTEP; startStage(); snd::note(600); buzz(120, 60); }
      }
    }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    if (!over) {
      uint32_t cc = col(), dim = rgb(50, 50, 60);
      int n = trailN < NTRAIL ? trailN : NTRAIL;   // 最近 n 個撞擊點依序連線
      for (int k = 1; k < n; k++) { auto& a = trail[(trailN - k) % NTRAIL]; auto& p = trail[(trailN - k - 1) % NTRAIL]; cv.drawLine(a[0], a[1], p[0], p[1], dim); }
      cv.drawRect((int)(CX - S / 2), (int)(CY - S / 2), (int)S, (int)S, cc);
      cv.fillRect((int)(x - s / 2), (int)(y - s / 2), (int)s, (int)s, cc);
    }
    drawSparks();
    char t[28]; snprintf(t, sizeof t, "arena %d/%d  bounces %d", stage + 1, STAGES, bounces);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4);
  }
}
