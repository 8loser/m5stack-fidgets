#pragma once
#include "numberlib.h"

// ================= 35. 比大小與加法 =================
// 兩種題目輪流:「哪邊大」把裝置往大的那邊傾斜(或 A 左 C 右)作答;「加起來是多少」三個選項點對的。
// 答錯扣一條命,3 條命用完結束,分數是答對題數,前 5 名存 NVS
namespace nbcompare {
  using namespace numberlib;
  static int a, b, opt[3], score, lives, rank, picked, ans; static float showT, tiltT, endT; static bool over, sum;
  static Board board = { "nbcompare" };
  static int maxN() { return score < 5 ? 10 : score < 12 ? 20 : 50; }
  static void next() {
    sum = score >= 3 && frand() < 0.5f; int m = maxN();
    a = 1 + (int)(frand() * m) % m; do { b = 1 + (int)(frand() * m) % m; } while (b == a);
    if (sum) { ans = a + b; makeOptions(ans, opt, 100); } else ans = a > b ? 0 : 2;
    picked = -1; showT = tiltT = 0;
  }
  void init() { score = 0; lives = 3; rank = -1; over = false; endT = 0; next(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); return; }
    if (picked >= 0) { if ((showT += c.dt) > 0.9f) { if (!lives) { over = true; rank = board.record(score); } else next(); } return; }
    int k = -1;
    if (sum) k = pickOption(c);
    else {   // 比大小:傾斜超過 0.4 g 持續 0.4 秒,或 A/C
      if (c.tapA) k = 0; if (c.tapC) k = 2;
      if (fabsf(c.gx) > 0.4f) { if ((tiltT += c.dt) > 0.4f) k = c.gx > 0 ? 2 : 0; } else tiltT = 0;
    }
    if (k < 0) return;
    picked = k; bool ok = sum ? opt[k] == ans : k == ans;
    if (ok) { score++; snd::note(600); buzz(40, 15); } else { lives--; snd::note(180); buzz(150, 100); }
  }
  void draw() {
    cv.fillScreen(rgb(150, 210, 230));
    cv.fillRect(0, 170, W, 26, rgb(200, 200, 200));
    bool res = picked >= 0, ok = res && (sum ? opt[picked] == ans : picked == ans);
    int bs = fitBs(a > b ? a : b, 120, 140);
    drawTower(90, 170, a, bs, true, !res ? face::CALM : ok ? face::HAPPY : face::SAD); drawTower(230, 170, b, bs, true, !res ? face::CALM : ok ? face::HAPPY : face::SAD);
    cv.setTextDatum(middle_center); cv.setTextSize(3); cv.setTextColor(rgb(30, 30, 40), rgb(150, 210, 230)); cv.drawString(sum ? "+" : "?", 160, 100);
    if (sum) drawOptions(opt, picked, res && !ok);
    else {
      cv.setTextSize(1); cv.setTextColor(rgb(60, 60, 80), rgb(150, 210, 230)); cv.drawString("which is bigger? tilt / A / C", 160, 217);
      if (res) { int wx = ans == 0 ? 90 : 230; cv.drawRoundRect(wx - 62, 20, 124, 155, 8, ok ? rgb(40, 160, 60) : rgb(200, 40, 40)); }
    }
    if (res && !ok && sum) { char s[8]; snprintf(s, sizeof s, "%d", ans); cv.setTextDatum(top_center); cv.setTextSize(3); cv.setTextColor(rgb(200, 40, 40), rgb(150, 210, 230)); cv.drawString(s, 160, 30); }
    char t[24]; snprintf(t, sizeof t, "score %d  best %u", score, board.best());
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(30, 30, 40), rgb(150, 210, 230)); cv.drawString(t, 4, 4); drawLives(lives);
    if (over) { snprintf(t, sizeof t, "SCORE %d", score); board.draw(t, rank); }
  }
}
