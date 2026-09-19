#pragma once
#include "ringlib.h"

// ================= 23. 同色合併競賽 =================
// 頂端每 0.7 秒丟 3 顆隨機顏色的小球進六色環;同色球碰到就合併成更大的一顆(面積相加),
// 最先長到門檻的顏色贏。傾斜給重力,點螢幕在手指處丟 3 顆
namespace colormerge {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks; using ringlib::segCol;
  constexpr int CX = 160, CY = 120, RAD = 106, MAXB = 60, NC = 6; constexpr float G = 320, REST = 0.35f, R0 = 4, WIN_R = 34, DROP_T = 0.7f;
  struct B { float x, y, vx, vy, r; int8_t col; bool live; } static b[MAXB];
  static float rot, dropT, endT; static int winner; static bool over;
  static void drop(float x, float y) { for (int k = 0; k < 3; k++) for (auto& o : b) if (!o.live) { o = { x + (k - 1) * 9.0f, y, (frand() - 0.5f) * 30, 0, R0, (int8_t)((int)(frand() * NC) % NC), true }; break; } }
  void init() { memset(b, 0, sizeof b); rot = dropT = endT = 0; winner = -1; over = false; ringlib::reset(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    rot += (20 + (c.btnC - c.btnA) * 120) * c.dt;   // A/C 轉環(裝飾)
    if (over) { if ((endT += c.dt) > 2.5f) init(); stepSparks(c.dt); return; }
    if ((dropT += c.dt) > DROP_T) { dropT = 0; drop(CX + (frand() - 0.5f) * 40, CY - RAD + 14); }
    if (c.tap) { float dx = c.tx - CX, dy = c.ty - CY; if (dx * dx + dy * dy < (RAD - 12) * (RAD - 12)) drop(c.tx, c.ty); }
    for (auto& o : b) if (o.live) {
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt; o.vx *= 0.995f; o.vy *= 0.995f;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      wallCircle(o.x, o.y, o.vx, o.vy, o.r, CX, CY, RAD, REST);
    }
    for (int i = 0; i < MAXB; i++) if (b[i].live) for (int j = i + 1; j < MAXB; j++) if (b[j].live) {   // ponytail: O(n^2),n<=60
      auto& p = b[i]; auto& q = b[j];
      float dx = q.x - p.x, dy = q.y - p.y, d = sqrtf(dx * dx + dy * dy), rr = p.r + q.r;
      if (d >= rr || d < 1e-3f) continue;
      if (p.col == q.col) {   // 同色合併:面積相加,位置依面積加權
        float a1 = p.r * p.r, a2 = q.r * q.r, w = a2 / (a1 + a2);
        p.x += dx * w; p.y += dy * w; p.vx = p.vx * (1 - w) + q.vx * w; p.vy = p.vy * (1 - w) + q.vy * w; p.r = sqrtf(a1 + a2); q.live = false;
        snd::note(200 + p.r * 12); if (p.r > 12) buzz(30 + (int)p.r, 15);
        if (p.r >= WIN_R) { over = true; winner = p.col; buzz(200, 200); for (int k = 0; k < 6; k++) spark(p.x, p.y, segCol(p.col)); }
        continue;
      }
      float nx = dx / d, ny = dy / d, pen = rr - d, mp = q.r * q.r / (p.r * p.r + q.r * q.r), mq = 1 - mp;   // 大的少動
      p.x -= nx * pen * mp; p.y -= ny * pen * mp; q.x += nx * pen * mq; q.y += ny * pen * mq;
      float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny;
      if (rv < 0) { float jn = -(1 + REST) * rv; p.vx -= nx * jn * mp; p.vy -= ny * jn * mp; q.vx += nx * jn * mq; q.vy += ny * jn * mq; }
    }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    for (int s = 0; s < NC; s++) ringlib::arc(CX, CY, RAD + 3, rot + s * 60 + 2, rot + s * 60 + 58, segCol(s));
    cv.fillCircle(CX, CY - RAD - 6, 3, rgb(255, 255, 255));   // 發射器
    for (auto& o : b) if (o.live) cv.fillCircle((int)o.x, (int)o.y, (int)o.r, segCol(o.col));
    drawSparks();
    if (over) { cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(segCol(winner), 0); cv.drawString("WINS", CX, CY); }
    int big = -1; float br = 0; for (auto& o : b) if (o.live && o.r > br) { br = o.r; big = o.col; }
    if (big >= 0) { cv.fillRect(4, 5, (int)(br / WIN_R * 80), 6, segCol(big)); cv.drawRect(4, 5, 80, 6, rgb(80, 80, 90)); }   // 最大球離門檻多遠
  }
}
