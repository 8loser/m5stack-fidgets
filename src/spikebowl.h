#pragma once
#include "marblelib.h"

// ================= 29. 尖刺碗 =================
// 彈珠倒進一個碗,碗底有三根尖刺,碰到尖端的彈珠會破掉(進不了管子)。
// 傾斜只推彈珠,碗不跟著動;A/C 轉碗(A 順時針、C 逆時針,轉到夠斜碗底就朝側面倒出去),放開慢慢回正。
// 彈珠依顏色堆進右邊的管子(見 marblelib.h)
namespace spikebowl {
  using namespace marblelib;
  constexpr int CX = AW / 2, CY = 80, NSPIKE = 3; constexpr float BOWL_R = 88, A0 = 22, A1 = 158, SPIKE_A[NSPIKE] = { 68, 90, 112 }, SPIKE_L = 16;
  static float ba;
  void init() { reset(); ba = 0; }
  static void tip(int k, float& x, float& y) { float a = (SPIKE_A[k] + ba) * DEG_TO_RAD; x = CX + (BOWL_R - SPIKE_L) * cosf(a); y = CY + (BOWL_R - SPIKE_L) * sinf(a); }
  static void collide(M& o) {
    hitArc(o, CX, CY, BOWL_R, A0 + ba, A1 + ba);
    for (int k = 0; k < NSPIKE; k++) { float tx, ty; tip(k, tx, ty); float dx = o.x - tx, dy = o.y - ty; if (dx * dx + dy * dy < 16) { o.live = false; lost++; spark(o.x, o.y, col(o.col)); snd::note(200); buzz(30, 10); return; } }
  }
  void step(const Ctx& c) {
    ba += (c.btnA - c.btnC) * 90 * c.dt; ba = ba < -110 ? -110 : ba > 110 ? 110 : ba; if (!c.btnA && !c.btnC) ba *= 1 - 1.5f * c.dt;   // A/C 轉碗,放開慢慢回正
    integrate(c, CX, collide);
  }
  void draw() {
    cv.fillScreen(0);
    ringlib::arc(CX, CY, BOWL_R, A0 + ba, A1 + ba, rgb(200, 170, 90));
    for (int k = 0; k < NSPIKE; k++) {   // 刺:底在碗上、尖朝碗心
      float a = (SPIKE_A[k] + ba) * DEG_TO_RAD, tx, ty; tip(k, tx, ty);
      cv.fillTriangle((int)tx, (int)ty, (int)(CX + BOWL_R * cosf(a - 0.06f)), (int)(CY + BOWL_R * sinf(a - 0.06f)), (int)(CX + BOWL_R * cosf(a + 0.06f)), (int)(CY + BOWL_R * sinf(a + 0.06f)), rgb(255, 255, 255));
    }
    drawMarbles(); hud("popped");
  }
}
