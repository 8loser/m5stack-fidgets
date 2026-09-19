#pragma once
#include "ringlib.h"
#include "face.h"

// ================= 37. 地城勇者 =================
// 磚牆地城,勇者是一顆撞球式的騎士彈珠:沒有重力、定速直線走,撞牆依入射角反射,撞到磚牆就把它敲掉(像打磚塊)。
// 傾斜只做轉向:有傾斜時行進方向慢慢轉向傾斜那邊,放平就保持直線;A/C 像方向盤,按住就把行進方向往左 / 往右轉。
// 點螢幕揮劍;靠近 Boss 時會自動揮劍。
// Boss 同樣定速反射地彈來彈去(慢一點、不挖牆),每 2.5 秒揮一次爪子,勇者在爪子範圍內就扣血;外型依樓層輪換(惡魔頭 / 史萊姆王 / 骷髏 / 小龍)。
// 小怪(史萊姆 / 蝙蝠 / 骷髏)在地城裡亂彈,靠近就撲咬;揮劍砍到就死,層數越高越多。小怪死了會掉愛心或金幣,
// 每殺 5 隻 ATK +1;Boss 一開始不在,這層小怪清光後才在地圖上某個空地現身。
// 地圖每次隨機:愛心(回血)、劍(攻擊+)、盾(減傷)、尖刺陷阱。砍到 Boss 血歸零進下一層(更強)。
// 勇者血歸零結束,分數是清了幾層,前 5 名存 NVS
namespace dungeon {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CS = 16, COLS = W / CS, ROWS = (H - 16) / CS, TOP = 16, MAXITEM = 11, MAXMON = 5, NITEM0 = 6, HERO_R = 6, BOSS_R = 13, NBOSS = 4; constexpr float SPEED = 85, BOSS_SPEED = 55, MON_SPEED = 45, TURN = 2.2f, AC_TURN = 3.5f, SWING_T = 0.25f, CLAW_CD = 2.5f, CLAW_T = 0.3f, CLAW_R = 30;   // TURN:傾到底每秒轉幾弧度
  enum { FLOOR, WALL }; enum { HEART, SWORD, SHIELD, TRAP, COIN }; enum { SLIME, BAT, SKEL };
  struct Mon { float x, y, vx, vy, hp, hitT, lungeT; int8_t kind; bool live; } static mon[MAXMON];
  static uint8_t grid[ROWS][COLS];
  struct Item { int8_t c, r, kind; bool live; float cd; } static item[MAXITEM];
  struct { float x, y, vx, vy, hp, hitT, swingT, swingA, autoT; int atk, def; } static hero;
  struct { float x, y, vx, vy, hp, maxHp, cd, clawT, hitT; int kind; bool live; } static boss;   // kind:依樓層輪換的 Boss 外型
  static int level, rank, kills, coins; static float endT, animT; static bool over;
  static Board board = { "dungeon" };
  static uint8_t& cell(int r, int c) { return grid[r < 0 ? 0 : r >= ROWS ? ROWS - 1 : r][c < 0 ? 0 : c >= COLS ? COLS - 1 : c]; }
  static bool solid(float x, float y) { int c = (int)(x / CS), r = (int)((y - TOP) / CS); if (c < 0 || c >= COLS || r < 0 || r >= ROWS) return true; return grid[r][c] != FLOOR; }
  static void carve(int c, int r, int w, int h) { for (int y = r; y < r + h; y++) for (int x = c; x < c + w; x++) if (x >= 0 && x < COLS && y >= 0 && y < ROWS) grid[y][x] = FLOOR; }
  static void genMap() {
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) grid[r][c] = frand() < 0.55f ? WALL : FLOOR;
    for (int k = 0; k < 4; k++) carve((int)(frand() * (COLS - 6)), (int)(frand() * (ROWS - 4)), 3 + (int)(frand() * 4), 2 + (int)(frand() * 3));   // 幾個房間
    carve(0, 0, 4, 4);   // 勇者起點
    int bc = -10, br = -10;   // 沒有 Boss 房了,下面幾行的距離條件用不到但留著不影響
    memset(item, 0, sizeof item);
    static const int8_t KINDS[NITEM0] = { HEART, HEART, SWORD, SHIELD, TRAP, TRAP };
    for (int i = 0; i < NITEM0; i++) { int c, r; do { c = 3 + (int)(frand() * (COLS - 4)); r = (int)(frand() * ROWS); } while (abs(c - bc) < 4 && abs(r - br) < 4); grid[r][c] = FLOOR; item[i] = { (int8_t)c, (int8_t)r, KINDS[i], true, 0 }; }
    boss.live = false;   // 小怪清光才出現
    memset(mon, 0, sizeof mon);
    for (int i = 0; i < MAXMON && i < 2 + level; i++) { int c, r; do { c = 5 + (int)(frand() * (COLS - 8)); r = (int)(frand() * ROWS); } while (grid[r][c] != FLOOR || (abs(c - bc) < 4 && abs(r - br) < 4)); float a = frand() * 6.283f; mon[i] = { c * CS + CS / 2.0f, TOP + r * CS + CS / 2.0f, cosf(a), sinf(a), 2, 0, -1, (int8_t)((int)(frand() * 3) % 3), true }; }
    hero.x = 2 * CS; hero.y = TOP + 2 * CS; hero.vx = 120; hero.vy = -100;
  }
  void init() { level = 0; rank = -1; kills = coins = 0; over = false; endT = animT = 0; hero.hp = 100; hero.atk = 1; hero.def = 0; hero.hitT = hero.swingT = hero.autoT = 0; genMap(); ringlib::reset(); cv.fillScreen(0); }
  static void hurtHero(float dmg, uint32_t col) { if (hero.hitT > 0) return; hero.hitT = 0.6f; hero.hp -= fmaxf(1, dmg - hero.def); spark(hero.x, hero.y, col); snd::note(180); buzz(120, 60); if (hero.hp <= 0) { hero.hp = 0; over = true; rank = board.record(level); snd::note(100); buzz(255, 400); } }
  // 撞球式:定速直線,傾斜把方向往傾斜那邊轉,撞牆與磚依軸反射;dig 為真時磚牆會被敲掉
  static void bounce(float& x, float& y, float& vx, float& vy, float r, float speed, bool dig, const Ctx& c, bool steer) {
    if (steer && (fabsf(c.gx) > 0.15f || fabsf(c.gy) > 0.15f)) {   // 目前方向與傾斜方向的夾角,依 TURN 慢慢轉過去
      float want = atan2f(c.gy, c.gx), have = atan2f(vy, vx), d = want - have; while (d > 3.1416f) d -= 6.2832f; while (d < -3.1416f) d += 6.2832f;
      float amt = fminf(1, sqrtf(c.gx * c.gx + c.gy * c.gy)), step = TURN * amt * c.dt; if (fabsf(d) < step) step = fabsf(d); have += d > 0 ? step : -step; vx = cosf(have); vy = sinf(have);
    }
    if (steer && (c.btnA || c.btnC)) { float a = atan2f(vy, vx) + (c.btnC - c.btnA) * AC_TURN * c.dt; vx = cosf(a); vy = sinf(a); }   // A/C:方向盤
    setSpeed(vx, vy, speed);
    x += vx * c.dt; y += vy * c.dt;
    static const int8_t DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
    for (int k = 0; k < 4; k++) {
      float sx = x + DX[k] * r, sy = y + DY[k] * r; if (!solid(sx, sy)) continue;
      int cc = (int)floorf(sx / CS), rr = (int)floorf((sy - TOP) / CS);
      if (dig && cc >= 0 && cc < COLS && rr >= 0 && rr < ROWS && cell(rr, cc) == WALL) { cell(rr, cc) = FLOOR; spark(sx, sy, rgb(160, 110, 80)); snd::click(); }   // 只能敲磚,鐵欄敲不掉
      if (DX[k]) { x = DX[k] > 0 ? cc * CS - r - 0.5f : (cc + 1) * CS + r + 0.5f; vx = DX[k] > 0 ? -fabsf(vx) : fabsf(vx); }
      else { y = DY[k] > 0 ? TOP + rr * CS - r - 0.5f : TOP + (rr + 1) * CS + r + 0.5f; vy = DY[k] > 0 ? -fabsf(vy) : fabsf(vy); }
    }
    if (x < r) { x = r; vx = fabsf(vx); } if (x > W - r) { x = W - r; vx = -fabsf(vx); }
    if (y < TOP + r) { y = TOP + r; vy = fabsf(vy); } if (y > H - r) { y = H - r; vy = -fabsf(vy); }
    if (fabsf(vy) < speed * 0.15f) vy = (vy < 0 ? -1 : 1) * speed * 0.15f;   // 別變成純水平來回
    if (fabsf(vx) < speed * 0.15f) vx = (vx < 0 ? -1 : 1) * speed * 0.15f;
  }
  static void spawnBoss() {   // 小怪清光:在離勇者遠一點的空地清出 3x3 現身
    int c, r, tries = 0; do { c = 2 + (int)(frand() * (COLS - 4)); r = 2 + (int)(frand() * (ROWS - 4)); } while (++tries < 50 && (fabsf(c * CS - hero.x) < 80 && fabsf(TOP + r * CS - hero.y) < 80));
    carve(c - 1, r - 1, 3, 3);
    boss = { c * CS + CS / 2.0f, TOP + r * CS + CS / 2.0f, (frand() < 0.5f ? -1 : 1) * 90.0f, -100, 60.0f + level * 30, 60.0f + level * 30, CLAW_CD, 0, 0, level % NBOSS, true };
    for (int k = 0; k < 8; k++) spark(boss.x, boss.y, rgb(200, 60, 60)); snd::note(150); buzz(150, 120);
  }
  static void kill(const Mon& m) {   // 掉落(六成:三成愛心、三成金幣)、每 5 殺 ATK +1、清光就開 Boss 房的欄杆
    float r = frand(); if (r < 0.6f) for (auto& it : item) if (!it.live) { it = { (int8_t)(m.x / CS), (int8_t)((m.y - TOP) / CS), (int8_t)(r < 0.3f ? HEART : COIN), true, 0 }; break; }
    if (++kills % 5 == 0) { hero.atk++; snd::note(900); buzz(60, 30); }
    bool any = false; for (auto& o : mon) any |= o.live;
    if (!any && !boss.live) spawnBoss();
  }
  static float bossDist2() { float dx = hero.x - boss.x, dy = hero.y - boss.y; return dx * dx + dy * dy; }
  static void swing() {
    hero.swingT = SWING_T; hero.swingA = atan2f(boss.y - hero.y, boss.x - hero.x); snd::note(500); buzz(20, 10);
    for (auto& m : mon) if (m.live) { float dx = m.x - hero.x, dy = m.y - hero.y; if (dx * dx + dy * dy < 24 * 24) { m.hp -= hero.atk; m.hitT = 0.3f; spark(m.x, m.y, rgb(255, 255, 255)); if (m.hp <= 0) { m.live = false; snd::note(650); kill(m); } } }
    if (boss.live && bossDist2() < (BOSS_R + 24) * (BOSS_R + 24)) { boss.hp -= hero.atk * 5; boss.hitT = 0.25f; spark(boss.x, boss.y, rgb(255, 80, 80)); snd::note(300);
      if (boss.hp <= 0) { boss.live = false; level++; buzz(200, 200); snd::note(800); for (int k = 0; k < 6; k++) spark(boss.x, boss.y, hsv(frand())); genMap(); } }
  }
  void step(const Ctx& c) {
    animT += c.dt;
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); stepSparks(c.dt); return; }
    hero.hitT -= c.dt; hero.swingT -= c.dt; hero.autoT -= c.dt;
    bounce(hero.x, hero.y, hero.vx, hero.vy, HERO_R, SPEED, true, c, true);
    if (boss.live) {
      bounce(boss.x, boss.y, boss.vx, boss.vy, BOSS_R, BOSS_SPEED, false, c, false); boss.hitT -= c.dt; boss.clawT -= c.dt;
      float dx = hero.x - boss.x, dy = hero.y - boss.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f, rr = HERO_R + BOSS_R;
      if (d < rr) { hero.x = boss.x + dx / d * rr; hero.y = boss.y + dy / d * rr; float vn = hero.vx * dx / d + hero.vy * dy / d; if (vn < 0) { hero.vx -= 2 * vn * dx / d; hero.vy -= 2 * vn * dy / d; } }   // 撞到 Boss 反彈
      if ((boss.cd -= c.dt) <= 0) { boss.cd = fmaxf(1.2f, CLAW_CD - level * 0.2f); boss.clawT = CLAW_T; snd::note(220); if (d < CLAW_R + HERO_R) hurtHero(14 + level * 2, rgb(255, 80, 80)); }   // 揮爪
      if (d < BOSS_R + 24 && hero.autoT <= 0) { hero.autoT = 0.5f; swing(); }   // 靠近就自動揮劍
    }
    for (auto& m : mon) if (m.live) {   // 小怪:亂彈,靠近勇者就撲咬
      m.hitT -= c.dt; m.lungeT -= c.dt;
      float dx = hero.x - m.x, dy = hero.y - m.y, d2 = dx * dx + dy * dy;
      if (d2 < 45 * 45 && m.lungeT <= -1.2f) { m.lungeT = 0.35f; float d = sqrtf(d2) + 1e-3f; m.vx = dx / d; m.vy = dy / d; }
      bounce(m.x, m.y, m.vx, m.vy, 5, m.lungeT > 0 ? 200 : MON_SPEED, false, c, false);
      if (d2 < (HERO_R + 6) * (HERO_R + 6)) { hurtHero(m.kind == SKEL ? 10 : 7, rgb(200, 80, 200)); m.vx = -dx; m.vy = -dy; }
      if (d2 < 22 * 22 && hero.autoT <= 0) { hero.autoT = 0.5f; swing(); }
    }
    if (c.tap && hero.swingT <= -0.1f) swing();
    for (auto& it : item) if (it.live) {   // 道具:碰到就撿,陷阱是踩到扣血
      it.cd -= c.dt; float ix = it.c * CS + CS / 2.0f, iy = TOP + it.r * CS + CS / 2.0f, dx = hero.x - ix, dy = hero.y - iy; if (dx * dx + dy * dy > 11 * 11) continue;
      if (it.kind == COIN) { coins++; it.live = false; snd::note(900); }
      else if (it.kind == HEART) { hero.hp = fminf(100, hero.hp + 30); it.live = false; snd::note(750); } else if (it.kind == SWORD) { hero.atk++; it.live = false; snd::note(600); } else if (it.kind == SHIELD) { hero.def += 3; it.live = false; snd::note(400); }
      else if (it.cd <= 0) { it.cd = 1; hurtHero(12, rgb(220, 220, 220)); }
      if (!it.live) { spark(ix, iy, rgb(255, 230, 120)); buzz(40, 15); }
    }
    stepSparks(c.dt);
  }
  static void drawShield(int x, int y, int r) {   // 盾牌:上寬圓肩、下尖,深藍底金邊、中間十字
    uint32_t fill = rgb(50, 90, 200), edge = rgb(230, 190, 70);
    cv.fillRect(x - r, y - r, r * 2 + 1, r, fill); cv.fillCircle(x - r + 2, y - r + 2, 2, fill); cv.fillCircle(x + r - 2, y - r + 2, 2, fill);
    cv.fillTriangle(x - r, y, x + r, y, x, y + r + 3, fill);
    cv.drawLine(x - r, y - r + 2, x - r, y, edge); cv.drawLine(x + r, y - r + 2, x + r, y, edge); cv.drawLine(x - r, y, x, y + r + 3, edge); cv.drawLine(x + r, y, x, y + r + 3, edge); cv.drawFastHLine(x - r + 2, y - r, r * 2 - 3, edge);
    cv.drawFastVLine(x, y - r + 2, r * 2, edge); cv.drawFastHLine(x - r + 2, y - 1, r * 2 - 3, edge);
  }
  // 勇者:照 Dwarf Fortress(Steam 版 tileset)的矮人畫:16 px 見方的正面像,鋼盔壓到眉毛、方形大鬍子蓋住嘴到胸口、
  // 有腰帶的短袍、短腿靴子,右手一把戰斧;受傷整隻閃白
  static void drawHero(int x, int y, bool flash) {
    uint32_t W_ = rgb(255, 255, 255), steel = flash ? W_ : rgb(140, 145, 160), steelD = rgb(70, 72, 85), skin = flash ? W_ : rgb(235, 190, 145), beard = flash ? W_ : rgb(150, 85, 40), beardD = rgb(100, 55, 25),
             tunic = flash ? W_ : rgb(60, 110, 190), belt = rgb(70, 45, 25), boot = rgb(45, 35, 30), wood = rgb(120, 80, 40), eye = rgb(30, 30, 40);
    cv.fillRect(x - 6, y - 10, 13, 6, steel); cv.fillRect(x - 7, y - 6, 15, 2, steelD); cv.fillRect(x - 4, y - 11, 9, 1, steel);   // 鋼盔:圓頂、帽沿
    cv.fillRect(x - 5, y - 4, 11, 3, skin); cv.fillRect(x - 4, y - 4, 2, 2, eye); cv.fillRect(x + 3, y - 4, 2, 2, eye); cv.drawPixel(x, y - 2, beardD);   // 眉下的臉、眼、鼻
    cv.fillRect(x - 6, y - 1, 13, 6, beard); cv.fillRect(x - 5, y + 5, 11, 2, beard); cv.fillRect(x - 3, y + 7, 7, 1, beardD); cv.drawFastVLine(x - 2, y, 6, beardD); cv.drawFastVLine(x + 2, y, 6, beardD);   // 方形大鬍子、辮子紋
    cv.fillRect(x - 5, y + 3, 4, 5, tunic); cv.fillRect(x + 2, y + 3, 4, 5, tunic); cv.fillRect(x - 5, y + 8, 11, 2, belt); cv.drawPixel(x, y + 8, rgb(230, 190, 70));   // 短袍露在鬍子兩側、腰帶扣
    cv.fillRect(x - 4, y + 10, 3, 2, boot); cv.fillRect(x + 2, y + 10, 3, 2, boot);   // 靴子
    cv.fillRect(x + 7, y - 6, 2, 14, wood); cv.fillRect(x + 6, y - 9, 6, 5, steel); cv.drawFastVLine(x + 11, y - 9, 5, steelD);   // 右手戰斧
  }
  void draw() {
    static const uint32_t FL = rgb(38, 36, 44), FL2 = rgb(44, 42, 52), BRICK = rgb(120, 70, 55), MORTAR = rgb(70, 40, 35);
    cv.fillScreen(rgb(20, 18, 24));
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) {
      int x = c * CS, y = TOP + r * CS;
      if (grid[r][c] == FLOOR) cv.fillRect(x, y, CS, CS, (r + c) & 1 ? FL : FL2);
      else { cv.fillRect(x, y, CS, CS, MORTAR); cv.fillRect(x + 1, y + 1, 6, 6, BRICK); cv.fillRect(x + 9, y + 1, 6, 6, BRICK); cv.fillRect(x + 1, y + 9, 2, 6, BRICK); cv.fillRect(x + 5, y + 9, 6, 6, BRICK); cv.fillRect(x + 13, y + 9, 2, 6, BRICK); }   // 磚
    }
    for (auto& it : item) if (it.live) {
      int x = it.c * CS + 8, y = TOP + it.r * CS + 8;
      if (it.kind == HEART) { cv.fillCircle(x - 3, y - 2, 3, rgb(255, 70, 110)); cv.fillCircle(x + 3, y - 2, 3, rgb(255, 70, 110)); cv.fillTriangle(x - 6, y, x + 6, y, x, y + 6, rgb(255, 70, 110)); }
      else if (it.kind == SWORD) { cv.drawLine(x - 5, y + 5, x + 5, y - 5, rgb(240, 240, 250)); cv.drawLine(x - 4, y + 6, x + 6, y - 4, rgb(180, 180, 200)); cv.drawLine(x - 6, y + 1, x - 1, y + 6, rgb(200, 160, 60)); }
      else if (it.kind == SHIELD) drawShield(x, y, 7);
      else if (it.kind == COIN) { cv.fillCircle(x, y, 5, rgb(240, 200, 60)); cv.drawCircle(x, y, 5, rgb(160, 120, 20)); cv.drawFastVLine(x, y - 2, 5, rgb(160, 120, 20)); }
      else { for (int k = -1; k <= 1; k++) cv.fillTriangle(x + k * 5 - 2, y + 6, x + k * 5 + 2, y + 6, x + k * 5, y - 5, rgb(200, 200, 210)); }
    }
    for (auto& m : mon) if (m.live) {   // 小怪
      int mx = (int)m.x, my = (int)m.y; uint32_t col = m.hitT > 0 ? rgb(255, 255, 255) : m.kind == SLIME ? rgb(80, 200, 90) : m.kind == BAT ? rgb(150, 80, 200) : rgb(225, 225, 210);
      if (m.kind == SLIME) { cv.fillCircle(mx, my + 1, 6, col); cv.fillRect(mx - 6, my + 2, 12, 4, col); }
      else if (m.kind == BAT) { cv.fillCircle(mx, my, 4, col); int w = (int)(sinf(animT * 20) * 3); cv.fillTriangle(mx - 3, my, mx - 10, my - 3 + w, mx - 8, my + 3, col); cv.fillTriangle(mx + 3, my, mx + 10, my - 3 + w, mx + 8, my + 3, col); }
      else { cv.fillCircle(mx, my - 2, 5, col); cv.fillRect(mx - 3, my + 3, 6, 5, col); cv.drawFastHLine(mx - 4, my + 5, 8, rgb(120, 120, 110)); cv.fillRect(mx - 3, my - 3, 2, 2, 0); cv.fillRect(mx + 1, my - 3, 2, 2, 0); }
      if (m.kind != SKEL) face::draw(m.x, m.y - 1, 1, 0, 0, 1, m.lungeT > 0 ? face::ANNOYED : face::CALM, 0.5f);
      if (m.lungeT > 0) { cv.drawLine(mx - 4, my + 5, mx - 2, my + 8, rgb(255, 255, 255)); cv.drawLine(mx + 4, my + 5, mx + 2, my + 8, rgb(255, 255, 255)); }   // 撲咬:露牙
    }
    if (boss.live) {   // Boss 外型依樓層輪換;揮爪時兩側畫爪痕
      int bx = (int)boss.x, by = (int)boss.y; bool flash = boss.hitT > 0, charge = boss.cd < 0.4f;
      switch (boss.kind) {
        case 0: {   // 惡魔頭:角、紅臉、獠牙
          uint32_t bc = flash ? rgb(255, 255, 255) : charge ? rgb(230, 90, 60) : rgb(170, 40, 40);
          cv.fillTriangle(bx - 12, by - 6, bx - 6, by - 16, bx - 4, by - 4, rgb(230, 220, 200)); cv.fillTriangle(bx + 12, by - 6, bx + 6, by - 16, bx + 4, by - 4, rgb(230, 220, 200));
          cv.fillCircle(bx, by, BOSS_R, bc); cv.fillCircle(bx - 5, by - 3, 3, rgb(255, 240, 80)); cv.fillCircle(bx + 5, by - 3, 3, rgb(255, 240, 80)); cv.fillCircle(bx - 5, by - 3, 1, 0); cv.fillCircle(bx + 5, by - 3, 1, 0);
          cv.fillRect(bx - 7, by + 4, 14, 3, rgb(40, 0, 0)); for (int k = -6; k <= 6; k += 4) cv.fillTriangle(bx + k - 1, by + 4, bx + k + 1, by + 4, bx + k, by + 8, rgb(240, 240, 240)); break; }
        case 1: {   // 史萊姆王:綠色大果凍加皇冠
          uint32_t bc = flash ? rgb(255, 255, 255) : charge ? rgb(150, 240, 120) : rgb(70, 190, 90);
          cv.fillCircle(bx, by + 2, BOSS_R, bc); cv.fillRect(bx - BOSS_R, by + 4, BOSS_R * 2, 8, bc); cv.fillCircle(bx - 4, by - 2, 2, 0); cv.fillCircle(bx + 4, by - 2, 2, 0); cv.drawLine(bx - 4, by + 5, bx + 4, by + 5, 0);
          cv.fillRect(bx - 8, by - 16, 16, 5, rgb(255, 210, 40)); for (int k = -6; k <= 6; k += 6) cv.fillTriangle(bx + k - 3, by - 16, bx + k + 3, by - 16, bx + k, by - 22, rgb(255, 210, 40)); break; }
        case 2: {   // 骷髏王:白骨頭、黑眼窩、下巴
          uint32_t bc = flash ? rgb(255, 255, 255) : charge ? rgb(255, 230, 200) : rgb(225, 225, 210);
          cv.fillCircle(bx, by - 2, BOSS_R - 1, bc); cv.fillRect(bx - 7, by + 6, 14, 7, bc); cv.fillCircle(bx - 5, by - 3, 4, rgb(20, 0, 0)); cv.fillCircle(bx + 5, by - 3, 4, rgb(20, 0, 0)); cv.fillCircle(bx - 5, by - 3, 1, rgb(255, 60, 60)); cv.fillCircle(bx + 5, by - 3, 1, rgb(255, 60, 60));
          for (int k = -6; k <= 6; k += 3) cv.drawFastVLine(bx + k, by + 6, 6, rgb(120, 120, 110)); cv.fillTriangle(bx - 2, by + 1, bx + 2, by + 1, bx, by + 4, rgb(120, 120, 110)); break; }
        default: {   // 小龍:綠身、翅膀、角、噴火口
          uint32_t bc = flash ? rgb(255, 255, 255) : charge ? rgb(120, 230, 90) : rgb(60, 150, 70); int w = (int)(sinf(animT * 8) * 4);
          cv.fillTriangle(bx - 10, by - 2, bx - 26, by - 12 + w, bx - 22, by + 4, bc); cv.fillTriangle(bx + 10, by - 2, bx + 26, by - 12 + w, bx + 22, by + 4, bc);
          cv.fillCircle(bx, by, BOSS_R, bc); cv.fillTriangle(bx - 9, by - 8, bx - 6, by - 18, bx - 3, by - 9, rgb(230, 220, 200)); cv.fillTriangle(bx + 9, by - 8, bx + 6, by - 18, bx + 3, by - 9, rgb(230, 220, 200));
          cv.fillCircle(bx - 5, by - 3, 3, rgb(255, 200, 40)); cv.fillCircle(bx + 5, by - 3, 3, rgb(255, 200, 40)); cv.fillRect(bx - 6, by - 4, 2, 3, 0); cv.fillRect(bx + 4, by - 4, 2, 3, 0);
          cv.fillRect(bx - 6, by + 4, 12, 4, rgb(30, 60, 30)); if (charge) cv.fillCircle(bx, by + 10, 4, rgb(255, 140, 0)); break; }
      }
      if (boss.clawT > 0) { float t = 1 - boss.clawT / CLAW_T; for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k++) { float a = (s > 0 ? 0 : 3.1416f) + (t - 0.5f) * 2.0f * s + k * 0.25f; cv.drawLine(bx + (int)(cosf(a) * 14), by + (int)(sinf(a) * 14), bx + (int)(cosf(a) * CLAW_R), by + (int)(sinf(a) * CLAW_R), rgb(255, 230, 200)); } }
      cv.fillRect(bx - 14, by - 22, 28, 3, rgb(60, 20, 20)); cv.fillRect(bx - 14, by - 22, (int)(28 * boss.hp / boss.maxHp), 3, rgb(230, 50, 50));
    }
    drawHero((int)hero.x, (int)hero.y, hero.hitT > 0.4f);
    if (hero.swingT > 0) {   // 揮劍:劍繞著勇者從 -70 度掃到 +70 度,劍身 + 護手 + 柄
      float t = 1 - hero.swingT / SWING_T, a = hero.swingA - 1.2f + t * 2.4f, ca = cosf(a), sa = sinf(a), px = -sa, py = ca;
      int hx = (int)(hero.x + ca * 7), hy = (int)(hero.y + sa * 7), gx = (int)(hero.x + ca * 10), gy = (int)(hero.y + sa * 10), tx = (int)(hero.x + ca * 24), ty = (int)(hero.y + sa * 24);
      cv.drawLine(hx, hy, gx, gy, rgb(120, 80, 40)); cv.drawLine((int)(gx - px * 4), (int)(gy - py * 4), (int)(gx + px * 4), (int)(gy + py * 4), rgb(220, 180, 60));
      cv.drawLine(gx, gy, tx, ty, rgb(240, 240, 250)); cv.drawLine((int)(gx + px), (int)(gy + py), (int)(tx + px), (int)(ty + py), rgb(170, 170, 190));
      for (int k = 1; k <= 3; k++) { float b2 = a - k * 0.25f; cv.drawPixel((int)(hero.x + cosf(b2) * 22), (int)(hero.y + sinf(b2) * 22), rgb(200, 200, 230)); }   // 殘影
    }
    drawSparks();
    cv.fillRect(0, 0, W, TOP, rgb(20, 18, 24));   // HUD:血條、攻防、層數
    cv.fillCircle(8, 8, 3, rgb(255, 70, 110)); cv.fillRect(16, 4, 80, 8, rgb(60, 30, 30)); cv.fillRect(16, 4, (int)(80 * hero.hp / 100), 8, rgb(220, 50, 70));
    char t[40]; snprintf(t, sizeof t, "ATK %d DEF %d  F%d  $%d", hero.atk, hero.def, level + 1, coins);
    cv.setTextDatum(middle_left); cv.setTextSize(1); cv.setTextColor(rgb(220, 210, 180), rgb(20, 18, 24)); cv.drawString(t, 104, 8);
    if (over) { snprintf(t, sizeof t, "FLOORS %d", level); board.draw(t, rank); }
  }
}
