#pragma once
#include "ringlib.h"

// ================= 彈珠檯 =================
// 右邊發射道:點螢幕或按 A/C 發射,球沿頂端圓弧繞進場。A / 按住畫面左半 = 左擋板,C / 右半 = 右擋板(兩指可同按);傾斜輕推球。
// 版面照實體機台慣例:上段 3 條燈道 + 3 個彈跳柱排三角;中段兩側牆上各 3 個倒靶;
// 下段由外往內是 outlane(直通漏球口)、隔柱與 inlane 導軌(送回擋板根部)、彈弓三角、擋板。
// 燈道全亮分數倍率 +1,一側倒靶全倒加分重置;3 顆球,比分數,前 5 名排行。選單預覽自動打
namespace pinball {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int R = 5, SUB = 10, BALLS = 3, NB = 3, LANE_X0 = 110, LANE_W = 30, TGT_Y0 = 88, TGT_H = 14;
  constexpr float G = 300, VMAX = 650, FLEN = 32, FSPEED = 18, KICK = 260;   // ponytail: 手感旋鈕,實機再調
  constexpr float LP = 115, RP = 195, PY = 212, REST_A = 0.52f, UP_A = 0.44f;   // 擋板支點;場地以 x=155 左右對稱(鏡像 x' = 310 - x)
  constexpr float GATE_X = 270, GATE_Y0 = 20, GATE_Y1 = 96;   // 發射道頂端的單向閘
  struct Seg { float ax, ay, bx, by; };
  static Seg walls[40]; static int nw;
  static const Seg slingEdge[] = { { 72, 142, 72, 172 }, { 72, 172, 100, 186 }, { 238, 142, 238, 172 }, { 238, 172, 210, 186 } };   // 彈弓不會彈的兩邊
  static const Seg kicker[2] = { { 72, 142, 100, 186 }, { 238, 142, 210, 186 } };   // 彈弓朝場中的斜邊
  static const float tgtX[2] = { 46, 264 };
  static const float bump[NB][2] = { { 125, 78 }, { 185, 78 }, { 155, 108 } }; constexpr float BUMP_R = 11;
  static float x, y, vx, vy, fa[2], fw[2], flash[NB + 2], launchT, endT;
  static int score, mult, ballsLeft, rank; static bool lit[3], down[2][3], inLane, over;
  static Board board = { "pinball" };
  static void wall(float ax, float ay, float bx, float by) { walls[nw++] = { ax, ay, bx, by }; }
  static void arc(float cx, float cy, float r, float a0, float a1) {   // 圓弧切 8 段
    for (int i = 0; i < 8; i++) { float p = a0 + (a1 - a0) * i / 8, q = a0 + (a1 - a0) * (i + 1) / 8; wall(cx + cosf(p) * r, cy + sinf(p) * r, cx + cosf(q) * r, cy + sinf(q) * r); }
  }
  static void build() {
    nw = 0;
    wall(40, 52, 40, 250); arc(80, 52, 40, PI, PI * 1.5f); wall(80, 12, 246, 12); arc(246, 52, 40, PI * 1.5f, PI * 2);   // 外框,頂端兩角是圓弧(發射出來的球沿著繞)
    wall(286, 52, 286, 236); wall(270, 236, 286, 236); wall(GATE_X, GATE_Y1, GATE_X, 250);   // 發射道
    wall(56, 150, 56, 184); wall(56, 184, LP, PY - 2); wall(254, 150, 254, 184); wall(254, 184, RP, PY - 2);   // 隔柱 + inlane 導軌;外側到牆之間是 outlane
    for (int k = 0; k <= 3; k++) wall(LANE_X0 + k * LANE_W, 28, LANE_X0 + k * LANE_W, 46);   // 燈道隔板
  }
  static void serve() { x = 278; y = 236 - 1 - R; vx = vy = 0; inLane = true; launchT = 0; }
  void init() {
    build(); score = 0; mult = 1; ballsLeft = BALLS; rank = -1; over = false; endT = 0;
    memset(lit, 0, sizeof lit); memset(down, 0, sizeof down); memset(flash, 0, sizeof flash);
    fa[0] = REST_A; fa[1] = PI - REST_A; fw[0] = fw[1] = 0; serve(); ringlib::reset(); cv.fillScreen(0);
  }
  static void add(int p) { score = min(score + p * mult, 65535); }
  // 球對膠囊線段(半粗 th);sx,sy 是接觸點的表面速度(擋板在揮);回傳法向撞擊速度,沒撞到回 -1
  static float hitSeg(const Seg& s, float th, float rest, float sx = 0, float sy = 0, float* nxo = nullptr) {
    float ux = s.bx - s.ax, uy = s.by - s.ay, l2 = ux * ux + uy * uy, t = ((x - s.ax) * ux + (y - s.ay) * uy) / l2; t = t < 0 ? 0 : t > 1 ? 1 : t;
    float px = s.ax + ux * t, py = s.ay + uy * t, ex = x - px, ey = y - py, d = sqrtf(ex * ex + ey * ey) + 1e-4f, rr = R + th;
    if (d >= rr) return -1;
    float nx = ex / d, ny = ey / d; x = px + nx * rr; y = py + ny * rr;
    if (nxo) *nxo = nx;
    float vn = (vx - sx) * nx + (vy - sy) * ny; if (vn >= 0) return 0;
    vx -= (1 + rest) * vn * nx; vy -= (1 + rest) * vn * ny; return -vn;
  }
  static void physics(float dt, bool hold[2]) {
    for (int f = 0; f < 2; f++) {   // 擋板:往目標角度轉,轉速存起來算表面速度
      float target = f == 0 ? (hold[0] ? -UP_A : REST_A) : (hold[1] ? PI + UP_A : PI - REST_A);
      float d = target - fa[f], mx = FSPEED * dt; d = d > mx ? mx : d < -mx ? -mx : d; fa[f] += d; fw[f] = d / dt;
    }
    vy += G * dt; float v = sqrtf(vx * vx + vy * vy); if (v > VMAX) { vx *= VMAX / v; vy *= VMAX / v; }
    x += vx * dt; y += vy * dt;
    for (int i = 0; i < nw; i++) if (hitSeg(walls[i], 1, 0.5f) > 60) snd::click();
    if (x < GATE_X) hitSeg({ GATE_X, GATE_Y0, GATE_X, GATE_Y1 }, 1, 0.5f);   // 單向閘:發射道出來可以過,場地裡的球回不去
    for (auto& s : slingEdge) hitSeg(s, 2, 0.5f);
    for (int i = 0; i < 2; i++) {
      float nx, vn = hitSeg(kicker[i], 2, 0.5f, 0, 0, &nx);
      if (vn > 40 && (nx > 0) == (i == 0)) {   // 只有朝場中那面會彈
        float kx = kicker[i].by - kicker[i].ay, ky = kicker[i].ax - kicker[i].bx, l = sqrtf(kx * kx + ky * ky); if (i) { kx = -kx; ky = -ky; }
        vx += kx / l * KICK * 0.8f; vy += ky / l * KICK * 0.8f; flash[NB + i] = 0.15f; add(5); snd::note(520); buzz(40, 15);
      }
    }
    for (int s = 0; s < 2; s++) for (int k = 0; k < 3; k++) if (!down[s][k]) {   // 倒靶:撞到就倒,一側全倒加分重置
      float ty = TGT_Y0 + k * TGT_H;
      if (hitSeg({ tgtX[s], ty + 2, tgtX[s], ty + TGT_H - 2 }, 2, 0.4f) > 30) {
        down[s][k] = true; add(25); spark(tgtX[s], ty + TGT_H / 2, rgb(255, 160, 40)); snd::note(700 + k * 100); buzz(50, 20);
        if (down[s][0] && down[s][1] && down[s][2]) { memset(down[s], 0, sizeof down[s]); add(150); snd::note(1000); buzz(120, 60); }
      }
    }
    for (int i = 0; i < NB; i++) {
      float dx = x - bump[i][0], dy = y - bump[i][1], d = sqrtf(dx * dx + dy * dy) + 1e-4f; if (d >= BUMP_R + R) continue;
      float nx = dx / d, ny = dy / d; x = bump[i][0] + nx * (BUMP_R + R); y = bump[i][1] + ny * (BUMP_R + R);
      float vn = vx * nx + vy * ny; vx += (KICK - vn) * nx; vy += (KICK - vn) * ny;   // 彈跳柱:法向速度一律變成 KICK 往外
      flash[i] = 0.15f; add(10); spark(x, y, hsv(i / 3.0f)); snd::note(600 + i * 150); buzz(60, 20);
    }
    for (int f = 0; f < 2; f++) {
      float px = f ? RP : LP, cx = x - px, cy = y - PY;   // 表面速度 = 角速度 x 半徑(用球的位置近似接觸點)
      if (hitSeg({ px, PY, px + cosf(fa[f]) * FLEN, PY + sinf(fa[f]) * FLEN }, 3, 0.3f, -fw[f] * cy, fw[f] * cx) > 120) snd::click();
    }
    if (y < 42 && y > 32 && x > LANE_X0 && x < LANE_X0 + 3 * LANE_W && vy > 0) {   // 往下穿過燈道
      int k = (int)((x - LANE_X0) / LANE_W);
      if (!lit[k]) { lit[k] = true; add(20); snd::note(800); if (lit[0] && lit[1] && lit[2]) { memset(lit, 0, sizeof lit); mult++; add(100); buzz(120, 60); } }
    }
  }
  void step(const Ctx& c) {
    stepSparks(c.dt); for (auto& f : flash) if (f > 0) f -= c.dt;
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); return; }
    bool hold[2] = { c.btnA, c.btnC };
    for (int i = 0; i < (int)M5.Touch.getCount(); i++) { auto& d = M5.Touch.getDetail(i); if (d.isPressed() && d.y < H) hold[d.x >= W / 2] = true; }   // Ctx 只帶第一指,兩指同按要自己讀(同雙輪)
    if (muted) {   // 選單預覽:球靠近擋板且往下掉就揮
      hold[0] = x < 155 && y > 185 && vy > 0; hold[1] = x >= 155 && y > 185 && vy > 0;
    }
    if (inLane) {
      launchT += c.dt;
      if (c.tap || c.tapA || c.tapC || (muted && launchT > 0.6f)) { inLane = false; vy = -(520 + frand() * 80); snd::note(300); buzz(80, 30); }
    }
    vx += c.gx * 60 * c.dt;   // 傾斜輕推
    for (int s = 0; s < SUB; s++) physics(c.dt / SUB, hold);
    if (y > H + 10) {   // 掉出底部
      buzz(150, 120); snd::note(180);
      if (--ballsLeft > 0) serve();
      else if (muted) init();
      else { over = true; endT = 0; rank = board.record(score); }
    }
  }
  void draw() {
    cv.fillScreen(rgb(10, 12, 40));
    uint32_t wc = rgb(120, 140, 200);
    for (int i = 0; i < nw; i++) cv.drawWideLine(walls[i].ax, walls[i].ay, walls[i].bx, walls[i].by, 1.5f, wc);
    cv.drawLine(GATE_X, GATE_Y0, GATE_X, GATE_Y1, rgb(60, 70, 110));
    for (int i = 0; i < 2; i++) {
      auto& e = slingEdge[i * 2]; auto& k = kicker[i];
      cv.fillTriangle(e.ax, e.ay, e.bx, e.by, k.bx, k.by, rgb(90, 30, 70));
      cv.drawWideLine(k.ax, k.ay, k.bx, k.by, 2.5f, flash[NB + i] > 0 ? rgb(255, 255, 255) : rgb(255, 80, 160));
    }
    for (int s = 0; s < 2; s++) for (int k = 0; k < 3; k++) {
      int ty = TGT_Y0 + k * TGT_H + 2; cv.fillRect(tgtX[s] - (down[s][k] ? 1 : 2), ty, down[s][k] ? 2 : 5, TGT_H - 4, down[s][k] ? rgb(60, 40, 20) : rgb(255, 160, 40));
    }
    for (int i = 0; i < NB; i++) {
      cv.fillCircle(bump[i][0], bump[i][1], BUMP_R, flash[i] > 0 ? rgb(255, 255, 255) : hsv(i / 3.0f));
      cv.drawCircle(bump[i][0], bump[i][1], BUMP_R - 4, rgb(20, 20, 30));
    }
    for (int k = 0; k < 3; k++) cv.fillCircle(LANE_X0 + LANE_W / 2 + k * LANE_W, 38, 3, lit[k] ? rgb(255, 230, 0) : rgb(60, 60, 50));
    for (int f = 0; f < 2; f++) { float px = f ? RP : LP; cv.drawWideLine(px, PY, px + cosf(fa[f]) * FLEN, PY + sinf(fa[f]) * FLEN, 3, rgb(255, 200, 60)); }
    cv.fillCircle(x, y, R, rgb(230, 230, 240)); cv.drawPixel(x - 1, y - 2, rgb(255, 255, 255));
    drawSparks();
    char t[16]; cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(220, 220, 220), rgb(10, 12, 40));
    snprintf(t, sizeof t, "%d", score); cv.drawString(t, 2, 4);
    snprintf(t, sizeof t, "x%d", mult); cv.drawString(t, 2, 18);
    for (int i = 0; i < ballsLeft - (inLane ? 1 : 0); i++) cv.fillCircle(8 + i * 12, 36, R, rgb(230, 230, 240));   // 備用球
    if (over) { snprintf(t, sizeof t, "SCORE %d", score); board.draw(t, rank); }
  }
}
