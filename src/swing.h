#pragma once
#include "ragdoll.h"

// ================= 20. 火柴人盪繩 =================
// 天花板一排錨點,火柴人單手吊著繩子盪;點螢幕放手飛出去,手靠近另一個錨點就自動抓住。
// 傾斜給擺盪加力。掉到地上就暈一下、重新吊回第一個錨點。計數:連續抓到幾個錨點
namespace swing {
  using namespace ragdoll; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int NA = 5, AY = 18, FLOOR = H - 4; constexpr float ROPE = 75, GRAB_R = 20, GSW = 700;
  static const int16_t AX[NA] = { 36, 98, 160, 222, 284 };
  static int anchor, hand, grabs, best; static float freeT, fallT;
  static void hang(int a) { anchor = a; hand = RHAND; freeT = 0; place(AX[a], AY + ROPE + 10); p[hand].x = p[hand].ox = AX[a]; p[hand].y = p[hand].oy = AY + ROPE; for (auto& q : p) q.ox -= 4; }   // 給一點初速開始盪
  void init() { grabs = best = 0; fallT = 0; hang(0); ringlib::reset(); cv.fillScreen(0); }
  static float floorHit(Pt& q) {   // 地板與螢幕左右
    float v = pushOut(q, 0, -1, q.y - FLOOR); v = fmaxf(v, pushOut(q, 1, 0, 2 - q.x)); return fmaxf(v, pushOut(q, -1, 0, q.x - (W - 3)));
  }
  void step(const Ctx& c) {
    if (fallT > 0) { fallT -= c.dt; if (fallT <= 0) hang(0); }
    else if (c.tap && anchor >= 0) { anchor = -1; freeT = 0; snd::note(520); buzz(40, 15); }   // 放手
    freeT += c.dt;
    integrate(c, GSW);
    float maxV = 0; int maxI = -1;
    for (int it = 0; it < ITER; it++) {
      solveLinks();
      if (anchor >= 0) {   // 繩子:只拉不推
        auto& q = p[hand]; float dx = q.x - AX[anchor], dy = q.y - AY, d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        if (d > ROPE) { q.x -= dx / d * (d - ROPE); q.y -= dy / d * (d - ROPE); }
      }
      for (int i = 0; i < NP; i++) { float v = floorHit(p[i]) / c.dt; if (v > maxV) { maxV = v; maxI = i; } }
    }
    if (anchor < 0 && freeT > 0.25f && fallT <= 0) for (int h : { LHAND, RHAND }) for (int a = 0; a < NA && anchor < 0; a++) {   // 自動抓
      float dx = p[h].x - AX[a], dy = p[h].y - AY; if (dx * dx + dy * dy < GRAB_R * GRAB_R) { anchor = a; hand = h; grabs++;  spark(AX[a], AY, rgb(255, 230, 0)); snd::note(400 + a * 60); buzz(60, 20); }
    }
    bool floored = false; for (auto& q : p) floored |= q.y > FLOOR - 1;
    if (anchor < 0 && floored && fallT <= 0) { fallT = 1.5f; if (grabs > best) best = grabs; dizzyT = 1.5f; snd::note(160); buzz(150, 80); }
    react(maxV, maxI); updateMood(c.dt, anchor >= 0 && headV > 60); stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    cv.drawFastHLine(0, AY - 6, W, rgb(90, 110, 140)); cv.drawFastHLine(0, FLOOR + 1, W, rgb(90, 110, 140));
    for (int a = 0; a < NA; a++) cv.fillCircle(AX[a], AY, 3, a == anchor ? rgb(255, 230, 0) : rgb(160, 160, 180));
    if (anchor >= 0) cv.drawLine(AX[anchor], AY, (int)p[hand].x, (int)p[hand].y, rgb(200, 160, 90));
    drawBody(); drawSparks();
    char t[32]; snprintf(t, sizeof t, "grabs %d  best %d  %s", grabs, best, face::NAME[mood]);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 26);
  }
}
