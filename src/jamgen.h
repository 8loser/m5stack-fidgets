// 塞車(Rush Hour)的關卡產生器與解題表。不依賴 M5,主機上也能編:g++ -O2 -Isrc test/jamgen_check.cpp
#pragma once
#include <cstdint>

namespace jamgen {
  constexpr int N = 6, MAXV = 16, EXIT_ROW = 2, GOAL = N - 2;   // 紅車是 0 號,橫放在第 2 列,pos 到 4 就到出口
  struct Car { uint8_t horiz, len, fixed; };   // fixed:橫車在哪列 / 直車在哪行;能動的那個座標存在 state 裡
  struct Puzzle { int nv; Car car[MAXV]; };
  // state:第 i 台車的 pos 放在第 3i 位起的 3 bit
  static inline int pos(uint64_t s, int i) { return (s >> (3 * i)) & 7; }
  static inline uint64_t setPos(uint64_t s, int i, int p) { return (s & ~(7ull << (3 * i))) | ((uint64_t)p << (3 * i)); }
  static inline uint64_t cells(const Car& c, int p) { uint64_t m = 0; for (int k = 0; k < c.len; k++) m |= 1ull << (c.horiz ? c.fixed * N + p + k : (p + k) * N + c.fixed); return m; }
  static uint64_t occ(const Puzzle& P, uint64_t s) { uint64_t m = 0; for (int i = 0; i < P.nv; i++) m |= cells(P.car[i], pos(s, i)); return m; }
  // 列舉所有一步可達的盤面:一步 = 一台車沿自己的方向滑任意格。f(車號, 新 pos)
  template <class F> static void forMoves(const Puzzle& P, uint64_t s, F f) {
    uint64_t all = occ(P, s);
    for (int i = 0; i < P.nv; i++) {
      const Car& c = P.car[i]; int p = pos(s, i); uint64_t m = all & ~cells(c, p);
      for (int q = p - 1; q >= 0 && !(cells(c, q) & m); q--) f(i, q);
      for (int q = p + 1; q + c.len <= N && !(cells(c, q) & m); q++) f(i, q);
    }
  }
  static void range(const Puzzle& P, uint64_t s, int i, int& lo, int& hi) { lo = hi = pos(s, i); forMoves(P, s, [&](int j, int q) { if (j == i) { if (q < lo) lo = q; if (q > hi) hi = q; } }); }

  // 一個連通塊裡所有盤面與各自離出口的最少步數。記憶體由呼叫端給(ESP32 上放 PSRAM);cap 要是 2 的冪
  struct Table {
    int cap, n; uint64_t* st; uint8_t* dist; int32_t* hs; int32_t* q;   // hs:開放定址雜湊,大小 2*cap,存 st 的索引,-1 是空
    static uint32_t hash(uint64_t s) { return (uint32_t)((s * 0x9E3779B97F4A7C15ull) >> 32); }
    void clear() { n = 0; for (int i = 0; i < 2 * cap; i++) hs[i] = -1; }
    int find(uint64_t s) const { for (uint32_t h = hash(s) & (2 * cap - 1); hs[h] >= 0; h = (h + 1) & (2 * cap - 1)) if (st[hs[h]] == s) return hs[h]; return -1; }
    int add(uint64_t s) {   // 1 新加入、0 已有、-1 滿了
      uint32_t h = hash(s) & (2 * cap - 1);
      for (; hs[h] >= 0; h = (h + 1) & (2 * cap - 1)) if (st[hs[h]] == s) return 0;
      if (n >= cap) return -1;
      st[n] = s; hs[h] = n++; return 1;
    }
    int distOf(uint64_t s) const { int k = find(s); return k < 0 ? -1 : dist[k]; }
  };

  // 用 seed 隨機擺一盤,列舉它能走到的所有盤面,再從所有「紅車在出口」的盤面反向 BFS 算每個盤面的最少步數。
  // 回傳最遠那個盤面的步數並放進 start;整個連通塊留在 T 裡,之後查步數、給提示用。盤面數超過 cap 或無解回 -1
  static int attempt(uint32_t seed, Puzzle& P, Table& T, uint64_t& start) {
    auto rnd = [&](int n) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return (int)(seed % n); };
    P.nv = 1; P.car[0] = { 1, 2, EXIT_ROW };
    uint64_t s = setPos(0, 0, rnd(GOAL)), m = cells(P.car[0], pos(s, 0));
    for (int t = 0, want = 12 + rnd(3); t < 200 && P.nv < want; t++) {
      Car c = { (uint8_t)rnd(2), (uint8_t)(rnd(10) < 3 ? 3 : 2), (uint8_t)rnd(N) };
      if (c.horiz && c.fixed == EXIT_ROW) continue;   // 出口那列放橫車會永遠擋住
      int p = rnd(N - c.len + 1); uint64_t cm = cells(c, p);
      if (cm & m) continue;
      m |= cm; s = setPos(s, P.nv, p); P.car[P.nv++] = c;
    }
    T.clear(); T.add(s);
    for (int k = 0; k < T.n; k++) {
      uint64_t u = T.st[k]; bool full = false;
      forMoves(P, u, [&](int i, int q) { if (T.add(setPos(u, i, q)) < 0) full = true; });
      if (full) return -1;
    }
    int qn = 0;
    for (int k = 0; k < T.n; k++) { T.dist[k] = 255; if (pos(T.st[k], 0) == GOAL) { T.dist[k] = 0; T.q[qn++] = k; } }
    if (!qn) return -1;
    for (int h = 0; h < qn; h++) {
      int k = T.q[h]; uint64_t u = T.st[k];
      forMoves(P, u, [&](int i, int q) { int j = T.find(setPos(u, i, q)); if (T.dist[j] == 255) { T.dist[j] = T.dist[k] + 1; T.q[qn++] = j; } });
    }
    int best = 0; for (int k = 1; k < T.n; k++) if (T.dist[k] > T.dist[best]) best = k;
    start = T.st[best]; return T.dist[best];
  }
  // 往出口近一步的盤面(已在出口就原樣回傳)
  static uint64_t hint(const Puzzle& P, const Table& T, uint64_t s) {
    int d = T.distOf(s); uint64_t r = s;
    if (d > 0) forMoves(P, s, [&](int i, int q) { uint64_t v = setPos(s, i, q); if (T.distOf(v) == d - 1) r = v; });
    return r;
  }
}
