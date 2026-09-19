#pragma once
#include "ringlib.h"

// ================= 24. 黑筆塗圓 =================
// 白色圓盤裡一顆球彈,每撞一次牆就從上一個撞點畫一條粗黑弦、球變大一級、換一個顏色;
// 球大到塞滿圓盤就結束。傾斜給重力,點螢幕把球朝手指踢。
// 墨跡留在畫布上不重畫;球每幀先還原上一幀蓋掉的區域再畫,所以要一塊備份緩衝
namespace inkring {
  constexpr int CX = 160, CY = 120, RAD = 106, RMAX = 60, BOX = RMAX * 2 + 6; constexpr float G = 300, KICK = 260, VMIN = 170, R0 = 6, GROW = 1.4f;   // BOX = 備份框邊長 (r + 2) * 2 + 1
  static const uint8_t PAL[7][3] = { {40,200,80},{160,90,240},{230,150,20},{240,80,180},{40,160,240},{240,60,60},{250,210,40} };
  static float x, y, vx, vy, r, lx, ly, px, py, endT; static int bounces, pi; static bool over, hasLast, pend;   // pend:這次撞牆的弦還沒畫(要等 draw 還原畫布後才能畫)
  static uint8_t back[BOX * BOX]; static int bx0, by0, bw, bh;   // 球下面那塊畫布的備份
  static uint32_t col() { return rgb(PAL[pi][0], PAL[pi][1], PAL[pi][2]); }
  static void white() { cv.fillScreen(0); cv.fillCircle(CX, CY, RAD, rgb(255, 255, 255)); cv.drawCircle(CX, CY, RAD + 2, rgb(200, 200, 200)); }
  void init() { x = CX; y = CY; float a = frand() * 6.283f; vx = cosf(a) * 220; vy = sinf(a) * 220; r = R0; bounces = 0; pi = 0; over = false; hasLast = pend = false; endT = 0; bw = bh = 0; white(); }
  static void chord(float ax, float ay, float bx, float by, float w) {   // 粗線:一個四邊形
    float dx = bx - ax, dy = by - ay, d = sqrtf(dx * dx + dy * dy) + 1e-3f, nx = -dy / d * w / 2, ny = dx / d * w / 2;
    cv.fillTriangle((int)(ax + nx), (int)(ay + ny), (int)(bx + nx), (int)(by + ny), (int)(bx - nx), (int)(by - ny), 0);
    cv.fillTriangle((int)(ax + nx), (int)(ay + ny), (int)(bx - nx), (int)(by - ny), (int)(ax - nx), (int)(ay - ny), 0);
    cv.fillCircle((int)ax, (int)ay, (int)(w / 2), 0); cv.fillCircle((int)bx, (int)by, (int)(w / 2), 0);
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); return; }
    if (c.tap) tapKick(c, x, y, vx, vy, KICK);
    vx += c.gx * G * c.dt; vy += c.gy * G * c.dt;
    float v = sqrtf(vx * vx + vy * vy); if (v < VMIN && v > 1e-3f) { vx *= VMIN / v; vy *= VMIN / v; }
    x += vx * c.dt; y += vy * c.dt;
    if (wallCircle(x, y, vx, vy, r, CX, CY, RAD) > 0) {
      pend = hasLast; px = lx; py = ly; lx = x; ly = y; hasLast = true; bounces++; pi = (pi + 1) % 7; snd::click(); buzz(20, 8);
      if (r < RMAX) r += GROW; else { over = true; snd::note(700); buzz(200, 200); }
    }
  }
  void draw() {
    uint8_t* p = (uint8_t*)cv.getBuffer();
    for (int j = 0; j < bh; j++) memcpy(p + (by0 + j) * W + bx0, back + j * BOX, bw);   // 還原上一幀球蓋掉的區域
    if (pend) { chord(px, py, lx, ly, r * 0.6f); pend = false; }
    if (over) { cv.fillCircle(CX, CY, RAD, col()); bw = bh = 0; }
    else {
      int ri = (int)r + 2; bx0 = (int)x - ri; by0 = (int)y - ri; bw = bh = ri * 2 + 1;
      if (bx0 < 0) { bw += bx0; bx0 = 0; } if (by0 < 0) { bh += by0; by0 = 0; } if (bx0 + bw > W) bw = W - bx0; if (by0 + bh > H) bh = H - by0;
      for (int j = 0; j < bh; j++) memcpy(back + j * BOX, p + (by0 + j) * W + bx0, bw);
      cv.fillCircle((int)x, (int)y, (int)r, col());
    }
    cv.fillRect(0, 0, 110, 12, 0);
    char t[24]; snprintf(t, sizeof t, "bounces %d  r %d", bounces, (int)r);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4);
  }
}
