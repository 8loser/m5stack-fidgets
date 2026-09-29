#pragma once
#include "common.h"

// ================= 彩色連線 =================
// 全螢幕場地,一顆球彈來彈去;每撞一次牆就用球現在的顏色從上一個撞點畫一條線到這個撞點,然後球換色。
// 線留在畫布上一直疊,不會結束(B 雙擊清掉重來)。傾斜給重力,點螢幕把球朝手指踢,搖一下噴開,A / C 在「自動(每撞一次換色)」與 8 個固定顏色之間輪流切換。
// 球每幀先還原上一幀蓋掉的區域再畫,線才不會被球擦掉
namespace stringart {
  constexpr int NMODE = 9, R = 8, BOX = (R + 2) * 2 + 1; constexpr float G = 150, KICK = 160, VMIN = 70, VMAX = 160;   // 低於 VMIN 拉回、高於 VMAX 慢慢衰減回來:不會停,搖一下會快一陣
  static float x, y, vx, vy, hue, lx, ly, px, py, lineHue; static int mode; static bool hasLast, pend;   // mode 0 = 自動換色、1..8 = 固定顏色;pend:這次撞牆的線還沒畫(要等 draw 還原畫布後才能畫)
  static uint8_t back[BOX * BOX]; static int bx0, by0, bw, bh;   // 球下面那塊畫布的備份
  void init() { x = W / 2; y = H / 2; float a = frand() * 6.283f; vx = cosf(a) * 110; vy = sinf(a) * 110; hue = frand(); mode = 0; hasLast = pend = false; bw = bh = 0; cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (c.tap) tapKick(c, x, y, vx, vy, KICK);
    if (c.tapA || c.tapC) { mode = (mode + (c.tapC ? 1 : NMODE - 1)) % NMODE; if (mode) hue = (mode - 1) / 8.0f; snd::click(); }
    if (c.shake > SHAKE) { shakeKick(c, vx, vy); buzz(80, 30); }
    vx += c.gx * G * c.dt; vy += c.gy * G * c.dt;
    float v = sqrtf(vx * vx + vy * vy); if (v > 1e-3f) setSpeed(vx, vy, v < VMIN ? VMIN : v > VMAX ? VMAX + (v - VMAX) * expf(-1.5f * c.dt) : v);
    x += vx * c.dt; y += vy * c.dt;
    if (wallScreen(x, y, vx, vy, R) > 0) {
      pend = hasLast; px = lx; py = ly; lx = x; ly = y; hasLast = true; lineHue = hue;
      if (!mode) hue += 0.13f + frand() * 0.2f;
      snd::note(250 + frand() * 500); buzz(20, 8);
    }
  }
  void draw() {
    uint8_t* p = (uint8_t*)cv.getBuffer();
    for (int j = 0; j < bh; j++) memcpy(p + (by0 + j) * W + bx0, back + j * BOX, bw);   // 還原上一幀球蓋掉的區域
    if (pend) { cv.drawWideLine((int)px, (int)py, (int)lx, (int)ly, 1.5f, hsv(lineHue)); pend = false; }
    int ri = R + 2; bx0 = (int)x - ri; by0 = (int)y - ri; bw = bh = ri * 2 + 1;
    if (bx0 < 0) { bw += bx0; bx0 = 0; } if (by0 < 0) { bh += by0; by0 = 0; } if (bx0 + bw > W) bw = W - bx0; if (by0 + bh > H) bh = H - by0;
    for (int j = 0; j < bh; j++) memcpy(back + j * BOX, p + (by0 + j) * W + bx0, bw);
    cv.fillCircle((int)x, (int)y, R, hsv(hue));
  }
}
