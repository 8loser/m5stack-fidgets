#pragma once
#include "common.h"
#include "jamgen.h"
#include <esp_random.h>

// ================= 塞車 =================
// Rush Hour:6x6 停車場,每台車只能沿自己的方向滑,把紅車從右邊出口挪出去。關卡即時產生(jamgen.h),
// 目標最少步數每關 +2(4 起、15 封頂);產生器 3 秒內湊不到目標就用這段期間最難的那盤。
// 拖車移動(一次拖放算一步);A 重來這關、C 提示走一步(扣 10 秒)。180 秒內解幾關,前 5 名存 NVS
// 選單預覽沒有觸控,改成照提示自動解
namespace jam {
  using namespace jamgen;
  constexpr int CAP = 4096, CS = 36, BX = 12, BY = 12, PX = 240;
  constexpr float ROUND = 180, HINT_COST = 10;
  constexpr uint32_t GEN_MS = 3000;
  static Table tab = { CAP };
  static Puzzle P; static uint64_t s0, s;
  static int level, moves, best, rank, grab; static float lo, hi, off, t0, left, winT, endT, autoT; static uint32_t genStart, bestSeed; static bool gen, over;
  static Board board = { "jam" };
  static int target() { int t = 4 + 2 * level; return t > 15 ? 15 : t; }
  static void newLevel() { gen = true; genStart = millis(); best = 0; grab = -1; winT = 0; }
  void init() {
    if (!tab.st) {
      tab.st = (uint64_t*)heap_caps_malloc(CAP * sizeof(uint64_t), MALLOC_CAP_SPIRAM); tab.dist = (uint8_t*)heap_caps_malloc(CAP, MALLOC_CAP_SPIRAM);
      tab.hs = (int32_t*)heap_caps_malloc(2 * CAP * sizeof(int32_t), MALLOC_CAP_SPIRAM); tab.q = (int32_t*)heap_caps_malloc(CAP * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    }
    level = 0; left = ROUND; rank = -1; over = false; endT = autoT = 0; newLevel();
  }
  // ponytail: 產生在主迴圈裡做,每幀最多約 30 ms(單次嘗試可能超過);卡頓明顯再搬到另一顆核心的 task
  static void genStep() {
    for (uint32_t t = millis(); millis() - t < 30;) {
      uint32_t seed = esp_random() | 1; int d = attempt(seed, P, tab, s0);
      if (d > best) { best = d; bestSeed = seed; }
      bool done = d >= target();
      if (!done && best > 0 && millis() - genStart > GEN_MS) { if (seed != bestSeed) attempt(bestSeed, P, tab, s0); done = true; }
      if (done) { s = s0; moves = 0; gen = false; return; }
    }
  }
  static void doMove(uint64_t v) {
    if (v == s) return;
    s = v; moves++; snd::click();
    if (pos(s, 0) == GOAL) { level++; winT = 1e-3f; snd::note(700); buzz(60, 40); }
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); return; }
    if (gen) { genStep(); return; }
    if (winT > 0) { if ((winT += c.dt) > 0.7f) newLevel(); return; }
    if (muted) { if ((autoT += c.dt) > 0.5f) { autoT = 0; doMove(hint(P, tab, s)); } return; }   // 選單預覽
    if ((left -= c.dt) <= 0) { over = true; rank = board.record(level); buzz(150, 150); snd::note(300); return; }
    if (c.tapA) { s = s0; moves = 0; grab = -1; }
    if (c.tapC) { left -= HINT_COST; grab = -1; doMove(hint(P, tab, s)); return; }
    if (grab < 0 && c.tap && c.tx >= BX && c.ty >= BY) {
      int col = (c.tx - BX) / CS, row = (c.ty - BY) / CS;
      if (col < N && row < N) for (int i = 0; i < P.nv; i++) if (cells(P.car[i], pos(s, i)) & (1ull << (row * N + col))) {
        int a, b; range(P, s, i, a, b); grab = i; lo = a - pos(s, i); hi = b - pos(s, i); t0 = P.car[i].horiz ? c.tx : c.ty; off = 0;
      }
    }
    if (grab >= 0) {
      if (c.touch) off = fminf(fmaxf(((P.car[grab].horiz ? c.tx : c.ty) - t0) / (float)CS, lo), hi);
      else { int i = grab, p = pos(s, i) + (int)lroundf(off); grab = -1; doMove(setPos(s, i, p)); }
    }
  }
  void draw() {
    uint32_t bg = rgb(25, 25, 35), lot = rgb(60, 60, 72), line = rgb(80, 80, 95);
    cv.fillScreen(bg);
    cv.fillRect(BX, BY, N * CS, N * CS, lot);
    for (int k = 1; k < N; k++) { cv.drawFastVLine(BX + k * CS, BY, N * CS, line); cv.drawFastHLine(BX, BY + k * CS, N * CS, line); }
    cv.drawRect(BX - 2, BY - 2, N * CS + 4, N * CS + 4, rgb(160, 160, 175)); cv.drawRect(BX - 3, BY - 3, N * CS + 6, N * CS + 6, rgb(160, 160, 175));
    cv.fillRect(BX + N * CS, BY + EXIT_ROW * CS + 2, 4, CS - 4, lot);   // 出口缺口
    cv.setTextDatum(middle_left); cv.setTextSize(2); cv.setTextColor(rgb(220, 60, 60), bg); cv.drawString(">", BX + N * CS + 6, BY + EXIT_ROW * CS + CS / 2);
    if (!gen) {
      cv.setClipRect(BX, BY, N * CS + 4, N * CS);   // 紅車出場時滑進出口就消失
      for (int i = 0; i < P.nv; i++) {
        const Car& k = P.car[i]; float p = pos(s, i) + (i == grab ? off : 0) + (i == 0 ? winT * 10 : 0);
        int x = BX + (int)((k.horiz ? p : k.fixed) * CS), y = BY + (int)((k.horiz ? k.fixed : p) * CS), w = (k.horiz ? k.len : 1) * CS, h = (k.horiz ? 1 : k.len) * CS;
        uint32_t col = i == 0 ? rgb(230, 30, 30) : hsv(0.1f + 0.75f * i / P.nv);
        cv.fillRoundRect(x + 3, y + 3, w - 6, h - 6, 6, col); cv.drawRoundRect(x + 3, y + 3, w - 6, h - 6, 6, i == grab ? rgb(255, 255, 255) : rgb(20, 20, 20));
      }
      cv.clearClipRect();
    } else { cv.setTextDatum(middle_center); cv.setTextColor(rgb(200, 200, 210), lot); cv.drawString("...", BX + N * CS / 2, BY + N * CS / 2); }
    char t[24]; cv.setTextDatum(top_left); cv.setTextColor(rgb(255, 230, 0), bg); snprintf(t, sizeof t, "LV %d", level + 1); cv.drawString(t, PX, 20);
    cv.setTextSize(1); cv.setTextColor(rgb(200, 200, 210), bg);
    snprintf(t, sizeof t, "time %d", left > 0 ? (int)left : 0); cv.drawString(t, PX, 50);
    snprintf(t, sizeof t, "moves %d", moves); cv.drawString(t, PX, 70);
    if (!gen) { snprintf(t, sizeof t, "par %d", tab.distOf(s0)); cv.drawString(t, PX, 84); }
    snprintf(t, sizeof t, "best %u", board.best()); cv.drawString(t, PX, 104);
    cv.setTextColor(rgb(120, 120, 140), bg); cv.drawString("A reset", PX, 190); cv.drawString("C hint -10s", PX, 204);
    if (over) { snprintf(t, sizeof t, "SCORE %d", level); board.draw(t, rank); }
  }
}
