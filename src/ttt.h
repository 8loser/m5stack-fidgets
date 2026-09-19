#pragma once
#include "ringlib.h"

// ================= 4. 井字 =================
// 球在慢速旋轉的 3x3 箱裡彈,牆和已蓋的 O / X 都會反彈(撞擊噴火花 + 喀聲)。
// 每回合 2 秒到,就在離球最近的空格蓋上球的顏色(X 紅 / O 青),然後換色。A/C 加速轉箱子
namespace ttt {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CXT = 160, CYT = 120; constexpr float S = 150, HALF = S / 2, CELL = S / 3, G = 250, KICK = 220, OM = 0.35f, TURN_T = 2.0f, MARK_R = 15, BALL_R = 5;
  static float th, endT, turnT, stampT; static int8_t cell[9], turn, win[3], stampCell;
  static struct { float x, y, vx, vy; } b;   // 箱內座標,原點在箱中心
  static int trail[8][2], trailN;
  static void toScreen(float lx, float ly, int& sx, int& sy) { float c = cosf(th), s = sinf(th); sx = (int)(CXT + lx * c - ly * s); sy = (int)(CYT + lx * s + ly * c); }
  static void toLocal(float dx, float dy, float& lx, float& ly) { float c = cosf(th), s = sinf(th); lx = dx * c + dy * s; ly = -dx * s + dy * c; }
  static int8_t winner() {
    static const int8_t L[8][3] = { {0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6} };
    for (auto& l : L) if (cell[l[0]] && cell[l[0]] == cell[l[1]] && cell[l[0]] == cell[l[2]]) { memcpy(win, l, 3); return cell[l[0]]; }
    return 0;
  }
  static uint32_t markCol(int m) { return m == 1 ? rgb(0, 230, 200) : rgb(255, 60, 90); }
  static float cellX(int i) { return (i % 3 - 1) * CELL; } static float cellY(int i) { return (i / 3 - 1) * CELL; }
  void init() {
    th = endT = turnT = stampT = 0; memset(cell, 0, 9); turn = 2; win[0] = -1; stampCell = -1; trailN = 0;
    float a = frand() * 6.283f; b = { 0, 0, cosf(a) * 170, sinf(a) * 170 }; ringlib::reset(); cv.fillScreen(0);
  }
  void step(const Ctx& c) {
    th += (OM + (c.btnC - c.btnA) * 1.5f) * c.dt;   // A/C 轉箱子
    float gx, gy; toLocal(c.gx, c.gy, gx, gy);
    int sx, sy; toScreen(b.x, b.y, sx, sy); int ti = trailN++ % 8; trail[ti][0] = sx; trail[ti][1] = sy;
    if (c.tap) { float lx, ly; toLocal(c.tx - CXT, c.ty - CYT, lx, ly); float dx = lx - b.x, dy = ly - b.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f; b.vx = dx / d * KICK; b.vy = dy / d * KICK; }
    b.vx += gx * G * c.dt; b.vy += gy * G * c.dt;
    float v = sqrtf(b.vx * b.vx + b.vy * b.vy); if (v < 120 && v > 1e-3f) { b.vx *= 120 / v; b.vy *= 120 / v; }
    b.x += b.vx * c.dt; b.y += b.vy * c.dt;
    float hit = 0, lim = HALF - BALL_R;   // 撞擊的法向速度;球被重力壓在牆上時每幀只有幾個單位,不算撞
    if (b.x < -lim) { b.x = -lim; hit = -b.vx; b.vx = fabsf(b.vx); } if (b.x > lim) { b.x = lim; hit = b.vx; b.vx = -fabsf(b.vx); }
    if (b.y < -lim) { b.y = -lim; hit = -b.vy; b.vy = fabsf(b.vy); } if (b.y > lim) { b.y = lim; hit = b.vy; b.vy = -fabsf(b.vy); }
    for (int i = 0; i < 9; i++) if (cell[i]) {   // 記號當圓形障礙
      float dx = b.x - cellX(i), dy = b.y - cellY(i), d = sqrtf(dx * dx + dy * dy) + 1e-3f, R = MARK_R + BALL_R;
      if (d < R) { float nx = dx / d, ny = dy / d, vn = b.vx * nx + b.vy * ny; b.x = cellX(i) + nx * R; b.y = cellY(i) + ny * R; if (vn < 0) { b.vx -= 2 * vn * nx; b.vy -= 2 * vn * ny; hit = -vn; } }
    }
    if (hit > 60) { spark(sx, sy, markCol(turn)); snd::click(); }
    int8_t w = winner(); bool full = true; for (auto m : cell) full &= m != 0;
    if (!w && !full && (turnT += c.dt) >= TURN_T) {   // 回合到:蓋在離球最近的空格
      turnT = 0; int best = -1; float bd = 1e9f;
      for (int i = 0; i < 9; i++) if (!cell[i]) { float dx = b.x - cellX(i), dy = b.y - cellY(i), d = dx * dx + dy * dy; if (d < bd) { bd = d; best = i; } }
      cell[best] = turn; stampCell = best; stampT = 0.5f; turn = 3 - turn;
      int mx, my; toScreen(cellX(best), cellY(best), mx, my); spark(mx, my, markCol(cell[best])); snd::note(400 + best * 50); buzz(60, 20);
      if (winner()) buzz(180, 120);
    }
    if ((w || full) && (endT += c.dt) > 2.5f) init();
    stampT -= c.dt; stepSparks(c.dt);
  }
  static void line(float x0, float y0, float x1, float y1, uint32_t col) {
    int a, b2, c2, d; toScreen(x0, y0, a, b2); toScreen(x1, y1, c2, d);
    cv.drawLine(a, b2, c2, d, col); cv.drawLine(a + 1, b2, c2 + 1, d, col); cv.drawLine(a, b2 + 1, c2, d + 1, col);
  }
  void draw() {
    cv.fillScreen(0);
    uint32_t wh = rgb(255, 255, 255);
    for (int i = -1; i <= 1; i += 2) { line(-HALF, i * HALF, HALF, i * HALF, wh); line(i * HALF, -HALF, i * HALF, HALF, wh); }
    for (int i = -1; i <= 1; i += 2) { line(-HALF, i * CELL / 2, HALF, i * CELL / 2, wh); line(i * CELL / 2, -HALF, i * CELL / 2, HALF, wh); }
    for (int i = 0; i < 9; i++) if (cell[i]) {
      float cx = cellX(i), cy = cellY(i); uint32_t col = markCol(cell[i]); int sx, sy; toScreen(cx, cy, sx, sy);
      if (cell[i] == 1) { cv.drawCircle(sx, sy, 15, col); cv.drawCircle(sx, sy, 14, col); cv.drawCircle(sx, sy, 13, col); }
      else { line(cx - 12, cy - 12, cx + 12, cy + 12, col); line(cx - 12, cy + 12, cx + 12, cy - 12, col); }
      if (i == stampCell && stampT > 0) cv.drawCircle(sx, sy, (int)(18 + (0.5f - stampT) * 40), col);   // 蓋章擴散圈
    }
    if (win[0] >= 0 && winner()) line(cellX(win[0]), cellY(win[0]), cellX(win[2]), cellY(win[2]), rgb(255, 230, 0));
    uint32_t bc = markCol(turn);
    for (int k = 1; k < 8 && k < trailN; k++) { int* t = trail[(trailN - 1 - k + 8) % 8]; cv.fillCircle(t[0], t[1], 5 - k * 5 / 8, bc); }
    int sx, sy; toScreen(b.x, b.y, sx, sy); cv.fillCircle(sx, sy, (int)BALL_R, bc);
    drawSparks();
  }
}
