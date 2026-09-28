// jamgen 自我檢查(主機上跑):g++ -O2 -Isrc test/jamgen_check.cpp -o /tmp/jc && /tmp/jc
// 用另一套以格子陣列實作的走法做正向 BFS,核對產生器回報的最少步數,並檢查提示一路走得到出口
#include "jamgen.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <map>
#include <queue>
#include <vector>
using namespace jamgen;

static int naiveSolve(const Puzzle& P, uint64_t s0) {
  std::map<uint64_t, int> d{{s0, 0}}; std::queue<uint64_t> q; q.push(s0);
  while (!q.empty()) {
    uint64_t s = q.front(); q.pop();
    if (pos(s, 0) == GOAL) return d[s];
    int g[N][N] = {};
    for (int i = 0; i < P.nv; i++) for (int k = 0; k < P.car[i].len; k++) {
      int r = P.car[i].horiz ? P.car[i].fixed : pos(s, i) + k, c = P.car[i].horiz ? pos(s, i) + k : P.car[i].fixed;
      assert(!g[r][c]); g[r][c] = i + 1;   // 不重疊
    }
    for (int i = 0; i < P.nv; i++) for (int dir : {-1, 1}) for (int p = pos(s, i) + dir;; p += dir) {
      const Car& c = P.car[i]; int lead = dir < 0 ? p : p + c.len - 1;
      if (lead < 0 || lead >= N) break;
      int r = c.horiz ? c.fixed : lead, cc = c.horiz ? lead : c.fixed;
      if (g[r][cc] && g[r][cc] != i + 1) break;
      uint64_t v = setPos(s, i, p); if (d.emplace(v, d[s] + 1).second) q.push(v);
    }
  }
  return -1;
}

int main() {
  const int CAP = 4096; Table T{CAP, 0, new uint64_t[CAP], new uint8_t[CAP], new int32_t[2 * CAP], new int32_t[CAP]};
  Puzzle P; int ok = 0, fail = 0, hist[64] = {}; double worstMs = 0;
  for (uint32_t seed = 1; seed <= 2000; seed++) {
    uint64_t st; auto t0 = std::chrono::steady_clock::now();
    int d = attempt(seed * 2654435761u | 1, P, T, st);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); if (ms > worstMs) worstMs = ms;
    if (d < 0) { fail++; continue; }
    ok++; hist[d]++;
    if (seed <= 300) assert(naiveSolve(P, st) == d);
    uint64_t s = st; for (int k = 0; k < d; k++) s = hint(P, T, s);
    assert(pos(s, 0) == GOAL && T.distOf(s) == 0);
  }
  printf("ok %d  reject %d  worst %.2f ms\n", ok, fail, worstMs);
  for (int d = 0; d < 64; d++) if (hist[d]) printf("%2d:%d ", d, hist[d]);
  printf("\n");
}
