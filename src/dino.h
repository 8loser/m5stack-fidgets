#pragma once
#include "common.h"

// ================= 21. 小恐龍 =================
// Chrome 斷線小恐龍:仙人掌與翼龍從右邊來,點螢幕或 A 跳、按住螢幕下半部或 C 蹲;越跑越快,每 300 分日夜切換。
// 傾斜往左壓可以稍微減速(往右加速)。撞到就結束,1.5 秒後重來;前 5 名存 NVS
namespace dino {
  constexpr int GY = 190, DX = 50, MAXO = 4; constexpr float G = 1400, JUMP = 520, V0 = 170, VMAX = 420;
  struct Ob { float x; int8_t kind; bool live; } static ob[MAXO];   // kind 0..2 仙人掌(小/大/雙)、3 翼龍高、4 翼龍低
  static float y, vy, speed, dist, spawnT, endT, anim; static int score, rank; static bool dead, night, duck; static Board board = { "dino" };
  static uint32_t fg, bg;
  void init() { memset(ob, 0, sizeof ob); y = 0; vy = 0; speed = V0; dist = 0; spawnT = 1.2f; endT = 0; anim = 0; score = 0; dead = night = duck = false; cv.fillScreen(0); }
  static void spawn() {
    for (auto& o : ob) if (!o.live) { float r = frand(); o = { (float)W + 20, (int8_t)(score > 150 && r < 0.25f ? 3 + (int)(frand() * 2) % 2 : (int)(r * 3) % 3), true }; return; }
  }
  // 障礙物在地面座標的包圍盒(x 相對 o.x,y 從地面往上算)
  static void box(const Ob& o, int& w, int& h, int& off) { switch (o.kind) { case 0: w = 10; h = 22; off = 0; break; case 1: w = 14; h = 32; off = 0; break; case 2: w = 26; h = 24; off = 0; break; case 3: w = 22; h = 12; off = 44; break; default: w = 22; h = 12; off = 18; } }
  void step(const Ctx& c) {
    if (dead) { if ((endT += c.dt) > 1.5f && (c.tap || c.tapA)) init(); return; }
    duck = (c.btnC || (c.touch && c.ty > H / 2)) && y <= 0;
    if ((c.tap || c.tapA) && y <= 0 && !duck) { vy = -JUMP; snd::note(600); buzz(30, 10); }
    vy += G * c.dt; y += vy * c.dt; if (y > 0) { y = 0; vy = 0; }
    float tilt = 1 + c.gx * 0.3f; if (tilt < 0.7f) tilt = 0.7f;
    speed = fminf(V0 + dist * 0.012f, VMAX);
    float v = speed * tilt; dist += v * c.dt; anim += v * c.dt;
    int s = (int)(dist / 12); if (s != score) { score = s; if (score % 100 == 0) snd::note(880); if (score % 300 == 0) { night = !night; buzz(60, 30); } }
    if ((spawnT -= c.dt) <= 0) { spawn(); spawnT = 0.9f + frand() * 1.2f - speed / VMAX * 0.4f; }
    int dw = duck ? 30 : 20, dh = duck ? 14 : 30, dy = (int)(GY + y);   // 恐龍包圍盒(左上 DX, dy-dh)
    for (auto& o : ob) if (o.live) {
      o.x -= v * c.dt; if (o.x < -40) { o.live = false; continue; }
      int w, h, off; box(o, w, h, off);
      bool hit = o.x < DX + dw - 3 && o.x + w > DX + 3 && GY - off - h < dy - 2 && GY - off > dy - dh + 2;
      if (hit) { dead = true; endT = 0; rank = board.record(score); snd::note(150); buzz(200, 200); }
    }
  }
  static void dinoSprite(int x, int y0, bool duckPose, int leg) {   // 用矩形拼的 T-Rex,右邊是頭
    if (duckPose) { cv.fillRect(x, y0 - 12, 26, 8, fg); cv.fillRect(x + 22, y0 - 16, 12, 8, fg); cv.fillRect(x + 30, y0 - 13, 3, 2, bg); cv.fillRect(x + 4 + leg * 8, y0 - 4, 4, 4, fg); cv.fillRect(x + 14 - leg * 8, y0 - 4, 4, 4, fg); return; }
    cv.fillRect(x + 12, y0 - 30, 14, 10, fg); cv.fillRect(x + 22, y0 - 27, 3, 2, bg);   // 頭、眼
    cv.fillRect(x + 12, y0 - 24, 12, 3, fg); cv.fillRect(x + 6, y0 - 22, 12, 14, fg); cv.fillRect(x, y0 - 18, 8, 6, fg);   // 嘴下緣、身、尾
    cv.fillRect(x + 16, y0 - 16, 4, 3, fg);   // 小手
    if (leg == 0) { cv.fillRect(x + 7, y0 - 8, 4, 8, fg); cv.fillRect(x + 14, y0 - 8, 4, 5, fg); } else { cv.fillRect(x + 7, y0 - 8, 4, 5, fg); cv.fillRect(x + 14, y0 - 8, 4, 8, fg); }
  }
  void draw() {
    fg = night ? rgb(240, 240, 240) : rgb(83, 83, 83); bg = night ? rgb(0, 0, 0) : rgb(247, 247, 247);
    cv.fillScreen(bg);
    cv.drawFastHLine(0, GY + 1, W, fg);
    for (int i = 0; i < 12; i++) { int px = (int)((i * 41 - (int)dist) % W + W) % W; cv.drawPixel(px, GY + 5 + (i * 7) % 6, fg); }   // 地面碎石
    for (int i = 0; i < 3; i++) { int cx = (int)((i * 130 - (int)(dist * 0.25f)) % (W + 60) + W + 60) % (W + 60) - 30, cy = 50 + i * 25; cv.fillRoundRect(cx, cy, 36, 10, 5, fg); cv.fillRoundRect(cx + 4, cy + 2, 28, 6, 3, bg); }   // 雲
    for (auto& o : ob) if (o.live) {
      int x = (int)o.x, w, h, off; box(o, w, h, off);
      if (o.kind <= 2) {   // 仙人掌:主幹 + 兩側短枝
        int n = o.kind == 2 ? 2 : 1; for (int k = 0; k < n; k++) { int bx = x + k * 14, bw = o.kind == 1 ? 8 : 6, bh = h; cv.fillRect(bx + 3, GY - bh, bw, bh, fg); cv.fillRect(bx, GY - bh + 8, 3, 8, fg); cv.fillRect(bx + 3 + bw, GY - bh + 5, 3, 8, fg); }
      } else {   // 翼龍:身體 + 拍動的翅膀
        int by = GY - off - 6, wing = ((int)(anim / 40) & 1) ? -8 : 6;
        cv.fillRect(x, by, 18, 5, fg); cv.fillRect(x + 14, by - 3, 8, 4, fg); cv.fillRect(x + 6, by + wing, 8, 6, fg);
      }
    }
    int leg = (y < 0) ? 0 : ((int)(anim / 25) & 1);
    dinoSprite(DX, (int)(GY + y), duck, leg);
    if (dead) { char s[20]; snprintf(s, sizeof s, "SCORE %d", score); board.draw(s, rank); }
    char t[24]; snprintf(t, sizeof t, "HI %05u  %05d", board.best(), score);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(fg, bg); cv.drawString(t, 4, 4);
  }
}
