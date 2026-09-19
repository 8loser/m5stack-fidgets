#pragma once
#include "ringlib.h"

// ================= 3. 色層擴張競賽(撞到哪色,那色就往外長一層;先填滿的贏,環炸開)=================
namespace expand {
  using namespace ringlib;
  static int cnt[SEG]; static bool over; static float endT;
  static float wallOf(int) { return over ? -1 : R0; }
  void init() { memset(cnt, 0, sizeof cnt); over = false; endT = 0; reset(); spawnBall(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (!over) rot += 25 * c.dt;   // 環持續旋轉
    int s = ballStep(c, wallOf);
    if (s >= 0 && cnt[s] < LAYERS - 1 && ++cnt[s] == LAYERS - 1) {   // 贏了:全部脫落
      over = true; buzz(200, 120);
      for (int k = 0; k < SEG; k++) for (int l = 0; l <= cnt[k]; l++) detach(l, k);
    }
    stepPieces(c.dt); stepSparks(c.dt);
    if (over && (endT += c.dt) > 4) init();
  }
  void draw() {
    cv.fillScreen(0);
    if (!over) for (int s = 0; s < SEG; s++) for (int l = 0; l < LAYERS; l++) ringArc(l, s, l <= cnt[s] ? segCol(s) : rgb(45, 45, 45));
    drawPieces(); drawSparks(); drawBall();
    for (int s = 0; s < SEG; s++) {   // 右側圖例:各色長了幾層
      cv.fillCircle(232, 45 + s * 24, 4, segCol(s));
      for (int k = 0; k < LAYERS - 1; k++) cv.fillRect(242 + k * 8, 43 + s * 24, 5, 4, k < cnt[s] ? segCol(s) : rgb(60, 60, 60));
    }
  }
}
