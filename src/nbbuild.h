#pragma once
#include "numberlib.h"

// ================= 34. 湊出這個數 =================
// 上面給目標數字,A 加一根 10 的柱、C 加 1 塊、點塔拿掉 1 塊、點上面的目標框送出。
// 疊到剛好就得分換下一題;錯了會顯示差多少。60 秒內湊對幾題,前 5 名存 NVS
namespace nbbuild {
  using namespace numberlib;
  constexpr float ROUND = 60;
  static int target, n, score, rank, diff; static float roundT, msgT, endT; static bool over;
  static Board board = { "nbbuild" };
  static int maxN() { return score < 3 ? 10 : score < 8 ? 40 : 100; }
  static void next() { target = 1 + (int)(frand() * maxN()) % maxN(); n = 0; diff = 0; msgT = 0; }
  void init() { score = 0; rank = -1; roundT = endT = 0; over = false; next(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); return; }
    if ((roundT += c.dt) >= ROUND) { over = true; rank = board.record(score); buzz(150, 150); snd::note(600); return; }
    if (msgT > 0) { msgT -= c.dt; return; }
    if (c.tapA && n + 10 <= 100) { n += 10; snd::note(500); }
    if (c.tapC && n < 100) { n++; snd::note(400 + (n % 10) * 30); }
    if (c.tap && c.ty < 50) {   // 送出
      if (n == target) { score++; snd::note(700); buzz(60, 30); next(); } else { diff = n - target; msgT = 1.2f; snd::note(180); buzz(120, 80); }
    } else if (c.tap && n > 0) { n--; snd::click(); }
  }
  void draw() {
    cv.fillScreen(rgb(150, 210, 230));
    cv.fillRoundRect(110, 8, 100, 40, 6, rgb(255, 255, 255)); cv.drawRoundRect(110, 8, 100, 40, 6, rgb(60, 60, 80));
    char s[16]; snprintf(s, sizeof s, "%d", target); cv.setTextDatum(middle_center); cv.setTextSize(3); cv.setTextColor(rgb(30, 30, 40), rgb(255, 255, 255)); cv.drawString(s, 160, 28);
    cv.setTextSize(1); cv.setTextColor(rgb(60, 60, 80), rgb(150, 210, 230)); cv.drawString("tap here to submit", 160, 56);
    cv.fillRect(0, 210, W, 30, rgb(200, 200, 200));
    int bs = fitBs(target > n ? target : n, 220, 140); drawTower(160, 210, n, bs, n > 0, msgT > 0 ? face::SAD : face::CALM);
    cv.setTextDatum(middle_left); cv.setTextColor(rgb(30, 30, 40), rgb(200, 200, 200)); cv.drawString("A +10", 6, 224); cv.setTextDatum(middle_right); cv.drawString("C +1", W - 6, 224);
    cv.setTextDatum(middle_center); snprintf(s, sizeof s, "%d  (tap: -1)", n); cv.drawString(s, 160, 224);
    if (msgT > 0) { snprintf(s, sizeof s, diff > 0 ? "%d too many" : "%d short", diff > 0 ? diff : -diff); cv.setTextSize(2); cv.setTextColor(rgb(200, 40, 40), rgb(150, 210, 230)); cv.drawString(s, 160, 100); }
    char t[32]; snprintf(t, sizeof t, "score %d  %ds  best %u", score, (int)(ROUND - roundT), board.best());
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(30, 30, 40), rgb(150, 210, 230)); cv.drawString(t, 4, 4);
    if (over) { snprintf(t, sizeof t, "SCORE %d", score); board.draw(t, rank); }
  }
}
