#pragma once
#include "numberlib.h"

// ================= 33. 看塔猜數 =================
// 畫一座沒標數字的塔,下面三個選項點對的(A 左、C 右、點中間)。答對加分,每 5 題數字範圍變大(10 → 30 → 100);
// 答錯扣一條命,3 條命用完結束,分數是答對題數,前 5 名存 NVS
namespace nbguess {
  using namespace numberlib;
  static int n, opt[3], score, lives, rank, picked; static float showT, endT; static bool over;
  static Board board = { "nbguess" };
  static int maxN() { return score < 5 ? 10 : score < 12 ? 30 : 100; }
  static void next() { n = 1 + (int)(frand() * maxN()) % maxN(); makeOptions(n, opt, maxN()); picked = -1; showT = 0; }
  void init() { score = 0; lives = 3; rank = -1; over = false; endT = 0; next(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); return; }
    if (picked >= 0) { if ((showT += c.dt) > 0.8f) { if (!lives) { over = true; rank = board.record(score); } else next(); } return; }   // 顯示結果 0.8 秒
    int k = pickOption(c); if (k < 0) return;
    picked = k;
    if (opt[k] == n) { score++; snd::note(600); buzz(40, 15); } else { lives--; snd::note(180); buzz(150, 100); }
  }
  void draw() {
    cv.fillScreen(rgb(150, 210, 230));
    cv.fillRect(0, 170, W, 26, rgb(200, 200, 200));
    int bs = fitBs(n, 200, 150); drawTower(160, 170, n, bs, true, picked < 0 ? face::CALM : opt[picked] == n ? face::HAPPY : face::SAD);
    drawOptions(opt, picked, picked >= 0 && opt[picked] != n);
    if (picked >= 0 && opt[picked] != n) { char s[8]; snprintf(s, sizeof s, "%d", n); cv.setTextDatum(top_center); cv.setTextSize(3); cv.setTextColor(rgb(200, 40, 40), rgb(150, 210, 230)); cv.drawString(s, 260, 30); }
    char t[24]; snprintf(t, sizeof t, "score %d  best %u", score, board.best());
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(30, 30, 40), rgb(150, 210, 230)); cv.drawString(t, 4, 4); drawLives(lives);
    if (over) { snprintf(t, sizeof t, "SCORE %d", score); board.draw(t, rank); }
  }
}
