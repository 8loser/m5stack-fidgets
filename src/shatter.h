#pragma once
#include "ringlib.h"

// ================= 18. 打碎環 =================
// 中心的球往外撞,同心的磚環一塊一塊被打碎;整環清空球就加速。
// 傾斜改變球的方向(速度長度固定),點螢幕把球朝手指踢。全部打碎就重來
namespace shatter {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, NR = 5, NB = 20, R0 = 46, DR = 12, BALL_R = 4, RAD = 110; constexpr float G = 250, KICK = 220, V0 = 130;
  static bool brick[NR][NB]; static int left[NR]; static float speed, endT; static bool over;
  static struct { float x, y, vx, vy; } b;
  static uint32_t col(int r) { return hsv(r / (float)NR); }
  void init() {
    for (int r = 0; r < NR; r++) { left[r] = NB; for (int k = 0; k < NB; k++) brick[r][k] = true; }
    speed = V0; over = false; endT = 0; float a = frand() * 6.283f; b = { CX, CY, cosf(a) * speed, sinf(a) * speed }; ringlib::reset(); cv.fillScreen(0);
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); stepSparks(c.dt); return; }
    if (c.tap) tapKick(c, b.x, b.y, b.vx, b.vy, KICK);
    b.vx += c.gx * G * c.dt; b.vy += c.gy * G * c.dt; setSpeed(b.vx, b.vy, speed);
    b.x += b.vx * c.dt; b.y += b.vy * c.dt;
    float dx = b.x - CX, dy = b.y - CY, d = sqrtf(dx * dx + dy * dy) + 1e-3f, nx = dx / d, ny = dy / d, vn = b.vx * nx + b.vy * ny;
    int k = (int)(fmodf(atan2f(dy, dx) * RAD_TO_DEG + 360, 360) / (360.0f / NB)) % NB;
    for (int r = 0; r < NR; r++) {   // 環 r 的磚在半徑 R0 + r*DR;球徑向穿過且該磚還在就打碎並反彈
      float rr = R0 + r * DR; if (fabsf(d - rr) > BALL_R + 2 || !brick[r][k]) continue;
      if ((vn > 0) != (d < rr)) continue;   // 只在朝磚移動時算撞
      brick[r][k] = false; b.vx -= 2 * vn * nx; b.vy -= 2 * vn * ny; b.x = CX + nx * (rr + (vn > 0 ? -1 : 1) * (BALL_R + 3)); b.y = CY + ny * (rr + (vn > 0 ? -1 : 1) * (BALL_R + 3));
      spark(b.x, b.y, col(r)); snd::click(); buzz(30, 10);
      if (--left[r] == 0) { speed *= 1.2f; snd::note(300 + r * 80); buzz(120, 60); }
      break;
    }
    if (wallCircle(b.x, b.y, b.vx, b.vy, BALL_R, CX, CY, RAD) > 0) snd::click();
    int total = 0; for (int r = 0; r < NR; r++) total += left[r];
    if (!total) { over = true; buzz(200, 150); for (int n = 0; n < 6; n++) spark(CX, CY, hsv(frand())); }
    stepSparks(c.dt);
  }
  void draw() {
    fadeCanvas();
    for (int r = 0; r < NR; r++) for (int k = 0; k < NB; k++) if (brick[r][k]) {
      float a0 = k * 360.0f / NB + 2, a1 = a0 + 360.0f / NB - 4; ringlib::arc(CX, CY, R0 + r * DR, a0, a1, col(r));
    }
    cv.drawCircle(CX, CY, RAD, rgb(80, 80, 90));
    if (!over) cv.fillCircle((int)b.x, (int)b.y, BALL_R, rgb(255, 255, 255));
    drawSparks();
    cv.fillRect(0, 0, 120, 12, 0);
    char s[20]; snprintf(s, sizeof s, "speed %d", (int)speed);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
