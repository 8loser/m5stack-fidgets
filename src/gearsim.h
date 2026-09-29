// 齒輪的關卡產生與連動計算,不依賴 M5;g++ -O2 -Isrc test/gears_check.cpp -o /tmp/gc && /tmp/gc 在主機上核對
#pragma once
#include <cmath>
#include <cstdint>

namespace gearsim {
  constexpr int MAXG = 12, NS = 3; constexpr float K = 2, TOL = 3, GAP = 8, PI_F = 3.14159265f;   // 節圓半徑 = 齒數 x K;中心距差 TOL 內算咬合
  constexpr int GW = 320, GH = 240, TOP = 16;   // 場地;頂端留給 HUD
  static const int TEETH[NS] = { 7, 11, 16 };
  // g[0] 驅動輪、g[1] 目標輪、g[2..] 空位(n = 0 是空的)。ans:正解齒數(0 = 陷阱,該留空);trap:陷阱放這個齒數會跟兩個鄰居繞成三角形卡死
  struct Gear { float x, y; int n, ans, trap; float th, rho; bool hit; };   // rho:相對驅動輪的轉速比,0 = 沒接上
  static Gear g[MAXG]; static int ng;
  static uint32_t seed = 1;
  static float rnd() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return (seed >> 8) * (1.0f / 16777216.0f); }
  static inline float rad(int n) { return n * K; }

  // 驅動輪轉到 th0,沿咬合關係推出每顆的角度與轉速比。回傳是否卡死:接上的齒輪跟別顆疊在一起,或同一顆被推出兩種轉速(奇數環)
  static bool solve(float th0) {
    bool seen[MAXG] = {}, jam = false, loop = false; int q[MAXG], qh = 0, qt = 0;
    for (int i = 0; i < ng; i++) { g[i].rho = 0; g[i].hit = false; }
    for (int i = 0; i < ng; i++) for (int j = i + 1; j < ng; j++) if (g[i].n && g[j].n) {
      float dx = g[j].x - g[i].x, dy = g[j].y - g[i].y;
      if (sqrtf(dx * dx + dy * dy) < rad(g[i].n) + rad(g[j].n) - TOL) g[i].hit = g[j].hit = true;
    }
    g[0].th = th0; g[0].rho = 1; seen[0] = true; q[qt++] = 0;
    while (qh < qt) {
      int i = q[qh++]; if (g[i].hit) jam = true;
      for (int j = 0; j < ng; j++) {
        if (j == i || !g[j].n) continue;
        float dx = g[j].x - g[i].x, dy = g[j].y - g[i].y, d = sqrtf(dx * dx + dy * dy);
        if (fabsf(d - rad(g[i].n) - rad(g[j].n)) > TOL) continue;
        float rho = -g[i].rho * g[i].n / g[j].n;
        if (seen[j]) { if (fabsf(rho - g[j].rho) > 1e-4f * fabsf(rho)) loop = true; continue; }
        // 接觸點上 i 的齒對 j 的齒縫:θj = φ + π - π/nj + (φ - θi)·ni/nj
        float phi = atan2f(dy, dx);
        seen[j] = true; g[j].rho = rho; g[j].th = phi + PI_F - PI_F / g[j].n + (phi - g[i].th) * g[i].n / g[j].n; q[qt++] = j;
      }
    }
    if (loop) for (int i = 0; i < ng; i++) if (seen[i]) g[i].hit = true;
    return jam || loop;
  }

  static bool inField(float x, float y, float r) { return x > r + 2 && x < GW - r - 2 && y > TOP + r && y < GH - r - 2; }
  // 新齒輪 (x, y, r) 跟 g[0..cnt) 裡除了 a、b 之外的都至少隔 GAP
  static bool clear(float x, float y, float r, const Gear* p, int cnt, int a = -1, int b = -1) {
    for (int k = 0; k < cnt; k++) {
      if (k == a || k == b) continue;
      float dx = p[k].x - x, dy = p[k].y - y, lim = rad(p[k].n) + r + GAP;
      if (dx * dx + dy * dy < lim * lim) return false;
    }
    return true;
  }

  // 第 level 關(1 起算):驅動輪到目標輪之間一條 ns 格空位的齒輪串;第 3 關起加陷阱空位
  // ponytail: 拒絕取樣,失敗就少一格重來;場地小,5 格以上常排不下
  static void gen(int level) {
    int nd = level >= 6 ? 2 : level >= 3 ? 1 : 0;
    for (int ns = level / 2 + 2 > 5 ? 5 : level / 2 + 2; ; ns = ns > 1 ? ns - 1 : 1) for (int tries = 0; tries < 300; tries++) {
      Gear p[MAXG] = {}; int np = ns + 2;
      for (int k = 0; k < np; k++) p[k].n = TEETH[(int)(rnd() * NS)];
      float r0 = rad(p[0].n); p[0].x = r0 + 4 + rnd() * 20; p[0].y = TOP + r0 + rnd() * (GH - TOP - 2 * r0 - 4);
      bool ok = true;
      for (int k = 1; k < np && ok; k++) {
        ok = false;
        for (int t = 0; t < 40 && !ok; t++) {
          float a = (rnd() * 2 - 1) * 1.3f, d = rad(p[k - 1].n) + rad(p[k].n), r = rad(p[k].n);
          p[k].x = p[k - 1].x + cosf(a) * d; p[k].y = p[k - 1].y + sinf(a) * d;
          ok = inField(p[k].x, p[k].y, r) && clear(p[k].x, p[k].y, r, p, k - 1);
        }
      }
      if (!ok) continue;
      // 陷阱:跟串上相鄰兩顆同時咬合的位置(兩圓交點),放對齒數就成三角形
      int nt = 0; Gear tr[2] = {};
      for (int t = 0; t < 40 && nt < nd; t++) {
        int a = (int)(rnd() * (np - 1)), b = a + 1, n = TEETH[(int)(rnd() * NS)];
        float ra = rad(p[a].n) + rad(n), rb = rad(p[b].n) + rad(n), dx = p[b].x - p[a].x, dy = p[b].y - p[a].y, d = sqrtf(dx * dx + dy * dy);
        float m = (ra * ra - rb * rb + d * d) / (2 * d), h = sqrtf(fmaxf(ra * ra - m * m, 0)), s = rnd() < 0.5f ? -1.f : 1.f;
        float x = p[a].x + (dx * m - dy * h * s) / d, y = p[a].y + (dy * m + dx * h * s) / d;
        if (!inField(x, y, rad(n)) || !clear(x, y, rad(n), p, np, a, b) || !clear(x, y, rad(n), tr, nt)) continue;
        tr[nt].x = x; tr[nt].y = y; tr[nt].n = n; nt++;
      }
      if (nt < nd) continue;
      ng = 0;
      g[ng++] = p[0]; g[ng++] = p[np - 1];
      for (int k = 1; k < np - 1; k++) { g[ng] = p[k]; g[ng].ans = p[k].n; g[ng].n = 0; ng++; }
      for (int k = 0; k < nt; k++) { g[ng] = tr[k]; g[ng].trap = tr[k].n; g[ng].n = 0; ng++; }
      for (int i = 0; i < ng; i++) g[i].th = rnd() * 6.283f;
      return;
    }
  }
}
