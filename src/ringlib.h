#pragma once
#include "common.h"

// ================= 環系列共用:幾何、球、碎片(落下堆積)、火花 =================
namespace ringlib {
  constexpr int CXR = 110, CYR = 120, LAYERS = 8, R0 = 40, DR = 9, SEG = 6, BALL_R = 4, NPIECE = LAYERS * SEG;
  constexpr float G = 350, KICK = 220, HA = 27;   // HA = 每片弧的半角(度),60 度扇區留 6 度縫
  static const float NOTE[SEG] = { 262, 294, 330, 392, 440, 523 };   // 撞到哪個扇區就彈哪個音
  inline uint32_t segCol(int s) { return hsv(s / (float)SEG); }
  static float rot = 0;   // 整個環的旋轉角(度),扇區 s 的起點在 s*60+rot
  // ponytail: 用短線段畫弧,drawArc 每次掃整個外接矩形,幾十條會掉幀
  static void arc(float cx, float cy, float r, float a0, float a1, uint32_t col) {
    float px = cx + r * cosf(a0 * DEG_TO_RAD), py = cy + r * sinf(a0 * DEG_TO_RAD);
    for (float a = a0 + 5; a < a1 + 5; a += 5) {
      if (a > a1) a = a1;   // 最後一段補到 a1,短弧才不會缺尾
      float x = cx + r * cosf(a * DEG_TO_RAD), y = cy + r * sinf(a * DEG_TO_RAD);
      cv.drawLine((int)px, (int)py, (int)x, (int)y, col); cv.drawLine((int)px, (int)py + 1, (int)x, (int)y + 1, col);
      px = x; py = y;
    }
  }
  static void ringArc(int l, int s, uint32_t col) { arc(CXR, CYR, R0 + l * DR, rot + s * 60 + 30 - HA, rot + s * 60 + 30 + HA, col); }

  // 火花
  struct Spark { float x, y, vx, vy, life; uint32_t col; } static sparks[24];
  static void spark(float x, float y, uint32_t col) {
    for (int n = 0; n < 6; n++) for (auto& p : sparks) if (p.life <= 0) {
      float a = frand() * 6.283f, v = 40 + frand() * 120;
      p = { x, y, cosf(a) * v, sinf(a) * v, 0.5f + frand() * 0.3f, col }; break;
    }
  }
  static void stepSparks(float dt) { for (auto& p : sparks) if (p.life > 0) { p.life -= dt; p.x += p.vx * dt; p.y += p.vy * dt; p.vy += 300 * dt; } }
  static void drawSparks() { for (auto& p : sparks) if (p.life > 0) cv.drawPixel((int)p.x, (int)p.y, p.col); }

  // 脫落的環片:以弧中點為樞軸的剛體,落到地板/堆上就凍住並把堆的高度圖抬高
  // ponytail: 堆積用每欄高度圖近似,片與片不互撞;要真堆疊再上 SAT 碰撞
  struct Piece { float px, py, vx, vy, rot, vrot, r; uint32_t col; bool live, rest; } static pcs[NPIECE];
  static int16_t floorY[W];
  static void reset() { rot = 0; memset(pcs, 0, sizeof pcs); memset(sparks, 0, sizeof sparks); for (auto& f : floorY) f = H - 1; }
  static void detach(int l, int s) {
    float am = (rot + s * 60 + 30) * DEG_TO_RAD, r = R0 + l * DR;
    for (auto& p : pcs) if (!p.live && !p.rest) {
      p = { CXR + r * cosf(am), CYR + r * sinf(am), cosf(am) * 140 + (frand() - 0.5f) * 40, sinf(am) * 140 - 40,
            rot + s * 60 + 210.0f, (frand() - 0.5f) * 180, r, segCol(s), true, false };
      return;
    }
  }
  static void pieceCenter(const Piece& p, float& cx, float& cy) { cx = p.px + p.r * cosf(p.rot * DEG_TO_RAD); cy = p.py + p.r * sinf(p.rot * DEG_TO_RAD); }
  static void stepPieces(float dt) {
    for (auto& p : pcs) if (p.live) {
      p.vy += G * dt; p.px += p.vx * dt; p.py += p.vy * dt; p.rot += p.vrot * dt;
      if (p.px < 0) { p.px = 0; p.vx = -p.vx * 0.5f; } if (p.px > W - 1) { p.px = W - 1; p.vx = -p.vx * 0.5f; }
      float cx, cy; pieceCenter(p, cx, cy);
      float pen = 0;   // 三個取樣點最深的穿入量
      for (int k = -1; k <= 1; k++) {
        float a = (p.rot + 180 + k * HA) * DEG_TO_RAD; int x = (int)(cx + p.r * cosf(a)); float y = cy + p.r * sinf(a);
        x = x < 0 ? 0 : x > W - 1 ? W - 1 : x; if (y - floorY[x] > pen) pen = y - floorY[x];
      }
      if (pen > 0) {   // 著地:抬起、凍住、更新高度圖
        p.py -= pen; p.live = false; p.rest = true; pieceCenter(p, cx, cy);
        for (float a = p.rot + 180 - HA; a <= p.rot + 180 + HA; a += 2) {
          int x = (int)(cx + p.r * cosf(a * DEG_TO_RAD)), y = (int)(cy + p.r * sinf(a * DEG_TO_RAD)) - 2;
          for (int dx = -1; dx <= 1; dx++) if (x + dx >= 0 && x + dx < W && y < floorY[x + dx]) floorY[x + dx] = y;
        }
        snd::note(150 + frand() * 60);
      }
    }
  }
  static void drawPieces() {
    for (auto& p : pcs) if (p.live || p.rest) { float cx, cy; pieceCenter(p, cx, cy); arc(cx, cy, p.r, p.rot + 180 - HA, p.rot + 180 + HA, p.col); }
  }
  static int pieceCount() { int n = 0; for (auto& p : pcs) n += p.live || p.rest; return n; }

  // 球:傾斜給重力、點螢幕踢球;撞到扇區 s 的牆(半徑由 wallOf 決定,<0 表示沒牆)就反彈並回傳 s
  static struct { float x, y, vx, vy; uint32_t col; } ball;
  static void spawnBall() { float a = frand() * 6.283f; ball = { (float)CXR, (float)CYR, cosf(a) * 160, sinf(a) * 160, rgb(255, 255, 255) }; }
  static int ballStep(const Ctx& c, float (*wallOf)(int)) {
    auto& b = ball;
    if (c.tap) { float dx = c.tx - b.x, dy = c.ty - b.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f; b.vx = dx / d * KICK; b.vy = dy / d * KICK; }
    b.vx += c.gx * G * c.dt; b.vy += c.gy * G * c.dt;
    float v = sqrtf(b.vx * b.vx + b.vy * b.vy); if (v < 80 && v > 1e-3f) { b.vx *= 80 / v; b.vy *= 80 / v; }   // 別停下來
    b.x += b.vx * c.dt; b.y += b.vy * c.dt;
    float dx = b.x - CXR, dy = b.y - CYR, d = sqrtf(dx * dx + dy * dy) + 1e-3f;
    float ang = fmodf(atan2f(dy, dx) * RAD_TO_DEG - rot + 720, 360);
    int s = (int)(ang / 60) % SEG; float wall = wallOf(s);
    if (wall < 0) { if (d > R0 + LAYERS * DR + 40) spawnBall(); return -1; }   // 這扇區破了,飛出去就重生
    wall -= BALL_R; float nx = dx / d, ny = dy / d, vn = b.vx * nx + b.vy * ny;
    if (d < wall || vn <= 0) return -1;
    b.x = CXR + nx * wall; b.y = CYR + ny * wall; b.vx -= 2 * vn * nx; b.vy -= 2 * vn * ny;
    b.col = segCol(s); spark(b.x, b.y, b.col); snd::note(NOTE[s]);
    return s;
  }
  static void drawBall() { cv.fillCircle((int)ball.x, (int)ball.y, BALL_R, ball.col); }
}
