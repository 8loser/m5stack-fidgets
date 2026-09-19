#pragma once
#include "common.h"

// ================= 17. 三色領土戰 =================
// 棋盤分成三色領土,各色一顆球;球撞到別色的格子就把它染成自己的顏色並反彈。
// 傾斜給球一點偏向(速度長度固定),點螢幕把最近的球朝手指踢。一色獨占全盤就重來
namespace war {
  constexpr int GW = 30, GH = 22, CS = 10, X0 = 10, Y0 = 10, NT = 3, BALL_R = 4; constexpr float SPEED = 170, G = 120, KICK = 200;
  static uint8_t grid[GH][GW]; static int cnt[NT]; static float endT; static bool over;
  static struct { float x, y, vx, vy; } b[NT];
  static uint32_t col(int t) { return t == 0 ? rgb(255, 160, 0) : t == 1 ? rgb(0, 210, 255) : rgb(255, 50, 130); }
  void init() {
    for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) grid[y][x] = x * NT / GW;
    for (int t = 0; t < NT; t++) { float a = frand() * 6.283f; b[t] = { X0 + (t + 0.5f) * GW * CS / NT, Y0 + GH * CS / 2.0f, cosf(a) * SPEED, sinf(a) * SPEED }; cnt[t] = GW * GH / NT; }
    over = false; endT = 0; cv.fillScreen(0);
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); return; }
    if (c.tap) { int best = -1; float bd = 1e9f; for (int t = 0; t < NT; t++) if (cnt[t]) { float dx = b[t].x - c.tx, dy = b[t].y - c.ty, d = dx * dx + dy * dy; if (d < bd) { bd = d; best = t; } }
                 if (best >= 0) tapKick(c, b[best].x, b[best].y, b[best].vx, b[best].vy, KICK); }
    for (int t = 0; t < NT; t++) if (cnt[t]) {
      auto& o = b[t];
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt; setSpeed(o.vx, o.vy, SPEED);
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      float lo = X0 + BALL_R, hi = X0 + GW * CS - BALL_R; if (o.x < lo) { o.x = lo; o.vx = fabsf(o.vx); } if (o.x > hi) { o.x = hi; o.vx = -fabsf(o.vx); }
      lo = Y0 + BALL_R; hi = Y0 + GH * CS - BALL_R;         if (o.y < lo) { o.y = lo; o.vy = fabsf(o.vy); } if (o.y > hi) { o.y = hi; o.vy = -fabsf(o.vy); }
      static const int8_t DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };   // 球前後左右四個取樣點碰到別色格就染色 + 沿該軸反彈
      for (int k = 0; k < 4; k++) {
        int gx = (int)((o.x + DX[k] * BALL_R - X0) / CS), gy = (int)((o.y + DY[k] * BALL_R - Y0) / CS);
        if (gx < 0 || gx >= GW || gy < 0 || gy >= GH || grid[gy][gx] == t) continue;
        cnt[grid[gy][gx]]--; grid[gy][gx] = t; cnt[t]++;
        if (DX[k]) o.vx = DX[k] > 0 ? -fabsf(o.vx) : fabsf(o.vx); else o.vy = DY[k] > 0 ? -fabsf(o.vy) : fabsf(o.vy);
        snd::note(220 + t * 110); break;
      }
    }
    for (int t = 0; t < NT; t++) if (cnt[t] == GW * GH) { over = true; buzz(200, 150); }
  }
  void draw() {
    cv.fillScreen(0);
    for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) cv.fillRect(X0 + x * CS, Y0 + y * CS, CS - 1, CS - 1, col(grid[y][x]));
    for (int t = 0; t < NT; t++) if (cnt[t]) { cv.fillCircle((int)b[t].x, (int)b[t].y, BALL_R, rgb(255, 255, 255)); cv.drawCircle((int)b[t].x, (int)b[t].y, BALL_R + 2, col(t)); }
    int xx = 0; for (int t = 0; t < NT; t++) { int w = W * cnt[t] / (GW * GH); cv.fillRect(xx, H - 5, w, 5, col(t)); xx += w; }   // 底部佔比條
  }
}
