#pragma once
#include "marblelib.h"

// ================= 彈珠機關 =================
// 把原本的轉環引路、蹺蹺板、雙輪、尖刺碗由上往下串成一條:彈珠倒進頂端兩層轉環,從缺口掉到兩塊蹺蹺板,
// 經過左右槳輪與中間的小槳輪(都自由轉動,被彈珠撞到或壓著就跟著轉)、彈跳柱(兩側各兩根、中間三根,碰到就被彈開)、斜板,落進底部的尖刺碗(碰到刺就破),轉碗才倒得出去。
// A / C 讓全部機關一起往逆 / 順時針轉(轉環、蹺蹺板、槳輪、碗);點蹺蹺板一端壓下去、按住槳輪用馬達轉它(往中間帶)。
// 碗放開慢慢回正。彈珠依顏色堆進右邊的管子(見 marblelib.h)
namespace marblerun {
  using namespace marblelib;
  constexpr int CX = AW / 2, NR = 2, NS = 2, NW = 3, NP = 4, NSPIKE = 3;
  constexpr float RCY = 8, RAD[NR] = { 30, 46 }, GAP[NR] = { 40, 34 }, DRIFT[NR] = { 14, -10 };          // 轉環
  constexpr float SLEN = 60, LIM = 0.6f, TORQUE = 0.6f, DAMP = 1.5f;                                         // 蹺蹺板
  constexpr float OMEGA = 2.2f, WK = 1.5e-4f, WDAMP = 1.2f, WMAX = 6;                                     // 槳輪;WK:彈珠動量換成大槳輪角速度的比例,小的照半徑平方放大(比較輕)
  constexpr float BCY = 182, BOWL_R = 46, A0 = 20, A1 = 160, SPIKE_A[NSPIKE] = { 70, 90, 110 }, SPIKE_L = 13;   // 尖刺碗
  constexpr int NB = 7; constexpr float BUMP_R = 4, BKICK = 240;                                                 // 彈跳柱:撞到往外補一記速度
  constexpr float FUN_LEN = 87, FUN_A = 0.278f;                                                              // 斜板:把兩邊掉下來的彈珠送進碗
  static const float SX[NS] = { 84, 172 }, SY[NS] = { 70, 70 }, WX[NW] = { 64, 192, 125 }, WY[NW] = { 140, 140, 128 }, WR[NW] = { 24, 24, 12 }, WIN[NW] = { 1, -1, 1 }, FX[2] = { 50, 206 }, FY = 172,   // 槳輪:左、右、兩輪中間的小的;WIN 是按住時的方向(往中間帶)
                     BX[NB] = { 125, 104, 146, 28, 28, 228, 228 }, BY[NB] = { 176, 154, 154, 100, 126, 100, 126 };   // 碗口一根、它上方左右兩根、兩側各兩根
  static float ang[NR], th[NS], om[NS], tq[NS], wth[NW], wom[NW], wimp[NW], ba, flash[NB]; static bool drive[NW];
  // 轉環每層兩個缺口相差 90°(在 ang 與 ang + 90),轉到兩個都朝下時一次從兩邊放球;h = 0 是兩缺口之間的短弧、1 是長弧
  static float arcA(int i, int h) { return ang[i] + h * 90 + GAP[i] / 2; }
  static float arcB(int i, int h) { return ang[i] + (h ? 360 : 90) - GAP[i] / 2; }

  void init() {
    reset(); ba = 0;
    for (int i = 0; i < NR; i++) ang[i] = frand() * 180;
    for (int i = 0; i < NS; i++) { th[i] = (frand() - 0.5f) * 0.4f; om[i] = tq[i] = 0; }
    for (int i = 0; i < NW; i++) { wth[i] = i * 0.4f; wom[i] = 0; }
  }
  static void tip(int k, float& x, float& y) { float a = (SPIKE_A[k] + ba) * DEG_TO_RAD; x = CX + (BOWL_R - SPIKE_L) * cosf(a); y = BCY + (BOWL_R - SPIKE_L) * sinf(a); }
  static void collide(M& o) {
    for (int i = 0; i < NR; i++) for (int h = 0; h < 2; h++) hitArc(o, CX, RCY, RAD[i], arcA(i, h), arcB(i, h));
    for (int i = 0; i < NS; i++) if (hitBar(o, SX[i], SY[i], SLEN, th[i], om[i])) tq[i] += barT / SUB;   // 壓在離軸多遠就多少力矩
    for (int i = 0; i < NW; i++) {
      hitCirc(o, WX[i], WY[i], WR[i] / 5);
      for (int k = 0; k < NP; k++) {   // 槳葉推彈珠多少,彈珠就反推槳輪多少(角動量 r x dv);壓著不動的彈珠每子步也被重力推一點,所以有重量
        float a = wth[i] + k * 1.5708f, vx0 = o.vx, vy0 = o.vy;
        if (hitBar(o, WX[i] + cosf(a) * WR[i] / 2, WY[i] + sinf(a) * WR[i] / 2, WR[i], a, wom[i])) { float r = WR[i] / 2 + barT; wimp[i] -= (cosf(a) * r * (o.vy - vy0) - sinf(a) * r * (o.vx - vx0)) * WK * (576 / (WR[i] * WR[i])); }
      }
    }
    for (int b = 0; b < NB; b++) if (hitCirc(o, BX[b], BY[b], BUMP_R)) {
      float dx = o.x - BX[b], dy = o.y - BY[b], d = sqrtf(dx * dx + dy * dy) + 1e-3f; o.vx += dx / d * BKICK; o.vy += dy / d * BKICK;
      if (flash[b] <= 0) { snd::note(520 + b * 130); buzz(40, 10); } flash[b] = 0.12f;
    }
    hitBar(o, FX[0], FY, FUN_LEN, FUN_A); hitBar(o, FX[1], FY, FUN_LEN, -FUN_A);
    hitArc(o, CX, BCY, BOWL_R, A0 + ba, A1 + ba);
    for (int k = 0; k < NSPIKE; k++) { float tx, ty; tip(k, tx, ty); float dx = o.x - tx, dy = o.y - ty; if (dx * dx + dy * dy < 16) { o.live = false; lost++; spark(o.x, o.y, col(o.col)); snd::note(200); buzz(30, 10); return; } }
  }
  void step(const Ctx& c) {
    float dir = (float)(c.btnC - c.btnA);   // C 順時針、A 逆時針
    if (c.tap) for (int i = 0; i < NS; i++) if (fabsf(c.ty - SY[i]) < 22 && fabsf(c.tx - SX[i]) < SLEN / 2) om[i] += (c.tx > SX[i] ? 1 : -1) * 4;   // 點哪一端就壓哪一端
    bool on[NW] = {};
    for (int i = 0; i < (int)M5.Touch.getCount(); i++) {
      auto& d = M5.Touch.getDetail(i); if (!d.isPressed() || d.y >= H) continue;
      for (int w = 0; w < NW; w++) { float dx = d.x - WX[w], dy = d.y - WY[w]; if (dx * dx + dy * dy < (WR[w] + 10) * (WR[w] + 10)) on[w] = true; }
    }
    for (int i = 0; i < NR; i++) ang[i] += (DRIFT[i] + dir * 60) * c.dt;
    ba += dir * 90 * c.dt; ba = ba < -110 ? -110 : ba > 110 ? 110 : ba; if (dir == 0) ba *= 1 - 1.5f * c.dt;   // 碗放開慢慢回正
    memset(tq, 0, sizeof tq); memset(wimp, 0, sizeof wimp);
    integrate(c, CX, collide);
    for (int i = 0; i < NS; i++) {
      om[i] += (tq[i] * TORQUE + dir * 3 - om[i] * DAMP) * c.dt; th[i] += om[i] * c.dt;
      if (th[i] > LIM) { th[i] = LIM; om[i] = -om[i] * 0.3f; } if (th[i] < -LIM) { th[i] = -LIM; om[i] = -om[i] * 0.3f; }
    }
    for (int i = 0; i < NW; i++) {   // 按住或 A / C:馬達帶到定速(按住往中間帶);沒操作就自由轉,吃彈珠的衝量、慢慢停
      drive[i] = on[i] || dir != 0;
      if (on[i]) wom[i] = WIN[i] * OMEGA; else if (dir != 0) wom[i] = dir * OMEGA;
      else { wom[i] = (wom[i] + wimp[i]) * expf(-WDAMP * c.dt); wom[i] = fmaxf(-WMAX, fminf(WMAX, wom[i])); }
      wth[i] += wom[i] * c.dt;
    }
    for (auto& f : flash) f -= c.dt;
  }
  void draw() {
    cv.fillScreen(0);
    uint32_t wc = rgb(130, 130, 145);
    for (int i = 0; i < NR; i++) for (int h = 0; h < 2; h++) ringlib::arc(CX, RCY, RAD[i], arcA(i, h), arcB(i, h), hsv(0.55f + i * 0.12f));
    for (int i = 0; i < NS; i++) {
      float ux = cosf(th[i]) * SLEN / 2, uy = sinf(th[i]) * SLEN / 2;
      cv.drawLine((int)(SX[i] - ux), (int)(SY[i] - uy), (int)(SX[i] + ux), (int)(SY[i] + uy), wc); cv.drawLine((int)(SX[i] - ux), (int)(SY[i] - uy) + 1, (int)(SX[i] + ux), (int)(SY[i] + uy) + 1, wc);
      cv.fillTriangle((int)SX[i], (int)SY[i], (int)SX[i] - 6, (int)SY[i] + 10, (int)SX[i] + 6, (int)SY[i] + 10, rgb(90, 90, 100));
    }
    for (int i = 0; i < NW; i++) {
      cv.drawCircle((int)WX[i], (int)WY[i], (int)WR[i] + 2, rgb(50, 50, 60)); cv.fillCircle((int)WX[i], (int)WY[i], (int)(WR[i] / 5), wc);
      for (int k = 0; k < NP; k++) { float a = wth[i] + k * 1.5708f; cv.drawLine((int)WX[i], (int)WY[i], (int)(WX[i] + cosf(a) * WR[i]), (int)(WY[i] + sinf(a) * WR[i]), drive[i] ? rgb(255, 230, 0) : wc); }
    }
    for (int s = 0; s < 2; s++) { float a = s ? -FUN_A : FUN_A, ux = cosf(a) * FUN_LEN / 2, uy = sinf(a) * FUN_LEN / 2; cv.drawLine((int)(FX[s] - ux), (int)(FY - uy), (int)(FX[s] + ux), (int)(FY + uy), wc); }
    for (int b = 0; b < NB; b++) { cv.fillCircle((int)BX[b], (int)BY[b], (int)BUMP_R, flash[b] > 0 ? rgb(255, 255, 255) : rgb(230, 60, 140)); cv.drawCircle((int)BX[b], (int)BY[b], (int)BUMP_R + 2, rgb(255, 160, 210)); }
    ringlib::arc(CX, BCY, BOWL_R, A0 + ba, A1 + ba, rgb(200, 170, 90));
    for (int k = 0; k < NSPIKE; k++) {   // 刺:底在碗上、尖朝碗心
      float a = (SPIKE_A[k] + ba) * DEG_TO_RAD, tx, ty; tip(k, tx, ty);
      cv.fillTriangle((int)tx, (int)ty, (int)(CX + BOWL_R * cosf(a - 0.06f)), (int)(BCY + BOWL_R * sinf(a - 0.06f)), (int)(CX + BOWL_R * cosf(a + 0.06f)), (int)(BCY + BOWL_R * sinf(a + 0.06f)), rgb(255, 255, 255));
    }
    drawMarbles(); hud("popped");
  }
}
