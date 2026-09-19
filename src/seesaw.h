#pragma once
#include "marblelib.h"

// ================= 27. 蹺蹺板 =================
// 四塊蹺蹺板交錯排列,彈珠壓在哪一邊那邊就往下沉,沉到底就倒出去;點蹺蹺板的一端把它壓下去,
// A/C 把全部蹺蹺板往左 / 右邊壓。彈珠依顏色堆進右邊的管子(見 marblelib.h)
namespace seesaw {
  using namespace marblelib;
  constexpr int NS = 4; constexpr float LEN = 100, LIM = 0.6f, TORQUE = 0.6f, DAMP = 1.5f;
  static const float PX[NS] = { 68, 188, 68, 188 }, PY[NS] = { 60, 105, 150, 195 };
  static float th[NS], om[NS], tq[NS];
  void init() { reset(); for (int i = 0; i < NS; i++) { th[i] = (frand() - 0.5f) * 0.4f; om[i] = tq[i] = 0; } }
  static void collide(M& o) { for (int i = 0; i < NS; i++) if (hitBar(o, PX[i], PY[i], LEN, th[i], om[i])) tq[i] += barT / SUB; }   // 壓在離軸多遠就多少力矩
  void step(const Ctx& c) {
    if (c.tap) for (int i = 0; i < NS; i++) if (fabsf(c.ty - PY[i]) < 22 && fabsf(c.tx - PX[i]) < LEN / 2) om[i] += (c.tx > PX[i] ? 1 : -1) * 4;   // 點哪一端就壓哪一端
    memset(tq, 0, sizeof tq);
    integrate(c, 68, collide);
    for (int i = 0; i < NS; i++) {
      om[i] += (tq[i] * TORQUE + (c.btnC - c.btnA) * 3 - om[i] * DAMP) * c.dt; th[i] += om[i] * c.dt;
      if (th[i] > LIM) { th[i] = LIM; om[i] = -om[i] * 0.3f; } if (th[i] < -LIM) { th[i] = -LIM; om[i] = -om[i] * 0.3f; }
    }
  }
  void draw() {
    cv.fillScreen(0);
    for (int i = 0; i < NS; i++) {
      float ux = cosf(th[i]) * LEN / 2, uy = sinf(th[i]) * LEN / 2; uint32_t wc = rgb(130, 130, 145);
      cv.drawLine((int)(PX[i] - ux), (int)(PY[i] - uy), (int)(PX[i] + ux), (int)(PY[i] + uy), wc); cv.drawLine((int)(PX[i] - ux), (int)(PY[i] - uy) + 1, (int)(PX[i] + ux), (int)(PY[i] + uy) + 1, wc);
      cv.fillTriangle((int)PX[i], (int)PY[i], (int)PX[i] - 6, (int)PY[i] + 10, (int)PX[i] + 6, (int)PY[i] + 10, rgb(90, 90, 100));
    }
    drawMarbles(); hud("");
  }
}
