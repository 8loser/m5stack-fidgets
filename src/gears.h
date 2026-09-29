#pragma once
#include "gearsim.h"

// ================= 齒輪 =================
// 左邊藍色驅動輪(白點是把手)、另一頭金色目標輪,中間的「+」空位點一下依序換成 小 / 中 / 大 / 空。
// 咬合的齒輪反向連動、轉速照齒數比(小的轉得快);目標輪每過一齒彈一個音,轉越快旋律越快。
// 手指在藍輪上畫圈轉它、放手靠慣性;A / C 是往左 / 往右的馬達;搖一下給一股衝力。
// 目標輪轉滿兩圈(外圈進度填滿)過關換新場景。齒輪疊在一起、或三顆繞成一圈互相咬住,整串卡死變紅:第 3 關起有空位是陷阱,要留空
namespace gears {
  using namespace gearsim;
  constexpr float GOAL = 4 * PI_F;   // 目標輪要轉的弧度
  constexpr int NC = 40;
  static float th0, om, prog, winT, shakeT, lastA; static int level, tickD, tickT, mel; static bool drag, jam;
  static struct { float x, y, vx, vy; uint32_t c; } conf[NC];
  static const float MEL[] = { 261.63f, 329.63f, 392.00f, 440.00f, 392.00f, 329.63f, 293.66f, 261.63f, 293.66f, 329.63f, 392.00f, 523.25f };

  static void next() { gearsim::seed = (uint32_t)(frand() * 4e9f) | 1; gen(level); th0 = 0; prog = 0; winT = 0; drag = false; solve(0); }
  void init() { level = 1; om = 3; next(); }

  static void bump() { if (shakeT <= 0) { buzz(150, 60); snd::click(); } shakeT = 0.3f; }
  static int toothAt(float th, int n) { return (int)floorf(th * n / (2 * PI_F)); }

  void step(const Ctx& c) {
    Gear& d = g[0];
    shakeT -= c.dt;
    if (winT > 0) {
      for (auto& p : conf) { p.vy += 300 * c.dt; p.x += p.vx * c.dt; p.y += p.vy * c.dt; }
      if ((winT -= c.dt) <= 0) { level++; next(); return; }
    } else if (c.tap) {
      int best = -1; float bd = 1e9f;   // 點空位優先,空位緊貼驅動輪時也點得到
      for (int i = 2; i < ng; i++) {
        float dx = c.tx - g[i].x, dy = c.ty - g[i].y, dd = dx * dx + dy * dy, lim = fmaxf(22, rad(g[i].n));
        if (dd < lim * lim && dd < bd) { bd = dd; best = i; }
      }
      float dx = c.tx - d.x, dy = c.ty - d.y, lim = rad(d.n) + 18;
      if (best >= 0) { int& n = g[best].n; n = n == 0 ? TEETH[0] : n == TEETH[0] ? TEETH[1] : n == TEETH[1] ? TEETH[2] : 0; snd::click(); }
      else if (dx * dx + dy * dy < lim * lim) { drag = true; lastA = atan2f(dy, dx); }
    }
    if (!c.touch) drag = false;

    float dth;
    if (winT > 0) { om = 8; dth = om * c.dt; }
    else if (drag) {
      float a = atan2f(c.ty - d.y, c.tx - d.x); dth = a - lastA; lastA = a;
      if (dth > PI_F) dth -= 2 * PI_F; if (dth < -PI_F) dth += 2 * PI_F;
      om = om * 0.6f + 0.4f * dth / fmaxf(c.dt, 1e-3f);
    } else {
      if (c.btnA || c.btnC) om += ((c.btnC - c.btnA) * 6 - om) * fminf(1, 3 * c.dt); else om *= expf(-0.4f * c.dt);
      if (c.shake > SHAKE) om += (frand() < 0.5f ? -1 : 1) * (4 + c.shake * 3);
      om = fmaxf(-14, fminf(14, om)); dth = om * c.dt;
    }

    jam = solve(th0);
    if (jam) { if (fabsf(dth) > 0.01f) bump(); om = 0; return; }
    th0 += dth; solve(th0);
    int td = toothAt(th0, d.n); if (td != tickD) { tickD = td; snd::click(); }
    if (g[1].rho != 0) {
      int tt = toothAt(g[1].th, g[1].n); if (tt != tickT) { tickT = tt; snd::note(MEL[mel++ % (sizeof MEL / sizeof MEL[0])]); }
      if (winT <= 0 && (prog += fabsf(g[1].rho * dth)) >= GOAL) {
        winT = 2.5f; buzz(200, 200); snd::note(523.25f);
        for (auto& p : conf) { float a = frand() * 6.283f, v = 80 + frand() * 220; p = { g[1].x, g[1].y, cosf(a) * v, sinf(a) * v - 150, hsv(frand()) }; }
      }
    }
  }

  static void drawGear(const Gear& q, uint32_t col, uint32_t dark) {
    float r = rad(q.n), x = q.x, y = q.y;
    if (q.hit && shakeT > 0) { x += frand() * 4 - 2; y += frand() * 4 - 2; }
    for (int k = 0; k < q.n; k++) {
      float a = q.th + k * 2 * PI_F / q.n, ca = cosf(a), sa = sinf(a);
      cv.drawWideLine((int)(x + ca * (r - 3)), (int)(y + sa * (r - 3)), (int)(x + ca * (r + 2)), (int)(y + sa * (r + 2)), 2.2f, col);
    }
    cv.fillCircle((int)x, (int)y, (int)r - 2, col);
    cv.fillCircle((int)x, (int)y, (int)(r * 0.5f), dark);
    cv.drawWideLine((int)x, (int)y, (int)(x + cosf(q.th) * r * 0.75f), (int)(y + sinf(q.th) * r * 0.75f), 2, dark);   // 小齒輪也看得出在轉
  }

  void draw() {
    uint32_t bg = rgb(18, 18, 30); cv.fillScreen(bg);
    for (int i = 2; i < ng; i++) if (!g[i].n) {
      uint32_t c = rgb(110, 110, 130); int x = (int)g[i].x, y = (int)g[i].y;
      cv.drawCircle(x, y, 11, c); cv.drawFastHLine(x - 5, y, 11, c); cv.drawFastVLine(x, y - 5, 11, c);
    }
    uint32_t red = rgb(255, 60, 60), dred = rgb(120, 20, 20);
    for (int i = ng - 1; i >= 0; i--) {
      const Gear& q = g[i]; if (!q.n) continue;
      uint32_t col, dark;
      if (q.hit) { col = red; dark = dred; }
      else if (i == 0) { col = rgb(60, 140, 255); dark = rgb(20, 50, 110); }
      else if (i == 1) { col = rgb(255, 200, 40); dark = rgb(130, 90, 0); }
      else if (q.n == TEETH[0]) { col = rgb(80, 220, 120); dark = rgb(20, 90, 40); }
      else if (q.n == TEETH[1]) { col = rgb(255, 140, 50); dark = rgb(120, 50, 10); }
      else { col = rgb(190, 100, 255); dark = rgb(80, 30, 120); }
      drawGear(q, col, dark);
    }
    const Gear& d = g[0]; float r0 = rad(d.n) * 0.6f;
    cv.fillCircle((int)(d.x + cosf(th0) * r0), (int)(d.y + sinf(th0) * r0), 5, rgb(255, 255, 255));   // 把手
    const Gear& t = g[1]; int rt = (int)rad(t.n);
    cv.drawCircle((int)t.x, (int)t.y, rt + 7, rgb(60, 60, 70));
    if (prog > 0) cv.fillArc((int)t.x, (int)t.y, rt + 6, rt + 8, -90, -90 + 360 * fminf(prog / GOAL, 1), rgb(255, 255, 255));
    cv.setTextSize(1); cv.setTextDatum(top_left); cv.setTextColor(rgb(200, 200, 200), bg);
    char s[12]; snprintf(s, sizeof s, "Lv %d", level); cv.drawString(s, 2, 2);
    if (winT > 0) {
      for (auto& p : conf) cv.fillRect((int)p.x, (int)p.y, 4, 4, p.c);
      cv.setTextDatum(middle_center); cv.setTextSize(3); cv.setTextColor(rgb(255, 230, 0)); cv.drawString("GREAT!", W / 2, H / 2);
    } else if (level == 1) {
      cv.setTextDatum(bottom_center); cv.setTextColor(rgb(120, 120, 140), bg); cv.drawString("tap + to add gears, spin the blue one", W / 2, H - 2);
    }
  }
}
