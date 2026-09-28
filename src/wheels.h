#pragma once
#include "marblelib.h"

// ================= 25. 雙輪 =================
// 彈珠從頂端倒下,被屋頂分到左右兩個槳輪;槳輪不按不轉,彈珠會卡在槳葉的口袋裡。
// 按住 A 或畫面左半轉左輪、C 或右半轉右輪(都往中間帶),把口袋裡的彈珠倒下去。
// 鍵區觸控只回報一點,A+C 同按不會兩個都亮;畫面上能兩點,所以兩輪同轉要靠按畫面。彈珠依顏色堆進右邊的管子(見 marblelib.h)
namespace wheels {
  using namespace marblelib;
  constexpr int NW = 2, NP = 4; constexpr float WR = 56, OMEGA = 2.2f, ROOF_Y = 45, ROOF_A = 0.45f;   // 屋頂「^」:左半往右上、右半往右下
  static const float WX[NW] = { 62, 194 }, WY[NW] = { 130, 130 };
  static float th[NW], om[NW];
  void init() { reset(); th[0] = 0; th[1] = 0.4f; om[0] = om[1] = 0; }
  static void collide(M& o) {
    hitBar(o, AW / 2 - 30, ROOF_Y, 70, -ROOF_A); hitBar(o, AW / 2 + 30, ROOF_Y, 70, ROOF_A);   // 屋頂,跟 draw 用同一組角度
    for (int i = 0; i < NW; i++) { hitCirc(o, WX[i], WY[i], 6); for (int k = 0; k < NP; k++) { float a = th[i] + k * 1.5708f; hitBar(o, WX[i] + cosf(a) * WR / 2, WY[i] + sinf(a) * WR / 2, WR, a, om[i]); } }
  }
  void step(const Ctx& c) {
    bool on[NW] = { c.btnA, c.btnC };
    for (int i = 0; i < (int)M5.Touch.getCount(); i++) { auto& d = M5.Touch.getDetail(i); if (d.isPressed() && d.y < H && d.x < AW) on[d.x >= AW / 2] = true; }
    om[0] = on[0] ? OMEGA : 0; om[1] = on[1] ? -OMEGA : 0;   // 左輪順時針、右輪逆時針:上面的口袋往中間倒
    integrate(c, AW / 2, collide);
    for (int i = 0; i < NW; i++) th[i] += om[i] * c.dt;
  }
  void draw() {
    cv.fillScreen(0);
    uint32_t wc = rgb(130, 130, 145);
    for (int s = -1; s <= 1; s += 2) { float x0 = AW / 2 + s * 30, ux = cosf(ROOF_A) * 35, uy = sinf(ROOF_A * s) * 35; cv.drawLine((int)(x0 - ux), (int)(ROOF_Y - uy), (int)(x0 + ux), (int)(ROOF_Y + uy), wc); }
    for (int i = 0; i < NW; i++) {
      cv.drawCircle((int)WX[i], (int)WY[i], (int)WR + 2, rgb(50, 50, 60)); cv.fillCircle((int)WX[i], (int)WY[i], 6, wc);
      for (int k = 0; k < NP; k++) { float a = th[i] + k * 1.5708f; cv.drawLine((int)WX[i], (int)WY[i], (int)(WX[i] + cosf(a) * WR), (int)(WY[i] + sinf(a) * WR), om[i] != 0 ? rgb(255, 230, 0) : wc); }
    }
    drawMarbles(); hud("");
  }
}
