#pragma once
#include "marblelib.h"

// ================= 26. 轉環引路 =================
// 彈珠倒進三層同心弧環的中心,每層環有兩個對開的缺口;A/C 轉整組環讓缺口對上,彈珠才出得來。
// 彈珠依顏色堆進右邊的管子(見 marblelib.h)
namespace arcring {
  using namespace marblelib;
  constexpr int CX = AW / 2, CY = 20, NR = 3; constexpr float RAD[NR] = { 50, 82, 114 }, GAP[NR] = { 44, 38, 34 }, DRIFT[NR] = { 12, -9, 7 };
  static float ang[NR];
  void init() { reset(); for (int i = 0; i < NR; i++) ang[i] = frand() * 180; }
  static void collide(M& o) { for (int i = 0; i < NR; i++) for (int h = 0; h < 2; h++) hitArc(o, CX, CY, RAD[i], ang[i] + h * 180 + GAP[i] / 2, ang[i] + h * 180 + 180 - GAP[i] / 2); }
  void step(const Ctx& c) {
    for (int i = 0; i < NR; i++) ang[i] += (DRIFT[i] + (c.btnC - c.btnA) * 60) * c.dt;
    integrate(c, CX, collide);
  }
  void draw() {
    cv.fillScreen(0);
    for (int i = 0; i < NR; i++) for (int h = 0; h < 2; h++) ringlib::arc(CX, CY, RAD[i], ang[i] + h * 180 + GAP[i] / 2, ang[i] + h * 180 + 180 - GAP[i] / 2, hsv(0.55f + i * 0.12f));
    drawMarbles(); hud("");
  }
}
