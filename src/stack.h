#pragma once
#include "ringlib.h"

// ================= 32. 物理方塊堆 =================
// 俄羅斯方塊的形狀,但每塊是真的剛體:會掉、會翻、會歪著卡住,落地不彈跳(影片裡就是這樣)。
// 一列填夠滿就消掉那一列的格子,剩下的碎塊繼續當剛體。傾斜(或 A/C)左右推正在掉的那塊,點螢幕讓它轉
// (點在左半逆轉、右半順轉)。有塊停在紅線以上就結束,分數是消的列數,前 5 名存 NVS。
// ponytail: 每塊 = 4 個 Verlet 點 + 6 條距離約束(位置式剛體,跟布娃娃同一套),碰撞是圓對圓的位置推開 + 摩擦;
//           衝量式剛體在這裡會亂彈,位置式的堆疊很穩
namespace stack {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CS = 14, COLS = 16, AX0 = (W - COLS * CS) / 2, AX1 = AX0 + COLS * CS, TOPLINE = 30, MAXP = 40, SUB = 2, ITER = 4;
  constexpr float CR = 6.5f, G = 500, FRIC = 0.5f, DAMP = 0.998f, PUSH = 600, SPIN = 5, VCAP = 6;   // VCAP:每子步最多移動幾 px
  struct Pt { float x, y, ox, oy; };
  struct Piece { Pt q[4]; int8_t lx[4], ly[4]; bool live, dead[4]; uint8_t n; uint32_t col; } static p[MAXP];
  static const int8_t SHAPES[7][4][2] = { {{-1,0},{0,0},{1,0},{2,0}}, {{-1,0},{0,0},{1,0},{1,-1}}, {{-1,0},{0,0},{1,0},{-1,-1}}, {{0,0},{1,0},{0,-1},{1,-1}},
                                          {{-1,0},{0,0},{0,-1},{1,-1}}, {{-1,0},{0,0},{1,0},{0,-1}}, {{-1,-1},{0,-1},{0,0},{1,0}} };
  static int cur, lines, rank, next, rowCnt[H / CS + 1]; static float spawnT, endT; static bool over;
  static Board board = { "stack" };
  static float restLen(const Piece& a, int i, int j) { float dx = (a.lx[i] - a.lx[j]) * CS, dy = (a.ly[i] - a.ly[j]) * CS; return sqrtf(dx * dx + dy * dy); }
  static void spawn() {
    for (int i = 0; i < MAXP; i++) if (!p[i].live) {
      auto& a = p[i]; a = {}; a.live = true; a.n = 4; a.col = hsv(next / 7.0f); float cx = AX0 + COLS * CS / 2 + (frand() - 0.5f) * 80, cy = -24;
      for (int k = 0; k < 4; k++) { a.lx[k] = SHAPES[next][k][0]; a.ly[k] = SHAPES[next][k][1]; a.q[k] = { cx + a.lx[k] * CS, cy + a.ly[k] * CS, cx + a.lx[k] * CS, cy + a.ly[k] * CS }; }
      cur = i; next = (int)(frand() * 7) % 7; return;
    }
  }
  void init() { memset(p, 0, sizeof p); cur = -1; lines = 0; rank = -1; spawnT = endT = 0; over = false; next = (int)(frand() * 7) % 7; spawn(); ringlib::reset(); cv.fillScreen(0); }
  // 把點沿法向推出 pen,法向速度歸零、切向速度乘 FRIC
  static void pushOut(Pt& q, float nx, float ny, float pen) {
    float vx = q.x - q.ox, vy = q.y - q.oy, vn = vx * nx + vy * ny, tx = vx - vn * nx, ty = vy - vn * ny;
    q.x += nx * pen; q.y += ny * pen; q.ox = q.x - tx * FRIC; q.oy = q.y - ty * FRIC;
  }
  static void centroid(const Piece& a, float& cx, float& cy) { cx = cy = 0; int n = 0; for (int k = 0; k < 4; k++) if (!a.dead[k]) { cx += a.q[k].x; cy += a.q[k].y; n++; } if (n) { cx /= n; cy /= n; } }
  static float speed(const Piece& a, float dt) { float s = 0; for (int k = 0; k < 4; k++) if (!a.dead[k]) { float vx = a.q[k].x - a.q[k].ox, vy = a.q[k].y - a.q[k].oy; s = fmaxf(s, sqrtf(vx * vx + vy * vy)); } return s / dt; }
  static void clearLines() {   // 每列格子數到 COLS-1 就消
    memset(rowCnt, 0, sizeof rowCnt);
    for (auto& a : p) if (a.live) for (int k = 0; k < 4; k++) if (!a.dead[k]) { int r = (int)(a.q[k].y / CS); if (r >= 0 && r < H / CS) rowCnt[r]++; }
    for (int r = 0; r < H / CS; r++) if (rowCnt[r] >= COLS - 1) {
      for (auto& a : p) if (a.live) { for (int k = 0; k < 4; k++) if (!a.dead[k] && (int)(a.q[k].y / CS) == r) { a.dead[k] = true; a.n--; spark(a.q[k].x, a.q[k].y, a.col); } if (!a.n) a.live = false; }
      lines++; snd::note(500 + lines * 20); buzz(120, 60);
    }
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); stepSparks(c.dt); return; }
    float dt = c.dt / SUB;
    if (cur >= 0 && p[cur].live) {   // 操作正在掉的那塊:傾斜推、點螢幕轉
      auto& a = p[cur]; float cx, cy; centroid(a, cx, cy);
      for (int k = 0; k < 4; k++) if (!a.dead[k]) {
        a.q[k].ox -= c.gx * PUSH * c.dt * c.dt;
        if (c.tap) { float w = (c.tx > W / 2 ? 1 : -1) * SPIN * c.dt; a.q[k].ox += w * (a.q[k].y - cy); a.q[k].oy -= w * (a.q[k].x - cx); }
      }
      if (c.tap) snd::click();
    }
    for (int s = 0; s < SUB; s++) {
      for (auto& a : p) if (a.live) for (int k = 0; k < 4; k++) if (!a.dead[k]) {   // Verlet 積分,位移封頂避免穿透
        auto& q = a.q[k]; float vx = (q.x - q.ox) * DAMP, vy = (q.y - q.oy) * DAMP + G * dt * dt;
        float v = sqrtf(vx * vx + vy * vy); if (v > VCAP) { vx *= VCAP / v; vy *= VCAP / v; }
        q.ox = q.x; q.oy = q.y; q.x += vx; q.y += vy;
      }
      for (int it = 0; it < ITER; it++) {
        for (auto& a : p) if (a.live) {
          for (int i = 0; i < 4; i++) if (!a.dead[i]) for (int j = i + 1; j < 4; j++) if (!a.dead[j]) {   // 形狀約束
            auto& u = a.q[i]; auto& v = a.q[j]; float dx = v.x - u.x, dy = v.y - u.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f, k = (d - restLen(a, i, j)) / d * 0.5f;
            u.x += dx * k; u.y += dy * k; v.x -= dx * k; v.y -= dy * k;
          }
          for (int k = 0; k < 4; k++) if (!a.dead[k]) {   // 牆與地板
            auto& q = a.q[k];
            if (q.x < AX0 + CR) pushOut(q, 1, 0, AX0 + CR - q.x); if (q.x > AX1 - CR) pushOut(q, -1, 0, q.x - (AX1 - CR)); if (q.y > H - CR) pushOut(q, 0, -1, q.y - (H - CR));
          }
        }
        for (int i = 0; i < MAXP; i++) if (p[i].live) for (int j = i + 1; j < MAXP; j++) if (p[j].live) {   // ponytail: O(n^2) 塊對塊,n<=40,先用中心距離篩
          auto& a = p[i]; auto& b = p[j]; if (fabsf(a.q[1].x - b.q[1].x) > 5 * CS || fabsf(a.q[1].y - b.q[1].y) > 5 * CS) continue;
          for (int k = 0; k < 4; k++) if (!a.dead[k]) for (int l = 0; l < 4; l++) if (!b.dead[l]) {
            auto& u = a.q[k]; auto& v = b.q[l]; float dx = u.x - v.x, dy = u.y - v.y, d2 = dx * dx + dy * dy;
            if (d2 >= 4 * CR * CR || d2 < 1e-3f) continue;
            float d = sqrtf(d2), nx = dx / d, ny = dy / d, pen = (2 * CR - d) / 2; pushOut(u, nx, ny, pen); pushOut(v, -nx, -ny, pen);
          }
        }
      }
    }
    // 正在掉的那塊停下來(慢過 12 px/s 持續 0.5 秒)就消列、換下一塊;有塊停在頂線以上就結束
    bool settled = cur < 0 || !p[cur].live || (speed(p[cur], dt) < 12 && p[cur].q[1].y > 0);
    if (settled) { if ((spawnT += c.dt) > 0.5f) { spawnT = 0; clearLines(); spawn(); } } else spawnT = 0;
    for (int i = 0; i < MAXP; i++) if (p[i].live && i != cur && speed(p[i], dt) < 12) { float cx, cy; centroid(p[i], cx, cy); if (cy < TOPLINE) { over = true; rank = board.record(lines); buzz(200, 200); snd::note(150); } }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    cv.drawFastVLine(AX0 - 1, 0, H, rgb(80, 80, 100)); cv.drawFastVLine(AX1, 0, H, rgb(80, 80, 100)); cv.drawFastHLine(AX0, TOPLINE, COLS * CS, rgb(120, 40, 40));
    for (auto& a : p) if (a.live) {   // 每格畫成轉過的小方塊;方向由前兩個活著的點決定
      int k1 = -1, k2 = -1; for (int k = 0; k < 4; k++) if (!a.dead[k]) { if (k1 < 0) k1 = k; else if (k2 < 0) k2 = k; }
      float ang = 0; if (k2 >= 0) ang = atan2f(a.q[k2].y - a.q[k1].y, a.q[k2].x - a.q[k1].x) - atan2f((float)(a.ly[k2] - a.ly[k1]), (float)(a.lx[k2] - a.lx[k1]));
      float cs = cosf(ang) * (CS / 2 - 1), sn = sinf(ang) * (CS / 2 - 1);
      for (int k = 0; k < 4; k++) if (!a.dead[k]) {
        float cx = a.q[k].x, cy = a.q[k].y;
        int x0 = (int)(cx - cs + sn), y0 = (int)(cy - sn - cs), x1 = (int)(cx + cs + sn), y1 = (int)(cy + sn - cs), x2 = (int)(cx + cs - sn), y2 = (int)(cy + sn + cs), x3 = (int)(cx - cs - sn), y3 = (int)(cy - sn + cs);
        cv.fillTriangle(x0, y0, x1, y1, x2, y2, a.col); cv.fillTriangle(x0, y0, x2, y2, x3, y3, a.col);
      }
    }
    for (int k = 0; k < 4; k++) cv.drawRect(AX1 + 8 + (SHAPES[next][k][0] + 1) * 6, 8 + (SHAPES[next][k][1] + 1) * 6, 5, 5, hsv(next / 7.0f));   // 下一塊
    drawSparks();
    char t[24]; snprintf(t, sizeof t, "lines %d", lines);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(t, 4, 4);
    if (over) { snprintf(t, sizeof t, "LINES %d", lines); board.draw(t, rank); }
  }
}
