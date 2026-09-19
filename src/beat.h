#pragma once
#include "common.h"

// ================= 14. 踩拍子 =================
// 球在一排琴鍵上一格一格跳,每次落地彈一個音;下一個音越長,跳得越高越久。
// 音是隨機的五聲音階旋律。往右傾拍子變快、往左變慢;點螢幕讓下一個音變成長音(跳最高)
namespace beat {
  constexpr int NK = 10, KW = 30, X0 = 10, BASE = 200; constexpr float G = 700;
  static const float DUR[3] = { 0.28f, 0.5f, 0.8f };
  static float t, T, x0, x1, y0, y1, h, glow[NK]; static int key, note[NK], lands; static bool longNext;
  static float freqOf(int n) { return 262 * powf(2, n / 5.0f * 0.7f); }   // 粗略把 0..9 映到約兩個八度,note() 會再吸到五聲音階
  static float keyX(int k) { return X0 + k * KW + KW / 2; }
  static void nextHop() {
    int d = longNext ? 2 : (int)(frand() * 3) % 3; longNext = false; T = DUR[d];
    x0 = keyX(key); y0 = 20 + note[key] * 3; key = (key + 1) % NK; note[key] = (int)(frand() * 10) % 10; x1 = keyX(key); y1 = 20 + note[key] * 3;
    h = G * T * T / 8; t = 0;   // 拋物線:高度由停空時間決定
  }
  void init() { for (int k = 0; k < NK; k++) { note[k] = (int)(frand() * 10) % 10; glow[k] = 0; } key = 0; lands = 0; longNext = false; nextHop(); t = 0; cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (c.tap) longNext = true;
    float tempo = 1 + c.gx * 0.6f; if (tempo < 0.4f) tempo = 0.4f;   // 傾斜調速
    t += c.dt * tempo;
    if (t >= T) {   // 落地
      lands++; glow[key] = 1; snd::note(freqOf(note[key])); buzz(30 + (int)(T * 60), 15);
      nextHop();
    }
    for (auto& g : glow) g -= c.dt * 2;
  }
  void draw() {
    fadeCanvas();
    for (int k = 0; k < NK; k++) {
      int hh = 20 + note[k] * 3; uint32_t cc = hsv(note[k] / 10.0f);
      cv.fillRect(X0 + k * KW + 2, BASE - hh, KW - 4, hh, glow[k] > 0 ? cc : rgb(40, 40, 60));
      if (glow[k] > 0) cv.fillRect(X0 + k * KW + 2, BASE - hh - (int)(glow[k] * 8), KW - 4, 2, cc);
    }
    float u = t / T, bx = x0 + (x1 - x0) * u, by = BASE - (y0 + (y1 - y0) * u) - h * 4 * u * (1 - u);   // 對稱拋物線,頂點在中間
    cv.fillCircle((int)bx, (int)by - 5, 5, rgb(255, 120, 200));
    cv.fillRect(0, 0, 150, 12, 0);
    char s[24]; snprintf(s, sizeof s, "landed %d  %s", lands, T > 0.6f ? "long" : T > 0.4f ? "mid" : "short");
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
