#pragma once
#include "ringlib.h"

// ================= 合成圓 =================
// 螢幕裡不斷從頂端隨機位置掉下多邊形,兩個相同的碰到就合成邊數多一級的(三角→四角→五角→六角→圓),
// 兩個圓再合成更大的三角形,一直循環下去,每級都比上一級大。
// 傾斜給重力,搖一下全部噴開,點螢幕把附近的彈飛。10 秒內沒有任何合成就結束,比撐了幾秒,前 5 名排行
namespace merge {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = W / 2, CY = H / 2, MAXB = 40, LV = 5; constexpr float G = 320, REST = 0.4f, IDLE = 10, KICK = 900;   // LV:一輪幾種形狀,最後一種是圓
  static const uint8_t PAL[LV][3] = { {255,200,0},{0,220,120},{60,140,255},{255,60,160},{255,80,40} };
  struct B { float x, y, vx, vy, rot; int8_t lv; bool live; } static b[MAXB];
  static int merges, rank; static bool over; static float endT, dropT, idleT, t;
  static Board board = { "merge" };
  static float rad(int lv) { return 7 * powf(1.3f, lv); }   // 每級大 1.3 倍,圓(lv 4)之後的三角(lv 5)接著變大
  static uint32_t col(int lv) { lv %= LV; return rgb(PAL[lv][0], PAL[lv][1], PAL[lv][2]); }
  static void spawn(float x, float y, int lv) {
    for (auto& o : b) if (!o.live) { o = { x, y, (frand() - 0.5f) * 40, 0, frand() * 6.283f, (int8_t)lv, true }; return; }
  }
  void init() { memset(b, 0, sizeof b); merges = 0; rank = -1; over = false; endT = dropT = idleT = t = 0; ringlib::reset(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (over && (endT += c.dt) > 1.5f && c.tap) { init(); return; }
    if (!over) { t += c.dt; idleT += c.dt; }
    if (!over && (dropT += c.dt) > 1.0f) { dropT = 0; spawn(rad(1) + frand() * (W - 2 * rad(1)), rad(1), frand() < 0.7f ? 0 : 1); }
    if (!over && c.shake > SHAKE) { for (auto& o : b) if (o.live) shakeKick(c, o.vx, o.vy); buzz(80, 30); }
    if (!over && c.tap) for (auto& o : b) if (o.live) tapPush(c, o.x, o.y, o.vx, o.vy, rad(o.lv), KICK);
    for (auto& o : b) if (o.live) {
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt; o.vx *= 0.995f; o.vy *= 0.995f;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt; o.rot += o.vx * 0.01f * c.dt;
      if (over) continue;
      if (wallScreen(o.x, o.y, o.vx, o.vy, rad(o.lv), REST) > 120) snd::click();
    }
    if (!over) for (int i = 0; i < MAXB; i++) if (b[i].live) for (int j = i + 1; j < MAXB; j++) if (b[j].live) {   // ponytail: O(n^2),n<=40
      auto& p = b[i]; auto& q = b[j];
      float dx = q.x - p.x, dy = q.y - p.y, d = sqrtf(dx * dx + dy * dy), rr = rad(p.lv) + rad(q.lv);
      if (d >= rr || d < 1e-3f) continue;
      if (p.lv == q.lv) {   // 合成
        p.lv++; idleT = 0; p.x = (p.x + q.x) / 2; p.y = (p.y + q.y) / 2; p.vx = (p.vx + q.vx) / 2; p.vy = (p.vy + q.vy) / 2; q.live = false;
        merges++; spark(p.x, p.y, col(p.lv)); snd::note(250 + p.lv % LV * 80 + p.lv / LV * 40); buzz(40 + p.lv * 10, 20);
        continue;
      }
      float nx = dx / d, ny = dy / d, pen = rr - d, mp = rad(q.lv) / rr, mq = rad(p.lv) / rr;   // 大的少動
      p.x -= nx * pen * mp; p.y -= ny * pen * mp; q.x += nx * pen * mq; q.y += ny * pen * mq;
      float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny;
      if (rv < 0) { float jn = -(1 + REST) * rv; p.vx -= nx * jn * mp; p.vy -= ny * jn * mp; q.vx += nx * jn * mq; q.vy += ny * jn * mq; }
    }
    if (!over && idleT > IDLE) { over = true; rank = board.record((int)t); buzz(200, 150); for (auto& o : b) if (o.live) { float ex = o.x - CX, ey = o.y - CY, e = sqrtf(ex * ex + ey * ey) + 1; o.vx = ex / e * 300; o.vy = ey / e * 300; } }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    for (auto& o : b) if (o.live) {
      float r = rad(o.lv); uint32_t cc = col(o.lv);
      if (o.lv % LV == LV - 1) { cv.fillCircle((int)o.x, (int)o.y, (int)r, cc); continue; }
      int n = o.lv % LV + 3, px[6], py[6];
      for (int k = 0; k < n; k++) { float a = o.rot + k * 6.2831853f / n; px[k] = (int)(o.x + r * cosf(a)); py[k] = (int)(o.y + r * sinf(a)); }
      for (int k = 1; k + 1 < n; k++) cv.fillTriangle(px[0], py[0], px[k], py[k], px[k + 1], py[k + 1], cc);
    }
    drawSparks();
    char s[32]; snprintf(s, sizeof s, "time %ds  merges %d  idle %d", (int)t, merges, over ? 0 : (int)ceilf(IDLE - idleT));
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
    if (over) { snprintf(s, sizeof s, "TIME %ds", (int)t); board.draw(s, rank); }
  }
}
