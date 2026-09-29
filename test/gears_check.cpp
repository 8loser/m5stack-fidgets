// gearsim 自我檢查(主機上跑):g++ -O2 -Isrc test/gears_check.cpp -o /tmp/gc && /tmp/gc
// 每關:全空接不上、照正解接得上且不卡死、少放任一格就接不上、陷阱放 trap 齒數就卡死;另核對咬合的兩顆齒對齒縫
#include "gearsim.h"
#include <cassert>
#include <cstdio>
using namespace gearsim;

int main() {
  for (int level = 1; level <= 12; level++) for (int s = 1; s <= 200; s++) {
    seed = s * 2654435761u | 1; gen(level);
    int nd = 0; for (int i = 2; i < ng; i++) nd += g[i].trap > 0;
    assert(nd == (level >= 6 ? 2 : level >= 3 ? 1 : 0));
    assert(!solve(0) && g[1].rho == 0);                        // 全空
    for (int i = 2; i < ng; i++) g[i].n = g[i].ans;
    assert(!solve(0.7f) && g[1].rho != 0);                     // 正解
    for (int i = 0; i < ng; i++) if (g[i].rho) {               // 咬合處齒對齒縫:兩邊接觸點的齒位相加 = 0.5(mod 1)
      for (int j = 0; j < ng; j++) if (j != i && g[j].rho && g[j].n) {
        float dx = g[j].x - g[i].x, dy = g[j].y - g[i].y, d = sqrtf(dx * dx + dy * dy);
        if (fabsf(d - rad(g[i].n) - rad(g[j].n)) > TOL) continue;
        float phi = atan2f(dy, dx), ui = (phi - g[i].th) * g[i].n / (2 * PI_F), uj = (phi + PI_F - g[j].th) * g[j].n / (2 * PI_F);
        float f = ui + uj - 0.5f; f -= floorf(f + 0.5f); assert(fabsf(f) < 1e-2f);
      }
    }
    for (int i = 2; i < ng; i++) if (g[i].ans) { g[i].n = 0; assert(solve(0) || g[1].rho == 0); g[i].n = g[i].ans; }
    for (int i = 2; i < ng; i++) if (g[i].trap) { g[i].n = g[i].trap; assert(solve(0)); g[i].n = 0; }
  }
  puts("ok");
}
