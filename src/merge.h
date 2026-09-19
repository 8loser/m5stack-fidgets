#pragma once
#include "ringlib.h"

// ================= 8. 合成圓 =================
// 圓形場地裡不斷掉下多邊形,兩個相同的碰到就合成邊數多一級的(三角→四角→五角→六角→圓)。
// 傾斜給重力,點螢幕在手指處丟一個三角形。生出第 3 個圓或場地塞滿就炸開重來
namespace merge {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, RAD = 106, MAXB = 40, LV = 5; constexpr float G = 320, REST = 0.4f;
  static const float RADIUS[LV] = { 7, 10, 14, 19, 26 };
  static const uint8_t PAL[LV][3] = { {255,200,0},{0,220,120},{60,140,255},{255,60,160},{255,80,40} };
  struct B { float x, y, vx, vy, rot; int8_t lv; bool live; } static b[MAXB];
  static int merges, circles; static bool over; static float endT, dropT;
  static uint32_t col(int lv) { return rgb(PAL[lv][0], PAL[lv][1], PAL[lv][2]); }
  static void spawn(float x, float y, int lv) {
    for (auto& o : b) if (!o.live) { o = { x, y, (frand() - 0.5f) * 40, 0, frand() * 6.283f, (int8_t)lv, true }; return; }
  }
  void init() { memset(b, 0, sizeof b); merges = circles = 0; over = false; endT = dropT = 0; ringlib::reset(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (!over && (dropT += c.dt) > 1.0f) { dropT = 0; spawn(CX + (frand() - 0.5f) * 120, CY - RAD + 12, frand() < 0.7f ? 0 : 1); }
    if (!over && c.tap) { float dx = c.tx - CX, dy = c.ty - CY; if (dx * dx + dy * dy < (RAD - 10) * (RAD - 10)) spawn(c.tx, c.ty, 0); }
    int live = 0;
    for (auto& o : b) if (o.live) {
      live++;
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt; o.vx *= 0.995f; o.vy *= 0.995f;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt; o.rot += o.vx * 0.01f * c.dt;
      if (over) continue;
      if (wallCircle(o.x, o.y, o.vx, o.vy, RADIUS[o.lv], CX, CY, RAD, REST) > 120) snd::click();
    }
    if (!over) for (int i = 0; i < MAXB; i++) if (b[i].live) for (int j = i + 1; j < MAXB; j++) if (b[j].live) {   // ponytail: O(n^2),n<=40
      auto& p = b[i]; auto& q = b[j];
      float dx = q.x - p.x, dy = q.y - p.y, d = sqrtf(dx * dx + dy * dy), rr = RADIUS[p.lv] + RADIUS[q.lv];
      if (d >= rr || d < 1e-3f) continue;
      if (p.lv == q.lv && p.lv < LV - 1) {   // 合成
        p.lv++; p.x = (p.x + q.x) / 2; p.y = (p.y + q.y) / 2; p.vx = (p.vx + q.vx) / 2; p.vy = (p.vy + q.vy) / 2; q.live = false;
        merges++; spark(p.x, p.y, col(p.lv)); snd::note(250 + p.lv * 80); buzz(40 + p.lv * 20, 20);
        if (p.lv == LV - 1 && ++circles >= 3) { over = true; buzz(200, 150); for (auto& o : b) if (o.live) { float ex = o.x - CX, ey = o.y - CY, e = sqrtf(ex * ex + ey * ey) + 1; o.vx = ex / e * 300; o.vy = ey / e * 300; } }
        continue;
      }
      float nx = dx / d, ny = dy / d, pen = rr - d, mp = RADIUS[q.lv] / rr, mq = RADIUS[p.lv] / rr;   // 大的少動
      p.x -= nx * pen * mp; p.y -= ny * pen * mp; q.x += nx * pen * mq; q.y += ny * pen * mq;
      float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny;
      if (rv < 0) { float jn = -(1 + REST) * rv; p.vx -= nx * jn * mp; p.vy -= ny * jn * mp; q.vx += nx * jn * mq; q.vy += ny * jn * mq; }
    }
    if (!over && live >= MAXB - 1) { over = true; buzz(200, 150); for (auto& o : b) if (o.live) { float ex = o.x - CX, ey = o.y - CY, e = sqrtf(ex * ex + ey * ey) + 1; o.vx = ex / e * 300; o.vy = ey / e * 300; } }
    if (over && (endT += c.dt) > 2.5f) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    if (!over) cv.drawCircle(CX, CY, RAD, rgb(120, 200, 255));
    for (auto& o : b) if (o.live) {
      float r = RADIUS[o.lv]; uint32_t cc = col(o.lv);
      if (o.lv == LV - 1) { cv.fillCircle((int)o.x, (int)o.y, (int)r, cc); continue; }
      int n = o.lv + 3, px[6], py[6];
      for (int k = 0; k < n; k++) { float a = o.rot + k * 6.2831853f / n; px[k] = (int)(o.x + r * cosf(a)); py[k] = (int)(o.y + r * sinf(a)); }
      for (int k = 1; k + 1 < n; k++) cv.fillTriangle(px[0], py[0], px[k], py[k], px[k + 1], py[k + 1], cc);
    }
    drawSparks();
    char s[24]; snprintf(s, sizeof s, "merges %d  circles %d", merges, circles);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
