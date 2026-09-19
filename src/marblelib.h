#pragma once
#include "ringlib.h"

// ================= 彈珠系列共用:彈珠池、碰撞、右側色管 =================
// 6 色彈珠輪流從上面倒進場地(左邊 256 px),穿過各遊戲自己的障礙掉出底部,就依顏色堆進右邊的管子累積;
// 哪根管子先堆滿就全部清空重來。沒有輸贏。重力永遠向下,傾斜只給左右分量;點螢幕推開彈珠。
// 物理每幀切 4 個子步,彈珠才不會穿過薄障礙
namespace marblelib {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int AW = 256, NCOL = 6, MAXM = 72, R = 3, SUB = 4, TUBE_MAX = 54; constexpr float G = 520, REST = 0.4f, VMAX = 380;
  struct M { float x, y, vx, vy; int8_t col; bool live; } static m[MAXM];
  static float spawnT; static int tube[NCOL], spawned, lost;
  static uint32_t col(int c) { return hsv(c / (float)NCOL); }
  static void reset() { memset(m, 0, sizeof m); memset(tube, 0, sizeof tube); spawnT = 0; spawned = lost = 0; ringlib::reset(); cv.fillScreen(0); }
  static void spawnNext(float x) { for (auto& o : m) if (!o.live) { o = { x + (frand() - 0.5f) * 30, -8, (frand() - 0.5f) * 40, 0, (int8_t)(spawned % NCOL), true }; spawned++; return; } }   // 六色輪流
  static float barT;   // 上一次 hitBar 撞到的位置(沿桿子的距離,中心為 0);蹺蹺板拿它算力矩
  static bool hitBar(M& o, float bx, float by, float len, float a, float spin = 0) {   // 線段:中心、長、角;spin 是角速度,撞到吃到桿面速度
    float ux = cosf(a), uy = sinf(a), dx = o.x - bx, dy = o.y - by, t = dx * ux + dy * uy; t = t < -len / 2 ? -len / 2 : t > len / 2 ? len / 2 : t;
    float px = bx + ux * t, py = by + uy * t, ex = o.x - px, ey = o.y - py, d = sqrtf(ex * ex + ey * ey) + 1e-3f, rr = R + 2;
    if (d >= rr) return false;
    barT = t; float nx = ex / d, ny = ey / d; o.x = px + nx * rr; o.y = py + ny * rr;
    float sx = -spin * (py - by), sy = spin * (px - bx), vn = (o.vx - sx) * nx + (o.vy - sy) * ny;
    if (vn < 0) { o.vx -= (1 + REST) * vn * nx; o.vy -= (1 + REST) * vn * ny; }
    return true;
  }
  static bool hitCirc(M& o, float cx, float cy, float r) {
    float dx = o.x - cx, dy = o.y - cy, d = sqrtf(dx * dx + dy * dy) + 1e-3f, rr = r + R; if (d >= rr) return false;
    float nx = dx / d, ny = dy / d, vn = o.vx * nx + o.vy * ny; o.x = cx + nx * rr; o.y = cy + ny * rr; if (vn < 0) { o.vx -= (1 + REST) * vn * nx; o.vy -= (1 + REST) * vn * ny; }
    return true;
  }
  static void hitArc(M& o, float cx, float cy, float r, float a0, float a1) {   // 弧:只有 a0..a1(度)這段是實體,兩面都擋
    float dx = o.x - cx, dy = o.y - cy, d = sqrtf(dx * dx + dy * dy) + 1e-3f; if (fabsf(d - r) >= R + 2) return;
    float a = fmodf(atan2f(dy, dx) * RAD_TO_DEG - a0 + 720, 360), span = fmodf(a1 - a0 + 720, 360); if (a > span) return;
    float nx = dx / d, ny = dy / d, side = d < r ? -1 : 1, vn = o.vx * nx + o.vy * ny;
    o.x = cx + nx * (r + side * (R + 2)); o.y = cy + ny * (r + side * (R + 2));
    if (vn * side < 0) { o.vx -= (1 + REST) * vn * nx; o.vy -= (1 + REST) * vn * ny; }
  }
  // 每幀呼叫;collide 對每顆彈珠處理遊戲自己的障礙,每個子步呼叫一次
  static void integrate(const Ctx& c, float spawnX, void (*collide)(M&)) {
    if ((spawnT += c.dt) > 0.25f) { spawnT = 0; spawnNext(spawnX); }
    if (c.tap && c.tx < AW) for (auto& o : m) if (o.live) { float dx = o.x - c.tx, dy = o.y - c.ty, d2 = dx * dx + dy * dy + 100; if (d2 < 45 * 45) { float f = 1.2e5f / d2; o.vx += dx / sqrtf(d2) * f; o.vy += dy / sqrtf(d2) * f; } }
    float dt = c.dt / SUB;
    for (int s = 0; s < SUB; s++) {
      for (auto& o : m) if (o.live) {
        o.vx += c.gx * G * dt; o.vy += G * dt; o.vx *= 1 - 0.15f * dt;
        float v = sqrtf(o.vx * o.vx + o.vy * o.vy); if (v > VMAX) { o.vx *= VMAX / v; o.vy *= VMAX / v; }
        o.x += o.vx * dt; o.y += o.vy * dt;
        if (o.x < R) { o.x = R; o.vx = fabsf(o.vx) * REST; } if (o.x > AW - R) { o.x = AW - R; o.vx = -fabsf(o.vx) * REST; }
        collide(o);
        if (o.y > H + 6) { o.live = false; tube[o.col]++; snd::click(); }   // 掉出底部:進管子
      }
      for (int i = 0; i < MAXM; i++) if (m[i].live) for (int j = i + 1; j < MAXM; j++) if (m[j].live) {   // ponytail: O(n^2),n<=72
        auto& p = m[i]; auto& q = m[j]; float dx = q.x - p.x, dy = q.y - p.y, d2 = dx * dx + dy * dy;
        if (d2 >= 4 * R * R || d2 < 1e-3f) continue;
        float d = sqrtf(d2), nx = dx / d, ny = dy / d, pen = (2 * R - d) / 2; p.x -= nx * pen; p.y -= ny * pen; q.x += nx * pen; q.y += ny * pen;
        float rv = (q.vx - p.vx) * nx + (q.vy - p.vy) * ny; if (rv < 0) { float jn = -(1 + REST) * rv / 2; p.vx -= nx * jn; p.vy -= ny * jn; q.vx += nx * jn; q.vy += ny * jn; }
      }
    }
    for (int k = 0; k < NCOL; k++) if (tube[k] >= TUBE_MAX) { memset(tube, 0, sizeof tube); for (int i = 0; i < 6; i++) spark(AW + 4 + k * 10, 20 + i * 30, col(k)); snd::note(700); buzz(120, 80); }   // 有管子滿了:全部清空
    stepSparks(c.dt);
  }
  static void drawMarbles() { for (auto& o : m) if (o.live) cv.fillCircle((int)o.x, (int)o.y, R, col(o.col)); drawSparks(); }
  static void hud(const char* what) {   // 右側色管;what 非空就在左上顯示它的計數(尖刺碗的 popped)
    cv.drawFastVLine(AW + 1, 0, H, rgb(60, 60, 70));
    for (int c = 0; c < NCOL; c++) {
      int x = AW + 4 + c * 10;
      cv.drawRect(x, 14, 8, H - 18, rgb(70, 70, 80));
      for (int k = 0; k < tube[c]; k++) cv.fillRect(x + 2, H - 8 - k * 4, 4, 3, col(c));
    }
    if (*what) { char t[24]; snprintf(t, sizeof t, "%s %d", what, lost); cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4); }
  }
}
