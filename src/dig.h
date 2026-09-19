#pragma once
#include "ringlib.h"
#include "face.h"

// ================= 31. 每彈一次挖一塊 =================
// Minecraft 式的地層剖面:表土、石頭、深板岩,越深礦越多(煤 / 鐵 / 金 / 紅石 / 鑽石),有洞穴,底部是岩漿。
// 球一路往下鑽,每撞到一塊就敲掉,落地只彈一小下所以挖得很快;掉進洞穴就自由落體。
// 傾斜(或 A/C)輕推左右鑽歪,點螢幕往下猛砸。碰到岩漿就結束,分數是深度(層),前 5 名存 NVS
namespace dig {
  using ringlib::spark; using ringlib::stepSparks;
  constexpr int CS = 16, COLS = W / CS, ROWS = 32, SURF = 5, BALL_R = 5, LAVA_D = 60, BOTTOM_D = 68; constexpr float G = 900, BOUNCE = 130, SIDE = 350, SLAM = 650;
  enum { AIR, DIRT, STONE, DEEP, COAL, IRON, GOLD, REDSTONE, DIAMOND, LAVA };
  static uint8_t grid[ROWS][COLS]; static int genRow; static uint32_t caveMask;   // 環形緩衝:世界第 r 列存在 grid[r % ROWS];caveMask 是下一列要挖成洞穴的欄
  static float bx, by, bvx, bvy, camY, endT; static int depth, rank, broken, ores; static bool over, dead; static face::Mood mood;
  static Board board = { "dig" };
  static uint8_t& cell(int r, int c) { return grid[((r % ROWS) + ROWS) % ROWS][c]; }
  static uint8_t ore(int d) {   // 依深度抽礦
    float r = frand();
    if (d > 45 && r < 0.03f) return DIAMOND; if (d > 30 && r < 0.07f) return REDSTONE; if (d > 20 && r < 0.11f) return GOLD; if (d > 8 && r < 0.17f) return IRON; if (r < 0.22f) return COAL;
    return d > 38 ? DEEP : STONE;
  }
  static void genRows(int upto) {
    for (; genRow <= upto; genRow++) {
      int d = genRow - SURF;
      for (int c = 0; c < COLS; c++) cell(genRow, c) = d < 0 ? AIR : d < 3 ? DIRT : d >= BOTTOM_D ? LAVA : d >= LAVA_D && frand() < 0.5f ? LAVA : (caveMask >> c) & 1 ? AIR : ore(d);
      uint32_t next = 0;   // 洞穴:既有的洞往下延續(帶點左右漂),偶爾開新的
      if (d >= 4 && frand() < 0.75f) { int sh = (int)(frand() * 3) - 1; next = sh > 0 ? caveMask << 1 : sh < 0 ? caveMask >> 1 : caveMask; }
      if (d >= 4 && frand() < 0.15f) { int c0 = (int)(frand() * (COLS - 5)), len = 3 + (int)(frand() * 4); for (int c = c0; c < c0 + len; c++) next |= 1u << c; }
      caveMask = next & ((1u << COLS) - 1);
    }
  }
  void init() { genRow = 0; caveMask = 0; genRows(ROWS - 1); bx = W / 2; by = (SURF - 2) * CS; bvx = bvy = 0; camY = endT = 0; depth = broken = ores = 0; rank = -1; over = dead = false; mood = face::HAPPY; ringlib::reset(); cv.fillScreen(0); }
  static uint8_t at(float x, float y) { int c = (int)(x / CS), r = (int)floorf(y / CS); if (c < 0 || c >= COLS) return STONE; return r < 0 ? AIR : cell(r, c); }
  static void smash(float x, float y) {   // 敲掉一塊
    int c = (int)(x / CS), r = (int)floorf(y / CS); if (c < 0 || c >= COLS || r < 0) return;
    uint8_t& k = cell(r, c); if (k == AIR) return;
    static const uint8_t OC[10][3] = { {0,0,0},{140,95,55},{150,150,155},{80,85,95},{40,40,40},{215,180,150},{255,210,60},{255,60,60},{80,240,240},{255,120,0} };
    spark(x, y, rgb(OC[k][0], OC[k][1], OC[k][2]));
    if (k >= COAL) { ores++; snd::note(400 + k * 60); buzz(40, 15); } else snd::click();
    k = AIR; broken++;
    int d = r - SURF; if (d > depth) depth = d;
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); stepSparks(c.dt); return; }
    bvx += c.gx * SIDE * c.dt; bvx *= 0.9f; bvy += G * c.dt;
    if (c.tap) { bvy = fmaxf(bvy, SLAM); snd::note(300); }
    bx += bvx * c.dt; by += bvy * c.dt;
    uint8_t k;
    if ((k = at(bx, by + BALL_R)) != AIR) { if (k == LAVA) dead = true; else { smash(bx, by + BALL_R); by = floorf((by + BALL_R) / CS) * CS - BALL_R - 0.1f; bvy = -BOUNCE; } }   // 落地:敲掉、小彈一下
    if ((k = at(bx, by - BALL_R)) != AIR && k != LAVA) { smash(bx, by - BALL_R); by = ceilf((by - BALL_R) / CS) * CS + BALL_R + 0.1f; bvy = fabsf(bvy) * 0.3f; }
    if ((k = at(bx + BALL_R, by)) != AIR) { if (k == LAVA) dead = true; else { smash(bx + BALL_R, by); bx = floorf((bx + BALL_R) / CS) * CS - BALL_R - 0.1f; bvx = -fabsf(bvx) * 0.3f; } }
    if ((k = at(bx - BALL_R, by)) != AIR) { if (k == LAVA) dead = true; else { smash(bx - BALL_R, by); bx = ceilf((bx - BALL_R) / CS) * CS + BALL_R + 0.1f; bvx = fabsf(bvx) * 0.3f; } }
    if (bx < BALL_R) { bx = BALL_R; bvx = fabsf(bvx); } if (bx > W - BALL_R) { bx = W - BALL_R; bvx = -fabsf(bvx); }
    if (dead) { over = true; rank = board.record(depth); mood = face::DEAD; spark(bx, by, rgb(255, 120, 0)); snd::note(120); buzz(255, 400); }
    float target = by - H * 0.45f; if (target > camY) camY += (target - camY) * 6 * c.dt;   // 只往下捲
    genRows((int)((camY + H) / CS) + 2);
    if (!dead) mood = bvy > 500 ? face::SCARED : at(bx, by + CS * 2) == LAVA ? face::SCARED : face::HAPPY;
    stepSparks(c.dt);
  }
  void draw() {
    static const uint8_t BC[10][3] = { {0,0,0},{125,85,50},{125,125,130},{55,60,70},{125,125,130},{125,125,130},{125,125,130},{55,60,70},{55,60,70},{235,110,20} };   // 各種塊的底色
    static const uint8_t DC[10][3] = { {0,0,0},{0,0,0},{0,0,0},{0,0,0},{30,30,30},{215,180,150},{255,210,60},{255,60,60},{80,240,240},{255,200,60} };   // 礦的點
    int r0 = (int)floorf(camY / CS), r1 = r0 + H / CS + 2;
    cv.fillScreen(rgb(18, 18, 24));
    if (r0 < SURF) cv.fillRect(0, 0, W, SURF * CS - (int)camY, rgb(120, 180, 240));   // 天空
    for (int r = r0; r <= r1; r++) for (int c = 0; c < COLS; c++) {
      if (r < 0) continue; uint8_t k = cell(r, c); if (k == AIR) continue;
      int y = r * CS - (int)camY, d = r - SURF; auto& b = BC[k];
      uint32_t col = (k == STONE || k == COAL || k == IRON || k == GOLD) && d > 38 ? rgb(55, 60, 70) : rgb(b[0], b[1], b[2]);   // 深了礦長在深板岩上
      cv.fillRect(c * CS, y, CS - 1, CS - 1, col);
      if (k >= COAL && k < LAVA) { auto& o = DC[k]; uint32_t oc = rgb(o[0], o[1], o[2]); cv.fillRect(c * CS + 3, y + 4, 3, 3, oc); cv.fillRect(c * CS + 9, y + 8, 3, 3, oc); cv.fillRect(c * CS + 5, y + 11, 2, 2, oc); }
      if (k == LAVA) cv.fillRect(c * CS + 2, y + 5, 6, 2, rgb(255, 220, 80));
    }
    for (auto& p : ringlib::sparks) if (p.life > 0) cv.drawPixel((int)p.x, (int)(p.y - camY), p.col);
    int sy = (int)(by - camY); cv.drawCircle((int)bx, sy, BALL_R + 3, rgb(120, 90, 20)); cv.fillCircle((int)bx, sy, BALL_R, rgb(255, 210, 60)); face::draw(bx, sy, 1, 0, 0, 1, mood, 0.7f);
    char t[40]; snprintf(t, sizeof t, "depth %d  ores %d  best %u", depth, ores, board.best());
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(220, 220, 220), rgb(18, 18, 24)); cv.drawString(t, 4, 4);
    if (over) { snprintf(t, sizeof t, "DEPTH %d", depth); board.draw(t, rank); }
  }
}
