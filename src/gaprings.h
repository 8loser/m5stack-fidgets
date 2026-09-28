#pragma once
#include "ringlib.h"

// ================= 逃脫加速 =================
// 同心環各開一道缺口、各自以不同方向與速度旋轉;球從中心往外逃,穿過缺口就進到下一層並加速。
// 逃出全部就下一回合、起始速度更快。
// 傾斜改變球的方向(速度長度固定),點螢幕把球朝手指踢,A/C 轉整組環
namespace gaprings {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, NR = 7, RMIN = 22, RSTEP = 13, BALL_R = 4, NROUND = 8; constexpr float GAP = 44, G = 250, KICK = 200, V0 = 110;
  static float ang[NR], spd[NR], speed, endT; static int level, round_, passes; static bool over;
  static struct { float x, y, vx, vy; } b;
  static float radius(int i) { return RMIN + i * RSTEP; }
  static bool inGap(int i, float a) { float d = fmodf(a - ang[i] + 720, 360); return d < GAP / 2 || d > 360 - GAP / 2; }
  static void launch() { float a = frand() * 6.283f; b.vx = cosf(a) * speed; b.vy = sinf(a) * speed; }
  static void start() {
    over = false; endT = 0; passes = 0; speed = V0 * (1 + round_ * 0.15f);
    for (int i = 0; i < NR; i++) { ang[i] = frand() * 360; spd[i] = (18 + i * 7) * (i & 1 ? -1 : 1); }
    level = 0;   // level L:球在環 L-1 與環 L 之間;0 = 中心區,NR = 全部逃出
    b.x = CX; b.y = CY; launch();
    ringlib::reset(); cv.fillScreen(0);
  }
  void init() { round_ = 0; start(); }
  void step(const Ctx& c) {
    for (int i = 0; i < NR; i++) ang[i] += (spd[i] + (c.btnC - c.btnA) * 70) * c.dt;   // A/C 整組環一起轉
    if (over) { if ((endT += c.dt) > 2) { round_ = 0; start(); } stepSparks(c.dt); return; }
    if (c.tap) tapKick(c, b.x, b.y, b.vx, b.vy, KICK);
    b.vx += c.gx * G * c.dt; b.vy += c.gy * G * c.dt; setSpeed(b.vx, b.vy, speed);
    b.x += b.vx * c.dt; b.y += b.vy * c.dt;
    float dx = b.x - CX, dy = b.y - CY, d = sqrtf(dx * dx + dy * dy) + 1e-3f, nx = dx / d, ny = dy / d, vn = b.vx * nx + b.vy * ny, a = atan2f(dy, dx) * RAD_TO_DEG;
    bool hit = false, pass = false;
    if (level > 0 && vn < 0 && d < radius(level - 1) + BALL_R) hit = true;   // 內環:擋住不讓回頭
    if (level < NR && vn > 0 && d > radius(level) - BALL_R) {      // 外環:穿缺口就逃出一層
      if (inGap(level, a)) { level++; pass = true; } else hit = true;
    }
    if (hit) { b.vx -= 2 * vn * nx; b.vy -= 2 * vn * ny; b.x += b.vx * c.dt; b.y += b.vy * c.dt; snd::click(); }
    if (pass) { speed *= 1.12f; passes++; spark(b.x, b.y, hsv(level / (float)NR)); snd::note(300 + level * 60); buzz(60, 20); }
    if (level == NR) {
      if (++round_ >= NROUND) { over = true; buzz(200, 150); } else { start(); }
    }
    stepSparks(c.dt);
  }
  void draw() {
    fadeCanvas();
    for (int i = 0; i < NR; i++) if (over || i >= level) ringlib::arc(CX, CY, radius(i), ang[i] + GAP / 2, ang[i] + 360 - GAP / 2, hsv(i / (float)NR));   // 已逃出的環就消失
    if (!over) cv.fillCircle((int)b.x, (int)b.y, BALL_R, rgb(255, 255, 255));
    drawSparks();
    char t[24]; snprintf(t, sizeof t, "round %d/%d  %d left", round_ + 1, NROUND, NR - level);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4);
  }
}
