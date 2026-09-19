#pragma once
#include "common.h"
#include "face.h"

// ================= Numberblocks 系列共用:畫「站直」的數字塔 =================
// N 就是 N 塊積木:10 以上先排「幾根 10 的柱」,零頭在最右邊一根;顏色照原作(1 紅 2 橘 3 黃 4 綠 5 淺藍 6 靛 7 紫 8 粉 9 灰 10 白),最上面一塊有臉
namespace numberlib {
  static const uint8_t COL[11][3] = { {0,0,0},{230,50,50},{240,140,40},{250,220,60},{70,190,80},{90,180,240},{80,70,200},{170,80,220},{240,120,190},{150,150,150},{245,245,245} };
  static uint32_t colOf(int n) { n = n % 10 ? n % 10 : 10; return rgb(COL[n][0], COL[n][1], COL[n][2]); }
  static int columns(int n) { return n / 10 + (n % 10 ? 1 : 0); }
  // 以 (cx, baseY) 為底的中心畫 N 的塔,bs 是每塊邊長
  static void drawTower(int cx, int baseY, int n, int bs, bool withFace = true, face::Mood mood = face::HAPPY) {
    if (n <= 0) return;
    int nc = columns(n), x0 = cx - nc * bs / 2;
    for (int c = 0; c < nc; c++) {
      int h = c < n / 10 ? 10 : n % 10; uint32_t col = c < n / 10 ? colOf(10) : colOf(n % 10), edge = rgb(30, 30, 40);
      for (int k = 0; k < h; k++) { int x = x0 + c * bs, y = baseY - (k + 1) * bs; cv.fillRect(x, y, bs, bs, col); cv.drawRect(x, y, bs, bs, edge); }
    }
    if (withFace) { int top = nc - 1, h = top < n / 10 ? 10 : n % 10; float fx = x0 + top * bs + bs / 2.0f, fy = baseY - h * bs + bs / 2.0f; face::draw(fx, fy, 1, 0, 0, 1, mood, bs / 14.0f); }
  }
  static int fitBs(int n, int maxW, int maxH) { int nc = columns(n), h = n < 10 ? n : 10; int bs = maxW / nc; if (bs > maxH / h) bs = maxH / h; return bs > 16 ? 16 : bs < 5 ? 5 : bs; }
  // 下排三個選項按鈕,回傳被點到的(0..2)或 -1;A 選左、C 選右、點中間選中
  static void drawOptions(const int* opt, int hi = -1, bool wrong = false) {
    for (int k = 0; k < 3; k++) {
      int x = 20 + k * 100; bool on = k == hi;
      cv.fillRoundRect(x, 200, 80, 34, 6, on ? (wrong ? rgb(150, 40, 40) : rgb(40, 130, 60)) : rgb(40, 40, 55)); cv.drawRoundRect(x, 200, 80, 34, 6, rgb(140, 140, 160));
      char s[8]; snprintf(s, sizeof s, "%d", opt[k]); cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(rgb(255, 255, 255), 0); cv.drawString(s, x + 40, 217);
    }
    cv.setTextSize(1); cv.setTextColor(rgb(120, 120, 140), 0); cv.drawString("A", 8, 217); cv.drawString("C", 312, 217);
  }
  static int pickOption(const Ctx& c) {
    if (c.tapA) return 0; if (c.tapC) return 2;
    if (c.tap && c.ty >= 196) { int k = (c.tx - 20) / 100; if (k >= 0 && k < 3 && c.tx >= 20 + k * 100 && c.tx < 100 + k * 100) return k; }
    return -1;
  }
  static void makeOptions(int answer, int* opt, int maxN) {   // 正解 + 兩個接近的錯誤選項,隨機排
    int a = answer, b, c2;
    do { b = a + (int)(frand() * 7) - 3; } while (b == a || b < 1 || b > maxN);
    do { c2 = a + (int)(frand() * 11) - 5; } while (c2 == a || c2 == b || c2 < 1 || c2 > maxN);
    int pos = (int)(frand() * 3) % 3; opt[pos] = a; opt[(pos + 1) % 3] = b; opt[(pos + 2) % 3] = c2;
  }
  static void drawLives(int lives) { for (int i = 0; i < 3; i++) cv.fillCircle(W - 12 - i * 12, 8, 4, i < lives ? rgb(240, 60, 80) : rgb(60, 60, 70)); }
}
