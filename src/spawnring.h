#pragma once
#include "ringlib.h"

// ================= 25. 每彈一次生一顆 =================
// 無重力的圓場,環是四段帶缺口的旋轉弧;球每撞一次弧就在撞點多生一顆,穿過缺口的球飛走。
// 傾斜左右改變環的轉速與方向,點螢幕在手指處丟一顆。5 秒內球數沒有創新高就結束,分數是這回合的最高球數,前 5 名存 NVS
namespace spawnring {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, RAD = 106, MAXB = 150, BALL_R = 4, NARC = 4; constexpr float SPEED = 120, ARC = 62, FULL = 140;   // 弧 62 度、缺口 28 度
  struct B { float x, y, vx, vy; uint32_t col; bool live; } static b[MAXB];
  static float rot, endT, stallT; static int nball, peak, rank; static bool over;
  static Board board = { "spawnring" };
  static void spawn(float x, float y) { for (auto& o : b) if (!o.live) { float a = frand() * 6.283f; o = { x, y, cosf(a) * SPEED, sinf(a) * SPEED, hsv(frand()), true }; nball++; if (nball > peak) { peak = nball; stallT = 0; } return; } }
  static bool onArc(float ang) { float a = fmodf(ang - rot + 720, 360); return fmodf(a, 90) < ARC; }   // 每 90 度一段弧
  void init() {
    memset(b, 0, sizeof b); rot = endT = stallT = 0; nball = peak = 0; rank = -1; over = false; spawn(CX, CY); ringlib::reset(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    rot += (30 + c.gx * 80 + (c.btnC - c.btnA) * 80) * c.dt;   // 傾斜或 A/C 控制轉速與方向
    if (over) { if ((endT += c.dt) > 2 && (c.tap || c.tapA || c.tapC)) init(); stepSparks(c.dt); return; }
    if (c.tap) { float dx = c.tx - CX, dy = c.ty - CY; if (dx * dx + dy * dy < (RAD - 10) * (RAD - 10)) spawn(c.tx, c.ty); }
    for (auto& o : b) if (o.live) {
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      float dx = o.x - CX, dy = o.y - CY, d = sqrtf(dx * dx + dy * dy) + 1e-3f;
      if (d > RAD + 30) { o.live = false; nball--; continue; }   // 從缺口飛走
      if (d > RAD - BALL_R && d < RAD + BALL_R) {
        float nx = dx / d, ny = dy / d, vn = o.vx * nx + o.vy * ny;
        if (vn > 0 && onArc(atan2f(dy, dx) * RAD_TO_DEG)) {
          o.x = CX + nx * (RAD - BALL_R); o.y = CY + ny * (RAD - BALL_R); o.vx -= 2 * vn * nx; o.vy -= 2 * vn * ny;
          if (frand() > nball / FULL) spawn(o.x - nx * 6, o.y - ny * 6);   // 越滿越難生,增長從指數變平緩
          spark(o.x, o.y, o.col); if (nball < 30) snd::note(300 + nball * 15); else if ((nball & 3) == 0) snd::click();
        }
      }
    }
    for (int i = 0; i < MAXB; i++) if (b[i].live) for (int j = i + 1; j < MAXB; j++) if (b[j].live) {   // ponytail: O(n^2),n<=150;等質量正碰就是交換法向速度
      auto& p = b[i]; auto& q = b[j];
      float dx = q.x - p.x, dy = q.y - p.y, d2 = dx * dx + dy * dy;
      if (d2 >= 4 * BALL_R * BALL_R || d2 < 1e-3f) continue;
      float d = sqrtf(d2), nx = dx / d, ny = dy / d, pen = (2 * BALL_R - d) / 2;
      p.x -= nx * pen; p.y -= ny * pen; q.x += nx * pen; q.y += ny * pen;
      float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny;
      if (rv < 0) { p.vx += nx * rv; p.vy += ny * rv; q.vx -= nx * rv; q.vy -= ny * rv; }
      setSpeed(p.vx, p.vy, SPEED); setSpeed(q.vx, q.vy, SPEED);
    }
    if ((stallT += c.dt) > 5) { over = true; rank = board.record(peak); buzz(150, 150); snd::note(rank == 0 ? 800 : rank >= 0 ? 600 : 300); }   // 5 秒沒創新高就結束
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    if (!over) for (int k = 0; k < NARC; k++) ringlib::arc(CX, CY, RAD, rot + k * 90, rot + k * 90 + ARC, hsv(k / (float)NARC + 0.1f));
    for (auto& o : b) if (o.live) { cv.fillCircle((int)o.x, (int)o.y, BALL_R, o.col); cv.drawPixel((int)o.x - 1, (int)o.y - 1, rgb(255, 255, 255)); }
    drawSparks();
    if (over) { char s[20]; snprintf(s, sizeof s, "PEAK %d", peak); board.draw(s, rank); }
    char t[32]; snprintf(t, sizeof t, "balls %d  peak %d  best %u", nball, peak, board.best());
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4);
  }
}
