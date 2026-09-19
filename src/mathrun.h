#pragma once
#include "ringlib.h"
#include "face.h"

// ================= 36. 算數跑酷(左移右移)=================
// 角色在透視跑道上一直往前跑,前方不斷來「門」,門的左右兩格是兩個答案,題目在上面;
// 傾斜(或 A/C、點螢幕左右半邊)把角色移到正確那一邊穿過去。走錯就結束,分數是通過幾道門,前 5 名存 NVS。
// 難度隨關卡:加法 → 減法 → 乘法 → 除法 → 混合,數字越來越大、門來得越來越快
namespace mathrun {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int HORIZON = 50, NEAR = 205, ROAD_FAR = 40, ROAD_NEAR = 300; constexpr float V0 = 0.35f, VSTEP = 0.012f;
  static int lane, level, rank, ans, wrong, a, b, op; static bool leftIsAns, over, passed; static float z, lx, endT, runT, shakeT;
  static Board board = { "mathrun" };
  static void question() {   // 依關卡出題:0 加 1 減 2 乘 3 除
    int L = level; op = L < 5 ? 0 : L < 10 ? 1 : L < 16 ? 2 : L < 22 ? 3 : (int)(frand() * 4) % 4;
    int m = op == 2 || op == 3 ? (L < 16 ? 5 : L < 30 ? 9 : 12) : (L < 5 ? 10 : L < 10 ? 20 : L < 30 ? 50 : 100);
    switch (op) {
      case 0: a = 1 + (int)(frand() * m) % m; b = 1 + (int)(frand() * m) % m; ans = a + b; break;
      case 1: a = 1 + (int)(frand() * m) % m; b = 1 + (int)(frand() * a) % a; ans = a - b; break;
      case 2: a = 2 + (int)(frand() * m) % m; b = 2 + (int)(frand() * m) % m; ans = a * b; break;
      default: b = 2 + (int)(frand() * m) % m; ans = 1 + (int)(frand() * m) % m; a = ans * b;
    }
    do { int d = (int)(frand() * 7) - 3; wrong = ans + (d ? d : 1) * (op >= 2 && frand() < 0.5f ? b : 1); } while (wrong == ans || wrong < 0);   // 錯的選項:差一點,乘除有時差一個倍數
    leftIsAns = frand() < 0.5f; z = 1; passed = false;
  }
  void init() { lane = 0; level = 0; rank = -1; over = false; endT = runT = shakeT = 0; lx = 0; question(); ringlib::reset(); cv.fillScreen(0); }
  static int sy(float zz) { return HORIZON + (int)((NEAR - HORIZON) * (1 - zz) * (1 - zz)); }   // z 1 = 遠,0 = 眼前;平方讓遠處變慢
  static float scale(float zz) { return 0.25f + 0.75f * (1 - zz); }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); stepSparks(c.dt); return; }
    runT += c.dt; shakeT -= c.dt;
    if (c.gx < -0.25f || c.btnA || (c.tap && c.tx < W / 2)) lane = -1; if (c.gx > 0.25f || c.btnC || (c.tap && c.tx >= W / 2)) lane = 1;   // 左 / 右車道
    lx += (lane * 0.5f - lx) * 8 * c.dt;   // -0.5 / 0.5 之間滑動
    z -= (V0 + level * VSTEP) * c.dt;
    if (z <= 0.05f && !passed) {   // 門到眼前:判定
      passed = true; bool ok = (lane < 0) == leftIsAns;
      if (ok) { level++; spark(160, 180, rgb(80, 220, 120)); snd::note(500 + (level % 8) * 40); buzz(40, 15); question(); }
      else { over = true; rank = board.record(level); shakeT = 1; spark(160, 180, rgb(255, 60, 60)); snd::note(150); buzz(255, 400); }
    }
    stepSparks(c.dt);
  }
  static void drawGate(float zz) {
    float s = scale(zz); int y = sy(zz), half = (int)((ROAD_NEAR - ROAD_FAR) / 2 * s), gw = (int)(120 * s), gh = (int)(36 * s);
    int lxp = 160 - half / 2, rxp = 160 + half / 2;   // 兩格的中心
    for (int k = -1; k <= 1; k += 2) {
      int cx = k < 0 ? lxp : rxp; bool isAns = (k < 0) == leftIsAns;
      cv.fillRect(cx - gw / 2, y - gh, gw, gh, isAns && passed ? rgb(40, 140, 60) : rgb(40, 40, 70)); cv.drawRect(cx - gw / 2, y - gh, gw, gh, rgb(200, 200, 230));
      char t[8]; snprintf(t, sizeof t, "%d", isAns ? ans : wrong); cv.setTextDatum(middle_center); cv.setTextSize(s > 0.6f ? 2 : 1); cv.setTextColor(rgb(255, 255, 255), rgb(40, 40, 70)); cv.drawString(t, cx, y - gh / 2);
    }
    cv.fillRect(160 - 2, y - gh, 4, gh, rgb(200, 200, 230));   // 中柱
  }
  void draw() {
    cv.fillScreen(rgb(120, 170, 230)); cv.fillRect(0, HORIZON, W, H - HORIZON, rgb(60, 140, 70));
    int x0 = (W - ROAD_FAR) / 2, x1 = (W + ROAD_FAR) / 2, X0 = (W - ROAD_NEAR) / 2, X1 = (W + ROAD_NEAR) / 2;   // 跑道梯形
    cv.fillTriangle(x0, HORIZON, x1, HORIZON, X1, H, rgb(90, 90, 100)); cv.fillTriangle(x0, HORIZON, X1, H, X0, H, rgb(90, 90, 100));
    for (int i = 0; i < 6; i++) { float zz = fmodf(i / 6.0f + runT * 0.4f, 1); int y = sy(zz), hw = (int)(ROAD_FAR / 2 + (ROAD_NEAR - ROAD_FAR) / 2 * (1 - zz)); cv.drawFastHLine(160 - hw, y, hw * 2, rgb(130, 130, 140)); }   // 往後流的橫紋
    cv.drawLine(160, HORIZON, 160, H, rgb(200, 200, 60));
    drawGate(z);
    // 角色:一顆有臉的球加兩條跑動的腿
    int px = 160 + (int)(lx * 110), py = NEAR + 8; float leg = sinf(runT * 14) * 6;
    if (shakeT > 0) px += (int)(sinf(shakeT * 60) * 4);
    cv.drawLine(px, py + 6, px - 5 + (int)leg, py + 22, rgb(255, 255, 255)); cv.drawLine(px, py + 6, px + 5 - (int)leg, py + 22, rgb(255, 255, 255));
    cv.fillCircle(px, py, 11, rgb(220, 120, 30)); face::draw(px, py, 1, 0, 0, 1, over ? face::DEAD : z < 0.3f ? face::SCARED : face::HAPPY, 1.2f);
    drawSparks();
    static const char OPS[4] = { '+', '-', 'x', '/' };
    char q[24]; snprintf(q, sizeof q, "%d %c %d = ?", a, OPS[op], b);
    cv.fillRoundRect(90, 6, 140, 30, 6, rgb(255, 255, 255)); cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(rgb(30, 30, 40), rgb(255, 255, 255)); cv.drawString(q, 160, 21);
    char t[24]; snprintf(t, sizeof t, "level %d  best %u", level, board.best());
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(255, 255, 255), rgb(120, 170, 230)); cv.drawString(t, 4, 4);
    if (over) { snprintf(t, sizeof t, "LEVEL %d", level); board.draw(t, rank); }
  }
}
