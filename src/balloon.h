#pragma once
#include "ringlib.h"

// ================= 10. 氣球與尖刺 =================
// 圓形場地邊上有 4 根尖刺,氣球每彈一次就充大一點,只有碰到尖刺才會破;破一個生兩個。
// 傾斜給重力,點螢幕在手指處生一顆氣球。生到上限或塞滿就全部飛出重來
namespace balloon {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, RAD = 106, MAXB = 44, NSPIKE = 4, SPIKE_L = 14; constexpr float G = 260, REST = 0.9f, R0 = 4, RMAX = 26, INFLATE = 1.07f;
  struct B { float x, y, vx, vy, r; uint32_t col; bool live; } static b[MAXB];
  static int pops; static bool over; static float endT;
  static void spawn(float x, float y) {
    for (auto& o : b) if (!o.live) { float a = frand() * 6.283f; o = { x, y, cosf(a) * 120, sinf(a) * 120, R0, hsv(frand()), true }; return; }
  }
  static void spikeTip(int k, float& tx, float& ty) { float a = k * 1.5708f + 0.4f; tx = CX + (RAD - SPIKE_L) * cosf(a); ty = CY + (RAD - SPIKE_L) * sinf(a); }
  void init() { memset(b, 0, sizeof b); pops = 0; over = false; endT = 0; ringlib::reset(); spawn(CX, CY); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (!over && c.tap) { float dx = c.tx - CX, dy = c.ty - CY; if (dx * dx + dy * dy < (RAD - 20) * (RAD - 20)) spawn(c.tx, c.ty); }
    int live = 0;
    for (auto& o : b) if (o.live) {
      live++;
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt;
      float v = sqrtf(o.vx * o.vx + o.vy * o.vy); if (!over && v < 70 && v > 1e-3f) { o.vx *= 70 / v; o.vy *= 70 / v; }
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      if (over) { float dx = o.x - CX, dy = o.y - CY; if (dx * dx + dy * dy > (RAD + 40) * (RAD + 40)) o.live = false; continue; }
      if (wallCircle(o.x, o.y, o.vx, o.vy, o.r, CX, CY, RAD, REST) > 0) { if (o.r < RMAX) o.r *= INFLATE; if (live < 10) snd::click(); }
      for (int k = 0; k < NSPIKE; k++) {   // 碰到尖刺尖端就破
        float tx, ty; spikeTip(k, tx, ty); float dx = o.x - tx, dy = o.y - ty;
        if (dx * dx + dy * dy < o.r * o.r) {
          o.live = false; pops++; spark(o.x, o.y, o.col); snd::note(300 + o.r * 12); buzz(40 + (int)o.r * 3, 20);
          for (int n = 0; n < 2; n++) spawn(CX + (frand() - 0.5f) * 60, CY + (frand() - 0.5f) * 60);
          break;
        }
      }
    }
    if (!over) for (int i = 0; i < MAXB; i++) if (b[i].live) for (int j = i + 1; j < MAXB; j++) if (b[j].live) {   // ponytail: O(n^2),n<=44
      auto& p = b[i]; auto& q = b[j];
      float dx = q.x - p.x, dy = q.y - p.y, d = sqrtf(dx * dx + dy * dy), rr = p.r + q.r;
      if (d >= rr || d < 1e-3f) continue;
      float nx = dx / d, ny = dy / d, pen = rr - d;
      p.x -= nx * pen / 2; p.y -= ny * pen / 2; q.x += nx * pen / 2; q.y += ny * pen / 2;
      float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny;
      if (rv < 0) { float jn = -(1 + REST) * rv / 2; p.vx -= nx * jn; p.vy -= ny * jn; q.vx += nx * jn; q.vy += ny * jn; if (p.r < RMAX) p.r *= INFLATE; if (q.r < RMAX) q.r *= INFLATE; }
    }
    if (!over && (live >= MAXB - 2 || pops >= 60)) { over = true; buzz(200, 150); for (auto& o : b) if (o.live) { float ex = o.x - CX, ey = o.y - CY, e = sqrtf(ex * ex + ey * ey) + 1; o.vx = ex / e * 300; o.vy = ey / e * 300; } }
    if ((over && (endT += c.dt) > 2.5f) || !live) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    uint32_t wc = rgb(255, 200, 40);
    if (!over) {
      cv.drawCircle(CX, CY, RAD, wc);
      for (int k = 0; k < NSPIKE; k++) {   // 尖刺:底在環上、尖端朝內
        float a = k * 1.5708f + 0.4f, tx, ty; spikeTip(k, tx, ty);
        cv.fillTriangle((int)tx, (int)ty, (int)(CX + RAD * cosf(a - 0.06f)), (int)(CY + RAD * sinf(a - 0.06f)), (int)(CX + RAD * cosf(a + 0.06f)), (int)(CY + RAD * sinf(a + 0.06f)), rgb(255, 255, 255));
      }
    }
    for (auto& o : b) if (o.live) { cv.fillCircle((int)o.x, (int)o.y, (int)o.r, o.col); if (o.r > 6) cv.drawPixel((int)(o.x - o.r * 0.4f), (int)(o.y - o.r * 0.4f), rgb(255, 255, 255)); }
    drawSparks();
    char s[20]; snprintf(s, sizeof s, "popped %d", pops);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
