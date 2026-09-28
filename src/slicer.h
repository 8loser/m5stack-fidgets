#pragma once
#include "ringlib.h"

// ================= 切割 =================
// 整個螢幕裡的多邊形帶著重力彈跳翻滾,開局點一下的位置放一顆不動的小球當切割點(放之前圖塊不長大);多邊形掃過小球就沿掃過的路徑被切成兩片,
// 切下來的小半換新顏色、大半保留原色,所有片都會慢慢長大。有一塊長到超過螢幕 1/20 就整圈炸開,比共切了幾刀,前 5 名排行。搖一下全部噴開、亂轉
namespace slicer {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CXS = W / 2, CYS = H / 2, MAXP = 64, MAXV = 10; constexpr float G = 300, MIN_AREA = 25, REST = 0.75f, GROW = 0.08f, MAX_R = 120, BIG = W * H / 20.0f;   // 每秒長大 8%,最遠頂點到 120 px 停(免得細長碎片比螢幕還大);BIG:任一塊面積超過就結束
  struct Poly { int n; float x[MAXV], y[MAXV]; float cx, cy, vx, vy, ang, vang; uint32_t col; bool live, touching, round; float ex, ey; } static p[MAXP];
  static bool over; static float endT, biggest; static int cuts, rank; static float kx, ky; static bool placed;   // kx, ky:切割點   // biggest:最大那塊的面積
  static Board board = { "slicer" };
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
    memset(p, 0, sizeof p); over = false; endT = biggest = 0; cuts = 0; rank = -1; placed = false; ringlib::reset();
    Poly& q = *slot(); q.n = MAXV; q.live = true; q.col = rgb(255, 40, 80); q.cx = CXS - 40; q.cy = CYS - 50; q.vx = 90; q.vy = 0; q.vang = 0.8f; q.round = true;
    for (int i = 0; i < q.n; i++) { float a = i * 6.2831853f / q.n; q.x[i] = 22 * cosf(a); q.y[i] = 22 * sinf(a); }   // 物理用 10 邊形近似,還沒被切之前畫成圓
    cv.fillScreen(0);
  }
  static bool push(Poly& q, float x, float y) { if (q.n >= MAXV) return false; q.x[q.n] = x; q.y[q.n] = y; q.n++; return true; }
  static void cut(Poly& q, float ax, float ay, float bx, float by) {   // 沿 local 座標的 A→B 直線切凸多邊形
    Poly L = q, R = q; L.n = R.n = 0; L.round = R.round = false;
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
    cuts++; spark(kx, ky, q.col); snd::note(300 + area(L) * 0.4f);
  }
  void step(const Ctx& c) {
    if (over && (endT += c.dt) > 1.5f && c.tap) { init(); return; }
    if (!placed && c.tap) { placed = true; kx = c.tx; ky = c.ty; snd::click(); return; }   // 第一下放切割點,不推圖塊
    int live = 0; biggest = 0;
    if (!over && c.shake > SHAKE) { for (auto& q : p) if (q.live) { shakeKick(c, q.vx, q.vy); q.vang += (frand() - 0.5f) * 8; } buzz(80, 30); }
    for (auto& q : p) if (q.live) {
      live++;
      if (!over && c.tap) { float dx = q.cx - c.tx, dy = q.cy - c.ty, d2 = dx * dx + dy * dy + 100; if (d2 < 80 * 80) { float f = 3e5f / d2; q.vx += dx / sqrtf(d2) * f; q.vy += dy / sqrtf(d2) * f; } }
      q.vx += c.gx * G * c.dt; q.vy += c.gy * G * c.dt;
      float v = sqrtf(q.vx * q.vx + q.vy * q.vy); if (!over && v < 40 && v > 1e-3f) { q.vx *= 40 / v; q.vy *= 40 / v; }
      q.cx += q.vx * c.dt; q.cy += q.vy * c.dt; q.ang += q.vang * c.dt; q.vang *= 0.995f;
      if (!over && placed) {   // 慢慢長大
        float r2 = 0; for (int i = 0; i < q.n; i++) r2 = fmaxf(r2, q.x[i] * q.x[i] + q.y[i] * q.y[i]);
        if (r2 < MAX_R * MAX_R) { float k = 1 + GROW * c.dt; for (int i = 0; i < q.n; i++) { q.x[i] *= k; q.y[i] *= k; } }
        biggest = fmaxf(biggest, area(q));
      }
      if (!over) {   // 頂點超出螢幕四邊就推回來、反彈
        float x0 = W, x1 = 0, y0 = H, y1 = 0, vn = 0;
        for (int i = 0; i < q.n; i++) { float wx, wy; toWorld(q, i, wx, wy); x0 = fminf(x0, wx); x1 = fmaxf(x1, wx); y0 = fminf(y0, wy); y1 = fmaxf(y1, wy); }
        if (x0 < 0) { q.cx -= x0; if (q.vx < 0) { vn = fmaxf(vn, -q.vx); q.vx *= -REST; } }
        if (x1 > W) { q.cx -= x1 - W; if (q.vx > 0) { vn = fmaxf(vn, q.vx); q.vx *= -REST; } }
        if (y0 < 0) { q.cy -= y0; if (q.vy < 0) { vn = fmaxf(vn, -q.vy); q.vy *= -REST; } }
        if (y1 > H) { q.cy -= y1 - H; if (q.vy > 0) { vn = fmaxf(vn, q.vy); q.vy *= -REST; } }
        if (vn > 0) { q.vang += (frand() - 0.5f) * 2; if (vn > 60) snd::click(); }
        if (!placed) continue;
        float lx, ly; toLocal(q, kx, ky, lx, ly); bool in = containsLocal(q, lx, ly);
        if (in && !q.touching) { q.touching = true; q.ex = lx; q.ey = ly; }
        else if (!in && q.touching) { q.touching = false; cut(q, q.ex, q.ey, lx, ly); }
      }
    }
    if (!over && biggest > BIG) {   // 有一塊超過螢幕 1/20:全部往外炸。片數滿了不結束,切下的另一半直接消失(刀數照算),圖塊照樣長大
      over = true; rank = board.record(cuts); buzz(200, 150);
      for (auto& q : p) if (q.live) { float dx = q.cx - kx, dy = q.cy - ky, d = sqrtf(dx * dx + dy * dy) + 1; q.vx = dx / d * 350; q.vy = dy / d * 350; q.vang = (frand() - 0.5f) * 8; }
    }
    if (!live) init();
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(0);
    for (auto& q : p) if (q.live) {
      float wx[MAXV], wy[MAXV]; for (int i = 0; i < q.n; i++) toWorld(q, i, wx[i], wy[i]);
      if (q.round) { cv.fillCircle((int)q.cx, (int)q.cy, (int)sqrtf(q.x[0] * q.x[0] + q.y[0] * q.y[0]), q.col); continue; }
      for (int i = 1; i + 1 < q.n; i++) cv.fillTriangle((int)wx[0], (int)wy[0], (int)wx[i], (int)wy[i], (int)wx[i + 1], (int)wy[i + 1], q.col);
    }
    if (placed) cv.fillCircle((int)kx, (int)ky, 3, rgb(255, 255, 255));
    else { cv.setTextDatum(middle_center); cv.setTextSize(1); cv.setTextColor(rgb(200, 200, 200), 0); cv.drawString("tap to place cutter", W / 2, H - 16); }
    drawSparks();
    char s[24]; snprintf(s, sizeof s, "cuts %d  max %d%%", cuts, (int)(biggest * 100 / BIG));
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString(s, 4, 4);
    if (over) { snprintf(s, sizeof s, "CUTS %d", cuts); board.draw(s, rank); }
  }
}
