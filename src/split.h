#pragma once
#include "ringlib.h"

// ================= 11. 細胞分裂 =================
// 紅藍兩隊細胞在圓形培養皿裡,每顆過一段時間就分裂成兩顆、互相推擠;塞滿時多的那隊贏。
// 傾斜給重力,點螢幕讓手指附近的細胞立刻分裂(幫你支持的那隊加速)
namespace split {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, RAD = 106, MAXC = 150; constexpr float G = 90, CR = 6;
  struct C { float x, y, vx, vy, t; int8_t team; bool live; } static cells[MAXC];
  static int cnt[2]; static bool over; static float endT;
  static uint32_t col(int team) { return team ? rgb(60, 120, 255) : rgb(255, 50, 110); }
  static void spawn(float x, float y, int team) {
    for (auto& o : cells) if (!o.live) { o = { x, y, 0, 0, 1.5f + frand() * 2.0f, (int8_t)team, true }; cnt[team]++; return; }
  }
  static void divide(C& o) { float a = frand() * 6.283f; o.t = 1.5f + frand() * 2.0f; spawn(o.x + cosf(a) * CR, o.y + sinf(a) * CR, o.team); snd::note(o.team ? 330 : 440); }
  void init() { memset(cells, 0, sizeof cells); cnt[0] = cnt[1] = 0; over = false; endT = 0; ringlib::reset(); spawn(CX - 30, CY, 0); spawn(CX + 30, CY, 1); cv.fillScreen(0); }
  void step(const Ctx& c) {
    int live = cnt[0] + cnt[1];
    for (auto& o : cells) if (o.live) {
      if (!over && live < MAXC && ((o.t -= c.dt) <= 0 || (c.tap && fabsf(o.x - c.tx) < 30 && fabsf(o.y - c.ty) < 30))) { divide(o); live++; }
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt; o.vx *= 0.9f; o.vy *= 0.9f;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      if (over) continue;
      wallCircle(o.x, o.y, o.vx, o.vy, CR, CX, CY, RAD, 0.2f);
    }
    if (!over) for (int i = 0; i < MAXC; i++) if (cells[i].live) for (int j = i + 1; j < MAXC; j++) if (cells[j].live) {   // ponytail: O(n^2),n<=150 約 11k 對,實測夠用
      auto& p = cells[i]; auto& q = cells[j];
      float dx = q.x - p.x, dy = q.y - p.y, d2 = dx * dx + dy * dy;
      if (d2 >= 4 * CR * CR || d2 < 1e-3f) continue;
      float d = sqrtf(d2), pen = (2 * CR - d) / 2, nx = dx / d, ny = dy / d;
      p.x -= nx * pen; p.y -= ny * pen; q.x += nx * pen; q.y += ny * pen;
    }
    if (!over && live >= MAXC) { over = true; buzz(200, 150); int w = cnt[1] > cnt[0]; for (int k = 0; k < 6; k++) spark(CX + (frand() - 0.5f) * 150, CY + (frand() - 0.5f) * 150, col(w)); }
    if (over && (endT += c.dt) > 3) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    cv.drawCircle(CX, CY, RAD, rgb(200, 200, 200));
    for (auto& o : cells) if (o.live) { cv.fillCircle((int)o.x, (int)o.y, (int)CR, col(o.team)); cv.fillCircle((int)o.x, (int)o.y, 2, rgb(255, 255, 255)); }
    int tot = cnt[0] + cnt[1]; if (!tot) tot = 1;   // 底部紅藍比例條
    int w0 = W * cnt[0] / tot; cv.fillRect(0, H - 4, w0, 4, col(0)); cv.fillRect(w0, H - 4, W - w0, 4, col(1));
    char s[24]; snprintf(s, sizeof s, "%d : %d", cnt[0], cnt[1]);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
    if (over) { cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(col(cnt[1] > cnt[0]), 0); cv.drawString(cnt[1] > cnt[0] ? "BLUE" : "RED", CX, CY); }
    drawSparks();
  }
}
