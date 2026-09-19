#pragma once
#include "ringlib.h"

// ================= 12. 塗滿圓 =================
// 一顆粗球在圓形場地裡彈跳,走過的地方留下顏色;每撞一次牆就換色,直到整個圓被塗滿。
// 畫布不清除,軌跡直接留在畫布上。傾斜給重力,點螢幕把球朝手指踢
namespace paint {
  constexpr int CX = 160, CY = 120, RAD = 106, BALL_R = 13; constexpr float G = 300, KICK = 260, VMIN = 160;
  static float x, y, vx, vy, hue, endT, fill; static int bounces, frame; static bool over;
  void init() { x = CX; y = CY; float a = frand() * 6.283f; vx = cosf(a) * 200; vy = sinf(a) * 200; hue = frand(); bounces = frame = 0; fill = 0; over = false; endT = 0; cv.fillScreen(0); }
  static float measure() {   // 圓內非黑像素比例
    uint8_t* p = (uint8_t*)cv.getBuffer(); int on = 0, tot = 0;
    for (int yy = CY - RAD; yy < CY + RAD; yy += 2) for (int xx = CX - RAD; xx < CX + RAD; xx += 2) {
      int dx = xx - CX, dy = yy - CY; if (dx * dx + dy * dy > (RAD - 2) * (RAD - 2)) continue;
      tot++; on += p[yy * W + xx] != 0;
    }
    return tot ? on / (float)tot : 0;
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); return; }
    if (c.tap) tapKick(c, x, y, vx, vy, KICK);
    vx += c.gx * G * c.dt; vy += c.gy * G * c.dt;
    float v = sqrtf(vx * vx + vy * vy); if (v < VMIN && v > 1e-3f) { vx *= VMIN / v; vy *= VMIN / v; }
    x += vx * c.dt; y += vy * c.dt;
    if (wallCircle(x, y, vx, vy, BALL_R, CX, CY, RAD) > 0) { bounces++; hue += 0.13f + frand() * 0.2f; snd::note(250 + fill * 500); }
    if (++frame % 10 == 0) { fill = measure(); if (fill > 0.985f) { over = true; buzz(200, 200); snd::note(800); } }
  }
  void draw() {
    if (over) { cv.fillCircle(CX, CY, RAD, hsv(hue)); }
    else cv.fillCircle((int)x, (int)y, BALL_R, hsv(hue));
    cv.drawCircle(CX, CY, RAD, rgb(255, 255, 255));
    cv.fillRect(0, 0, 150, 12, 0);   // 軌跡不清畫布,文字底要自己清
    char s[28]; snprintf(s, sizeof s, "filled %d%%  %d bounces", (int)(fill * 100), bounces);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
