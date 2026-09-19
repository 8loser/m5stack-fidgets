#pragma once
#include "ringlib.h"

// ================= 5. 切割 =================
// 圓框裡的多邊形帶著重力彈跳翻滾,正中央有一顆不動的小球;多邊形掃過小球就沿掃過的路徑被切成兩片,
// 切下來的小半換新顏色、大半保留原色,所有片都會慢慢長大。片數到上限後整圈炸開,重來
namespace slicer {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks; using ringlib::segCol;
  constexpr int CXS = 160, CYS = 120, RAD = 108, MAXP = 64, MAXV = 10; constexpr float G = 300, MIN_AREA = 25, REST = 0.75f, GROW = 0.08f, MAX_R = 55;   // 每秒長大 8%,最遠頂點到 55 px 停
  struct Poly { int n; float x[MAXV], y[MAXV]; float cx, cy, vx, vy, ang, vang; uint32_t col; bool live, touching; float ex, ey; } static p[MAXP];
  static bool over; static float endT;
  static const uint8_t PAL[7][3] = { {255,40,80},{255,120,0},{255,220,0},{0,230,120},{0,200,255},{120,60,255},{255,0,200} };
  static uint32_t palCol() { auto& c = PAL[(int)(frand() * 7) % 7]; return rgb(c[0], c[1], c[2]); }
  static float area(const Poly& q) { float a = 0; for (int i = 0; i < q.n; i++) { int j = (i + 1) % q.n; a += q.x[i] * q.y[j] - q.x[j] * q.y[i]; } return fabsf(a) / 2; }
  static void toWorld(const Poly& q, int i, float& wx, float& wy) { float c = cosf(q.ang), s = sinf(q.ang); wx = q.cx + q.x[i] * c - q.y[i] * s; wy = q.cy + q.x[i] * s + q.y[i] * c; }
  static void toLocal(const Poly& q, float wx, float wy, float& lx, float& ly) { float c = cosf(q.ang), s = sinf(q.ang), dx = wx - q.cx, dy = wy - q.cy; lx = dx * c + dy * s; ly = -dx * s + dy * c; }
  static bool containsLocal(const Poly& q, float x, float y) {
    bool pos = false, neg = false;
    for (int i = 0; i < q.n; i++) { int j = (i + 1) % q.n; float cr = (q.x[j] - q.x[i]) * (y - q.y[i]) - (q.y[j] - q.y[i]) * (x - q.x[i]); pos |= cr > 0; neg |= cr < 0; }
    return !(pos && neg);
  }
  static void recenter(Poly& q) {   // 把 local 頂點重心移到原點,世界座標跟著補償
    float mx = 0, my = 0; for (int i = 0; i < q.n; i++) { mx += q.x[i]; my += q.y[i]; } mx /= q.n; my /= q.n;
    for (int i = 0; i < q.n; i++) { q.x[i] -= mx; q.y[i] -= my; }
    float c = cosf(q.ang), s = sinf(q.ang); q.cx += mx * c - my * s; q.cy += mx * s + my * c;
  }
  static Poly* slot() { for (auto& q : p) if (!q.live) return &q; return nullptr; }
  void init() {
    memset(p, 0, sizeof p); over = false; endT = 0; ringlib::reset();
    Poly& q = *slot(); q.n = MAXV; q.live = true; q.col = rgb(255, 40, 80); q.cx = CXS - 40; q.cy = CYS - 50; q.vx = 90; q.vy = 0; q.vang = 0.8f;
    for (int i = 0; i < q.n; i++) { float a = i * 6.2831853f / q.n; q.x[i] = 22 * cosf(a); q.y[i] = 22 * sinf(a); }   // 圓盤用 10 邊形近似
    cv.fillScreen(0);
  }
  static bool push(Poly& q, float x, float y) { if (q.n >= MAXV) return false; q.x[q.n] = x; q.y[q.n] = y; q.n++; return true; }
  static void cut(Poly& q, float ax, float ay, float bx, float by) {   // 沿 local 座標的 A→B 直線切凸多邊形
    Poly L = q, R = q; L.n = R.n = 0;
    float nx = -(by - ay), ny = bx - ax, nl = sqrtf(nx * nx + ny * ny); if (nl < 1e-3f) return; nx /= nl; ny /= nl;
    for (int i = 0; i < q.n; i++) {
      int j = (i + 1) % q.n;
      float sc = (q.x[i] - ax) * nx + (q.y[i] - ay) * ny, sn = (q.x[j] - ax) * nx + (q.y[j] - ay) * ny;
      if (sc >= 0 && !push(L, q.x[i], q.y[i])) return; if (sc <= 0 && !push(R, q.x[i], q.y[i])) return;
      if ((sc > 0 && sn < 0) || (sc < 0 && sn > 0)) { float t = sc / (sc - sn), ix = q.x[i] + (q.x[j] - q.x[i]) * t, iy = q.y[i] + (q.y[j] - q.y[i]) * t;
        if (!push(L, ix, iy) || !push(R, ix, iy)) return; }
    }
    if (L.n < 3 || R.n < 3) return;
    L.live = area(L) >= MIN_AREA; R.live = area(R) >= MIN_AREA;
    float c = cosf(q.ang), s = sinf(q.ang), wnx = nx * c - ny * s, wny = nx * s + ny * c;   // 法向轉回世界座標
    L.vx += wnx * 40; L.vy += wny * 40; R.vx -= wnx * 40; R.vy -= wny * 40;
    L.vang = (frand() - 0.5f) * 3; R.vang = (frand() - 0.5f) * 3; (area(L) >= area(R) ? R : L).col = palCol(); L.touching = R.touching = false;
    if (L.live) recenter(L); if (R.live) recenter(R);
    Poly* f = R.live ? slot() : nullptr;
    q = L; if (f && f != &q) *f = R;   // 沒空位就讓另一半消失
    spark(CXS, CYS, q.col); snd::note(300 + area(L) * 0.4f);
  }
  void step(const Ctx& c) {
    int live = 0;
    for (auto& q : p) if (q.live) {
      live++;
      if (c.tap) { float dx = q.cx - c.tx, dy = q.cy - c.ty, d2 = dx * dx + dy * dy + 100; if (d2 < 80 * 80) { float f = 3e5f / d2; q.vx += dx / sqrtf(d2) * f; q.vy += dy / sqrtf(d2) * f; } }
      q.vx += c.gx * G * c.dt; q.vy += c.gy * G * c.dt;
      float v = sqrtf(q.vx * q.vx + q.vy * q.vy); if (!over && v < 40 && v > 1e-3f) { q.vx *= 40 / v; q.vy *= 40 / v; }
      q.cx += q.vx * c.dt; q.cy += q.vy * c.dt; q.ang += q.vang * c.dt; q.vang *= 0.995f;
      if (!over) {   // 慢慢長大
        float r2 = 0; for (int i = 0; i < q.n; i++) r2 = fmaxf(r2, q.x[i] * q.x[i] + q.y[i] * q.y[i]);
        if (r2 < MAX_R * MAX_R) { float k = 1 + GROW * c.dt; for (int i = 0; i < q.n; i++) { q.x[i] *= k; q.y[i] *= k; } }
      }
      if (!over) {   // 離圓心最遠的頂點超出框就推回來、反彈
        float far = 0, fx = 0, fy = 0;
        for (int i = 0; i < q.n; i++) { float wx, wy; toWorld(q, i, wx, wy); float dx = wx - CXS, dy = wy - CYS, d = dx * dx + dy * dy; if (d > far) { far = d; fx = dx; fy = dy; } }
        far = sqrtf(far);
        if (far > RAD - 2) {
          float nx = fx / far, ny = fy / far, pen = far - (RAD - 2), vn = q.vx * nx + q.vy * ny;
          q.cx -= nx * pen; q.cy -= ny * pen;
          if (vn > 0) { q.vx -= (1 + REST) * vn * nx; q.vy -= (1 + REST) * vn * ny; q.vang += (frand() - 0.5f) * 2; if (vn > 60) snd::click(); }
        }
        float lx, ly; toLocal(q, CXS, CYS, lx, ly); bool in = containsLocal(q, lx, ly);
        if (in && !q.touching) { q.touching = true; q.ex = lx; q.ey = ly; }
        else if (!in && q.touching) { q.touching = false; cut(q, q.ex, q.ey, lx, ly); }
      }
    }
    if (!over && live >= MAXP - 2) {   // 滿了:全部往外炸
      over = true; buzz(200, 150);
      for (auto& q : p) if (q.live) { float dx = q.cx - CXS, dy = q.cy - CYS, d = sqrtf(dx * dx + dy * dy) + 1; q.vx = dx / d * 350; q.vy = dy / d * 350; q.vang = (frand() - 0.5f) * 8; }
    }
    if ((over && (endT += c.dt) > 2) || !live) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    if (!over) for (int s = 0; s < 6; s++) ringlib::arc(CXS, CYS, RAD, s * 60 + 2, s * 60 + 58, segCol(s));
    for (auto& q : p) if (q.live) {
      float wx[MAXV], wy[MAXV]; for (int i = 0; i < q.n; i++) toWorld(q, i, wx[i], wy[i]);
      for (int i = 1; i + 1 < q.n; i++) cv.fillTriangle((int)wx[0], (int)wy[0], (int)wx[i], (int)wy[i], (int)wx[i + 1], (int)wy[i + 1], q.col);
    }
    cv.fillCircle(CXS, CYS, 3, rgb(255, 255, 255));
    drawSparks();
  }
}
