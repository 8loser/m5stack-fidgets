#pragma once
#include "ringlib.h"
#include "face.h"

// ================= 22. 壓板與 Seed =================
// 箱子裡一堆球,天花板的壓板每隔幾秒壓下來,壓到的每顆球變成 xN 顆從兩側噴出;球數到上限就改成壓板變寬。
// 你是有臉的 Seed:傾斜是箱子的重力(所有球都滑),但 Seed 對傾斜反應更快;點螢幕跳。被壓到就結束
namespace seed {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int MAXB = 160, BALL_R = 4, SR = 6, X0 = 0, X1 = W, FLOOR = H, TOP = 28, PLATE_H = 10; constexpr float G = 700, REST = 0.3f, PW0 = 110, PWMAX = 270;
  struct B { float x, y, vx, vy; uint32_t col; bool live; } static b[MAXB];
  static struct { float x, y, vx, vy; bool ground; int squeeze; } s;
  static float py, pw, phaseT, endT; static int phase, nball, crushes, mult, best; static bool dead; static face::Mood mood;   // phase 0 等待 1 下壓 2 壓底 3 上升
  static void spawn(float x, float y, float vx, float vy, uint32_t col) { for (auto& o : b) if (!o.live) { o = { x, y, vx, vy, col, true }; nball++; return; } }
  void init() {
    memset(b, 0, sizeof b); nball = crushes = 0; mult = 2; dead = false; endT = 0; py = TOP; pw = PW0; phase = 0; phaseT = 0; mood = face::CALM;
    for (int i = 0; i < 6; i++) spawn(80 + i * 32, FLOOR - BALL_R, 0, 0, hsv(0.5f));
    s = { X0 + 30, FLOOR - SR, 0, 0, false, 0 };
    ringlib::reset(); cv.fillScreen(0);
  }
  static float plateL() { return 160 - pw / 2; } static float plateR() { return 160 + pw / 2; }
  // 圓與箱子、壓板的碰撞;回傳是否站在東西上
  static bool wallHit(float& x, float& y, float& vx, float& vy, float r) {
    bool ground = false;
    if (x < X0 + r) { x = X0 + r; vx = fabsf(vx) * REST; } if (x > X1 - r) { x = X1 - r; vx = -fabsf(vx) * REST; }
    if (y > FLOOR - r) { y = FLOOR - r; if (vy > 0) vy = -vy * REST; vx *= 0.97f; ground = true; }
    if (y < TOP + r) { y = TOP + r; vy = fabsf(vy); }
    if (x > plateL() - r && x < plateR() + r && y < py + r && y > py - PLATE_H - r) {   // 壓板:從最近的一側推出
      float dl = x - (plateL() - r), dr = (plateR() + r) - x, db = (py + r) - y;
      if (db < dl && db < dr) { y = py + r; if (vy < 0) vy = 0; if (phase == 1) vy = fmaxf(vy, 260); } else if (dl < dr) { x = plateL() - r; vx = -fabsf(vx); } else { x = plateR() + r; vx = fabsf(vx); }
    }
    return ground;
  }
  static void crush() {   // 壓底:壓板下的球全部增殖(到上限就改變寬),Seed 在下面就結束
    static float cx[MAXB], cy[MAXB]; int under = 0;   // 先收集再生,新生的球不能在同一輪再被壓
    for (auto& o : b) if (o.live && o.x > plateL() - BALL_R && o.x < plateR() + BALL_R) { o.live = false; nball--; cx[under] = o.x; cy[under] = o.y; under++; }
    uint32_t col = hsv(crushes * 0.07f + frand() * 0.1f);
    for (int i = 0; i < under; i++) { float side = cx[i] < 160 ? -1 : 1; for (int k = 0; k < mult; k++) spawn(cx[i], cy[i], side * (120 + frand() * 200), -150 - frand() * 250, col); }
    if (under) { crushes++; spark(160, FLOOR - 6, rgb(255, 230, 0)); snd::note(180 + crushes * 20); buzz(120, 60); }
    if (nball >= MAXB - mult * 2 && pw < PWMAX) pw += 25;   // 球滿了:壓板變寬
    if (crushes % 4 == 0 && under && mult < 6) mult++;
    if (s.x > plateL() - SR && s.x < plateR() + SR) { dead = true; endT = 0; mood = face::DEAD; if (nball > best) best = nball; spark(s.x, s.y, rgb(255, 80, 80)); snd::note(120); buzz(255, 400); }
  }
  void step(const Ctx& c) {
    if (dead) { if ((endT += c.dt) > 1 && c.tap) init(); return; }
    phaseT += c.dt;   // 壓板週期
    switch (phase) {
      case 0: if (phaseT > 1.6f) { phase = 1; phaseT = 0; } break;
      case 1: py += 300 * c.dt; if (py >= FLOOR - BALL_R * 2) { py = FLOOR - BALL_R * 2; crush(); phase = 2; phaseT = 0; } break;
      case 2: if (phaseT > 0.35f) { phase = 3; phaseT = 0; } break;
      default: py -= 180 * c.dt; if (py <= TOP) { py = TOP; phase = 0; phaseT = 0; }
    }
    float gx = c.gx * G;
    for (auto& o : b) if (o.live) { o.vx += gx * c.dt; o.vy += G * c.dt; o.x += o.vx * c.dt; o.y += o.vy * c.dt; wallHit(o.x, o.y, o.vx, o.vy, BALL_R); }
    s.vx += (gx + c.gx * 1200) * c.dt; s.vy += G * c.dt;   // Seed 對傾斜反應更快
    if (c.tap && s.ground) { s.vy = -330; snd::note(500); }
    s.x += s.vx * c.dt; s.y += s.vy * c.dt; s.ground = wallHit(s.x, s.y, s.vx, s.vy, SR); s.squeeze = 0;
    auto bump = [&](float& ax, float& ay, float& avx, float& avy, float ar, float& bx, float& by, float& bvx, float& bvy, float br2) {
      float dx = bx - ax, dy = by - ay, d = sqrtf(dx * dx + dy * dy), pen = ar + br2 - d;
      if (pen <= 0 || d < 1e-3f) return false;
      float nx = dx / d, ny = dy / d; ax -= nx * pen / 2; ay -= ny * pen / 2; bx += nx * pen / 2; by += ny * pen / 2;
      float rv = (bvx - avx) * nx + (bvy - avy) * ny;
      if (rv < 0) { float j = -(1 + REST) * rv / 2; avx -= nx * j; avy -= ny * j; bvx += nx * j; bvy += ny * j; }
      return true;
    };
    for (int i = 0; i < MAXB; i++) if (b[i].live) {   // ponytail: O(n^2),n<=160
      for (int j = i + 1; j < MAXB; j++) if (b[j].live) bump(b[i].x, b[i].y, b[i].vx, b[i].vy, BALL_R, b[j].x, b[j].y, b[j].vx, b[j].vy, BALL_R);
      if (bump(s.x, s.y, s.vx, s.vy, SR, b[i].x, b[i].y, b[i].vx, b[i].vy, BALL_R)) { s.squeeze++; if (b[i].y < s.y) s.ground = true; }
    }
    bool underPlate = s.x > plateL() - SR && s.x < plateR() + SR;
    mood = underPlate && phase == 1 ? face::SCARED : s.squeeze >= 4 ? face::HURT : underPlate ? face::SAD : nball > 60 ? face::CALM : face::HAPPY;
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    cv.fillRect(150, 0, 20, (int)py - PLATE_H, rgb(90, 90, 100));   // 壓柱
    cv.fillRect((int)plateL(), (int)py - PLATE_H, (int)pw, PLATE_H, rgb(130, 130, 145));
    for (int x = (int)plateL(); x < plateR(); x += 8) cv.fillRect(x, (int)py - 3, 4, 3, phase == 0 && phaseT > 1.0f && ((int)(phaseT * 10) & 1) ? rgb(255, 60, 60) : rgb(255, 210, 0));   // 警示條
    for (auto& o : b) if (o.live) cv.fillCircle((int)o.x, (int)o.y, BALL_R, o.col);
    cv.fillCircle((int)s.x, (int)s.y, SR, rgb(220, 120, 30)); face::draw(s.x, s.y, 1, 0, 0, 1, mood, 0.8f);
    drawSparks();
    char t[40]; snprintf(t, sizeof t, "x%d  balls %d  best %d", mult, nball, best);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4);
    if (dead) { cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(rgb(255, 80, 80), 0); snprintf(t, sizeof t, "CRUSHED AT %d", nball); cv.drawString(t, 160, 110); cv.setTextSize(1); cv.setTextColor(rgb(200, 200, 200), 0); cv.drawString("tap to retry", 160, 130); }
  }
}
