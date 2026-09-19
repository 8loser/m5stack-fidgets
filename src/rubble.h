#pragma once
#include "ringlib.h"

// ================= 15. 碎石 =================
// 圓形場地裡的球每撞一次牆就長大;邊上一根尖刺,球碰到就碎成一堆碎石落到底部堆起來,尖刺也跟著變長。
// 傾斜給重力(落下中的碎石也受影響,停住的不再動),點螢幕在手指處丟一顆新球,A/C 轉尖刺。碎石堆滿就重來
namespace rubble {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 120, RAD = 106, MAXB = 6, MAXP = 260; constexpr float G = 300, R0 = 5, RMAX = 40, GROWF = 1.06f, L0 = 12, LSTEP = 4, LMAX = 70;
  struct B { float x, y, vx, vy, r; bool live; } static b[MAXB];
  struct P { float x, y, vx, vy; bool live, rest; } static rub[MAXP];
  static float spikeA, spikeL, endT; static int shatters, nrub; static bool over;
  static void spawn(float x, float y) { for (auto& o : b) if (!o.live) { float a = frand() * 6.283f; o = { x, y, cosf(a) * 150, sinf(a) * 150, R0, true }; return; } }
  static void spikeTip(float& tx, float& ty) { tx = CX + (RAD - spikeL) * cosf(spikeA); ty = CY + (RAD - spikeL) * sinf(spikeA); }
  void init() { memset(b, 0, sizeof b); memset(rub, 0, sizeof rub); spikeA = 1.2f; spikeL = L0; shatters = nrub = 0; over = false; endT = 0; ringlib::reset(); spawn(CX, CY); cv.fillScreen(0); }
  static void shatter(B& o) {
    o.live = false; shatters++; spikeL = fminf(spikeL + LSTEP, LMAX); spark(o.x, o.y, rgb(255, 255, 255)); snd::note(200 + o.r * 6); buzz(80 + (int)o.r * 3, 40);
    int n = (int)(o.r * 1.5f); for (auto& p : rub) if (!p.live && !p.rest && n-- > 0) { float a = frand() * 6.283f, v = 40 + frand() * 120; p = { o.x + (frand() - 0.5f) * o.r, o.y + (frand() - 0.5f) * o.r, cosf(a) * v, sinf(a) * v, true, false }; nrub++; }
    spawn(CX + (frand() - 0.5f) * 40, CY + (frand() - 0.5f) * 40);
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 2.5f) init(); stepSparks(c.dt); return; }
    spikeA += (0.15f + (c.btnC - c.btnA) * 1.5f) * c.dt;   // A/C 轉尖刺
    if (c.tap) { float dx = c.tx - CX, dy = c.ty - CY; if (dx * dx + dy * dy < (RAD - 20) * (RAD - 20)) spawn(c.tx, c.ty); }
    float tx, ty; spikeTip(tx, ty);
    for (auto& o : b) if (o.live) {
      o.vx += c.gx * G * c.dt; o.vy += c.gy * G * c.dt;
      float v = sqrtf(o.vx * o.vx + o.vy * o.vy); if (v < 90 && v > 1e-3f) { o.vx *= 90 / v; o.vy *= 90 / v; }
      o.x += o.vx * c.dt; o.y += o.vy * c.dt;
      if (wallCircle(o.x, o.y, o.vx, o.vy, o.r, CX, CY, RAD) > 0) { if (o.r < RMAX) o.r *= GROWF; snd::click(); }
      float dx = o.x - tx, dy = o.y - ty; if (dx * dx + dy * dy < o.r * o.r) shatter(o);   // 碰到尖刺尖端就碎
    }
    for (auto& p : rub) if (p.live) {   // 碎石:碰到牆或已停的碎石就停住
      p.vx += c.gx * G * c.dt; p.vy += c.gy * G * c.dt; p.x += p.vx * c.dt; p.y += p.vy * c.dt;
      float dx = p.x - CX, dy = p.y - CY, d = sqrtf(dx * dx + dy * dy);
      if (d > RAD - 2) { p.x = CX + dx / d * (RAD - 2); p.y = CY + dy / d * (RAD - 2); p.live = false; p.rest = true; continue; }
      for (auto& q : rub) if (q.rest) { float ex = p.x - q.x, ey = p.y - q.y; if (ex * ex + ey * ey < 6) { p.live = false; p.rest = true; break; } }   // ponytail: O(n^2) 只查 live 對 rest
    }
    if (nrub >= MAXP - 5) { over = true; buzz(200, 150); }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    cv.drawCircle(CX, CY, RAD, rgb(255, 150, 30));
    float tx, ty; spikeTip(tx, ty);
    cv.fillTriangle((int)tx, (int)ty, (int)(CX + (RAD + 2) * cosf(spikeA - 0.09f)), (int)(CY + (RAD + 2) * sinf(spikeA - 0.09f)), (int)(CX + (RAD + 2) * cosf(spikeA + 0.09f)), (int)(CY + (RAD + 2) * sinf(spikeA + 0.09f)), rgb(255, 255, 255));
    for (auto& p : rub) if (p.live || p.rest) cv.drawPixel((int)p.x, (int)p.y, rgb(200, 200, 200));
    for (auto& o : b) if (o.live) { cv.fillCircle((int)o.x, (int)o.y, (int)o.r, rgb(150, 150, 140)); cv.drawCircle((int)o.x, (int)o.y, (int)o.r, rgb(255, 60, 120)); }
    drawSparks();
    char s[24]; snprintf(s, sizeof s, "rubble %d  spike %d", nrub, (int)spikeL);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
  }
}
