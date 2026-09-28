#pragma once
#include "ringlib.h"

// ================= 三色吞食 =================
// 紅綠藍三色球在整個螢幕裡彈來彈去。兩顆不同色的撞到,一起變成第三色;
// 一顆同時碰到 2 顆以上不同色的,就把它們吞掉變大,變成參與者裡最多的那色(平手維持自己的)。
// 全部變成同一色就結束重來。傾斜給重力,搖一下全部噴開(同遊戲 1),點螢幕把附近的彈飛
namespace split {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int N = 40; constexpr float G = 900, R0 = 8, REST = 0.9f, KICK = 700, CD = 0.4f, HIT = 30;   // CD:變色/吞食後的冷卻,免得貼著連鎖;HIT:接近速度超過才算撞到變色
  static const uint8_t PAL[3][3] = { {255,60,60},{60,220,90},{60,120,255} };
  static const char* NAME[3] = { "RED", "GREEN", "BLUE" };
  struct C { float x, y, vx, vy, r, cd; int8_t k; bool live; } static cells[N];
  static int cnt[3], winner; static bool over; static float endT;
  static uint32_t col(int k) { return rgb(PAL[k][0], PAL[k][1], PAL[k][2]); }
  static bool touch(const C& p, const C& q) { float dx = q.x - p.x, dy = q.y - p.y, rr = p.r + q.r; return dx * dx + dy * dy < rr * rr; }
  void init() {
    memset(cells, 0, sizeof cells); over = false; endT = 0; ringlib::reset(); cv.fillScreen(0);
    for (int i = 0; i < N; i++) cells[i] = { R0 + frand() * (W - 2 * R0), R0 + frand() * (H - 2 * R0), (frand() - 0.5f) * 240, (frand() - 0.5f) * 240, R0, 1.0f, (int8_t)(i % 3), true };   // 開局冷卻 1 秒,等重疊推開
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); stepSparks(c.dt); return; }
    if (c.shake > SHAKE) { for (auto& o : cells) if (o.live) shakeKick(c, o.vx, o.vy); buzz(80, 30); }   // 搖一下:全部往隨機方向噴開
    for (auto& o : cells) if (o.live) {
      if (c.tap) tapPush(c, o.x, o.y, o.vx, o.vy, o.r, KICK);
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt; o.vx *= 0.999f; o.vy *= 0.999f;
      o.x += o.vx * c.dt; o.y += o.vy * c.dt; o.cd -= c.dt;
      wallScreen(o.x, o.y, o.vx, o.vy, o.r, REST);
    }
    for (int i = 0; i < N; i++) {   // 吞食:同時碰到 2 顆以上不同色的
      auto& p = cells[i]; if (!p.live || p.cd > 0) continue;
      int hit[N], n = 0;
      for (int j = 0; j < N; j++) if (j != i && cells[j].live && cells[j].k != p.k && touch(p, cells[j])) hit[n++] = j;
      if (n < 2) continue;
      int votes[3] = {}; float area = p.r * p.r; votes[p.k]++;
      for (int m = 0; m < n; m++) { auto& q = cells[hit[m]]; votes[q.k]++; area += q.r * q.r; q.live = false; spark(q.x, q.y, col(q.k)); }
      int best = p.k; for (int k = 0; k < 3; k++) if (votes[k] > votes[best]) best = k;
      p.k = best; p.r = sqrtf(area); p.cd = CD; snd::note(200 + n * 60); buzz(80, 40);
    }
    for (int i = 0; i < N; i++) if (cells[i].live) for (int j = i + 1; j < N; j++) if (cells[j].live) {   // ponytail: O(n^2),n<=40
      auto& p = cells[i]; auto& q = cells[j];
      float dx = q.x - p.x, dy = q.y - p.y, d = sqrtf(dx * dx + dy * dy), rr = p.r + q.r;
      if (d >= rr || d < 1e-3f) continue;
      float nx = dx / d, ny = dy / d, pen = rr - d, mp = q.r * q.r / (p.r * p.r + q.r * q.r), mq = 1 - mp;   // 依面積分配,大的少動
      p.x -= nx * pen * mp; p.y -= ny * pen * mp; q.x += nx * pen * mq; q.y += ny * pen * mq;
      float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny;
      if (rv >= 0) continue;
      float jn = -(1 + REST) * rv; p.vx -= nx * jn * mp; p.vy -= ny * jn * mp; q.vx += nx * jn * mq; q.vy += ny * jn * mq;
      if (p.k != q.k && rv < -HIT && p.cd <= 0 && q.cd <= 0) { p.k = q.k = 3 - p.k - q.k; p.cd = q.cd = CD; snd::note(300 + p.k * 120); }   // 三色互換:變成第三色
    }
    cnt[0] = cnt[1] = cnt[2] = 0; for (auto& o : cells) if (o.live) cnt[o.k]++;
    for (int k = 0; k < 3; k++) if (cnt[k] && cnt[k] == cnt[0] + cnt[1] + cnt[2]) {
      over = true; winner = k; buzz(200, 150);
      for (auto& o : cells) if (o.live) spark(o.x, o.y, col(k));
    }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    for (auto& o : cells) if (o.live) cv.fillCircle((int)o.x, (int)o.y, (int)o.r, col(o.k));
    int tot = cnt[0] + cnt[1] + cnt[2]; if (!tot) tot = 1;   // 底部三色比例條
    for (int k = 0, x = 0; k < 3; k++) { int w = k == 2 ? W - x : W * cnt[k] / tot; cv.fillRect(x, H - 4, w, 4, col(k)); x += w; }
    char s[24]; snprintf(s, sizeof s, "%d : %d : %d", cnt[0], cnt[1], cnt[2]);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
    if (over) { cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(col(winner), 0); cv.drawString(NAME[winner], W / 2, H / 2); }
    drawSparks();
  }
}
