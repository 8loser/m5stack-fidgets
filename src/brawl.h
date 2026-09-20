#pragma once
#include "ringlib.h"

// ================= 38. 彈珠勇者大戰 =================
// 取自「24 MARBLES MUST DEFEND THE MINECRAFT FARM」:去掉任務與農場保護,只留兩批彈珠對打。
// 8 色勇者與一批怪(殭屍 / 骷髏 / 苦力怕)加一隻 Boss(凋零怪)各從一個小空房出發,其餘全是 Minecraft 式的方塊層:
// 木板敲兩下、石頭三下、礦(煤 / 鐵 / 金 / 鑽石)四下、鐵欄敲不掉、TNT 敲到就炸(方塊與雙方都炸)、寶箱敲開掉武器(劍 / 弓 / 長矛)或蘋果。
// 方塊、怪、道具都是 8x8 像素圖放大兩倍畫的(照 Minecraft 的樣子手打),init 時先畫進小畫布,每幀直接貼
// 勇者是 Steve 式的方頭(頭髮是那顆的顏色),撞球式定速反射,傾斜(或 A/C)把所有勇者的方向轉向傾斜那邊,點螢幕全部朝手指衝;撞到方塊就敲。
// 武器繞著勇者轉(每人最多 4 把):劍掃到怪就扣血、弓每 1.5 秒朝最近的怪射箭、長矛比劍長、傷害高。勇者撞到怪兩邊都反彈但不扣血,只有被攻擊才扣。
// 怪也會挖方塊:殭屍貼著每 1.2 秒咬一口、骷髏射箭、苦力怕貼上來就爆(連方塊一起炸掉)、Boss 追著勇者每 2 秒揮一次(30 px 內都中)。
// 怪清光就結算這一波誰還活著,接著下一波(死掉的顏色不回來、地圖重生、怪更多更硬);勇者全滅結束,分數是撐了幾波,前 5 名存 NVS
namespace brawl {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CS = 16, COLS = W / CS, ROWS = (H - 16) / CS, TOP = 16, NH = 8, MAXMOB = 10, MAXW = 4, MAXARROW = 24, MAXITEM = 10, HERO_R = 7, MOB_R = 7, BOSS_R = 12;
  constexpr float SPEED = 80, MOB_SPEED = 35, TURN = 2.2f, AC_TURN = 3.5f, HP0 = 30, SHOW_T = 2.5f;
  enum { FLOOR, WOOD, STONE, IRON, CHEST, TNT, COAL, IRONORE, GOLD, DIAMOND, NTILE }; enum { SWORD = 1, BOW, SPEAR, APPLE }; enum { ZOMBIE, SKEL, CREEPER, BOSS };
  static uint8_t grid[ROWS][COLS], thp[ROWS][COLS];   // thp:方塊還要敲幾下
  struct Hero { float x, y, vx, vy, hp, hitT, bowT; int8_t w[MAXW]; bool live; } static hero[NH];
  struct Mob { float x, y, vx, vy, hp, maxHp, hitT, cd, fuse; int8_t kind; bool live; } static mob[MAXMOB];
  struct Arrow { float x, y, vx, vy, life; bool live, mine; } static arrow[MAXARROW];
  struct Item { int8_t c, r, kind; bool live; } static item[MAXITEM];
  static int wave, rank, alive; static float animT, endT, showT; static bool over;
  static Board board = { "brawl" };
  static uint32_t col(int i) { return hsv(i / (float)NH); }
  static uint8_t hits(uint8_t k) { return k == WOOD ? 2 : k == STONE ? 3 : k >= COAL ? 4 : 1; }
  static bool solid(float x, float y) { int c = (int)(x / CS), r = (int)((y - TOP) / CS); if (c < 0 || c >= COLS || r < 0 || r >= ROWS) return true; return grid[r][c] != FLOOR; }
  static void dropItem(int c, int r, int kind) { for (auto& it : item) if (!it.live) { it = { (int8_t)c, (int8_t)r, (int8_t)kind, true }; return; } }
  static void explodeAt(float x, float y, bool hurtMobs);
  static void breakTile(int c, int r) {   // 方塊碎掉;寶箱掉武器或愛心
    if (c < 0 || c >= COLS || r < 0 || r >= ROWS || grid[r][c] == FLOOR) return;
    uint8_t k = grid[r][c]; grid[r][c] = FLOOR; spark(c * CS + CS / 2, TOP + r * CS + CS / 2, k == WOOD ? rgb(170, 120, 60) : k == CHEST ? rgb(255, 220, 90) : rgb(150, 150, 155));
    if (k == CHEST) { float p = frand(); dropItem(c, r, p < 0.3f ? APPLE : p < 0.55f ? SWORD : p < 0.8f ? BOW : SPEAR); snd::note(700); }
    else if (k == TNT) explodeAt(c * CS + CS / 2.0f, TOP + r * CS + CS / 2.0f, true);
    else snd::click();
  }
  // ---- 像素圖:'.' 透明,其他字母查 pal();init 時各畫進一張小畫布(8-bit),每幀用 pushSprite 貼、洋紅當透明色 ----
  static uint32_t pal(char ch) {
    switch (ch) {
      case 'K': return rgb(20, 20, 20);    case 'W': return rgb(255, 255, 255); case 'u': return rgb(75, 75, 80);     case 'g': return rgb(70, 150, 70);   case 'G': return rgb(40, 95, 40);
      case 'c': return rgb(90, 200, 80);   case 's': return rgb(215, 215, 205); case 'd': return rgb(120, 120, 110);  case 't': return rgb(130, 130, 135); case 'T': return rgb(95, 95, 100);
      case 'w': return rgb(165, 120, 65);  case 'v': return rgb(120, 85, 40);   case 'e': return rgb(120, 85, 58);    case 'E': return rgb(95, 65, 42);    case 'B': return rgb(80, 55, 35);
      case 'i': return rgb(200, 200, 210); case 'y': return rgb(255, 210, 60);  case 'D': return rgb(80, 240, 240);   case 'x': return rgb(40, 40, 40);    case 'n': return rgb(215, 180, 150);
      case 'r': return rgb(220, 40, 40);   default: return rgb(255, 0, 255);
    }
  }
  static const char* const SPR[][12] = {
    { "........", "........", "........", "........", "........", "........", "........", "........" },   // FLOOR(不用圖,drawTile 直接畫泥土)
    { "wwwwwwww", "wwwvwwww", "vvvvvvvv", "wwwwwwvw", "wwwwwwww", "vvvvvvvv", "wvwwwwww", "wwwwwwww" },   // WOOD 木板
    { "tttttttt", "tTTttttt", "ttttttTt", "tttttTTt", "tTtttttt", "ttttTttt", "ttTTtttt", "tttttttt" },   // STONE
    { "KiKKiKKi", "KiKKiKKi", "iiiiiiii", "KiKKiKKi", "KiKKiKKi", "iiiiiiii", "KiKKiKKi", "KiKKiKKi" },   // IRON 鐵欄
    { "BBBBBBBB", "BwwwwwwB", "BwwiiwwB", "BBBiiBBB", "BwwiiwwB", "BwwwwwwB", "BwwwwwwB", "BBBBBBBB" },   // CHEST
    { "rrrrrrrr", "rrrrrrrr", "WWWWWWWW", "WKKWKWKW", "WWWWWWWW", "rrrrrrrr", "rrrrrrrr", "rrrrrrrr" },   // TNT
    { "tttttttt", "t**ttttt", "t**tt**t", "ttttt**t", "tt**tttt", "tt**tt*t", "ttttttt*", "tttttttt" },   // 礦('*' 換成礦的顏色)
    { "GGGGGGGG", "GggggggG", "gggggggg", "gKKggKKg", "gggggggg", "ggGGGGgg", "ggGggGgg", "gggggggg" },   // ZOMBIE
    { "ssssssss", "ssssssss", "sKKssKKs", "sKKssKKs", "ssssssss", "sssKKsss", "sdsdsdsd", "ssssssss" },   // SKEL
    { "cccccccc", "cccccccc", "cKKccKKc", "cKKccKKc", "cccKKccc", "ccKKKKcc", "ccKKKKcc", "ccKccKcc" },   // CREEPER
    { "...uuuuuu...", "...uuuuuu...", "uuuuWuuWuuuu", "uWuuuuuuuuWu", "uuuuuKKuuuuu", "uuuuuuuuuuuu", ".K..KKKK..K.", "..KKKKKKKK..", "....KKKK....", "...KKKKKK...", "....KKKK....", ".....KK....." },   // 凋零怪 12x12
    { "....B...", "...BB...", "..rrrr..", ".rrrrrr.", ".rWrrrr.", ".rrrrrr.", "..rrrr..", "........" },   // APPLE
    { "......ii", ".....ii.", "....ii..", ".y.ii...", "..yy....", ".yBy....", "B.......", "........" },   // SWORD
    { "..BBBBBW", ".B....W.", "B....W..", "B...W...", "v..W....", "B.W.....", "BW......", "W......." },   // BOW(弓背在左上、弦是斜線,像 Minecraft 的圖示)
    { "......ii", ".....ii.", "....iB..", "...B....", "..B.....", ".B......", "B.......", "........" },   // SPEAR
    { "********", "********", "*nnnnnn*", "nnnnnnnn", "nWKnnKWn", "nnnBBnnn", "nBnnnnBn", "nnBBBBnn" },   // 勇者的頭(Steve 式,'*' 是那顆的顏色當頭髮)
  };
  enum { S_ORE = COAL, S_ZOMBIE = COAL + 1, S_SKEL, S_CREEPER, S_WITHER, S_APPLE, S_SWORD, S_BOW, S_SPEAR, S_HEAD };   // 0..TNT 與方塊種類同號
  static M5Canvas spr[NTILE + 8 + NH];   // 方塊 10 張(四種礦各一張)+ 怪 4 + 道具 4 + 勇者頭 8
  static bool sprMade = false;
  static void makeSprites() {
    sprMade = true; static const uint32_t ORE[4] = { rgb(40, 40, 40), rgb(215, 180, 150), rgb(255, 210, 60), rgb(80, 240, 240) };
    for (int i = 0; i < NTILE + 8 + NH; i++) {
      int src = i < COAL ? i : i < NTILE ? S_ORE : i < NTILE + 8 ? i - NTILE + S_ZOMBIE : S_HEAD, n = src == S_WITHER ? 12 : 8;
      uint32_t star = src == S_ORE ? ORE[i - COAL] : col(i - NTILE - 8);
      spr[i].setColorDepth(8); spr[i].createSprite(n * 2, n * 2); spr[i].fillScreen(rgb(255, 0, 255));
      for (int r = 0; r < n; r++) for (int c = 0; c < n; c++) { char ch = SPR[src][r][c]; if (ch != '.') spr[i].fillRect(c * 2, r * 2, 2, 2, ch == '*' ? star : pal(ch)); }
    }
  }
  static void blit(int idx, int x, int y) { spr[idx].pushSprite(&cv, x, y, (uint32_t)rgb(255, 0, 255)); }   // idx:方塊用種類號,其他用 NTILE + (S_x - S_ORE - 1)
  static int sprOf(int s) { return NTILE + s - S_ZOMBIE; }
  static void drawHead(int i, int x, int y) { blit(NTILE + 8 + i, x - 8, y - 8); }
  static void genMap() {   // 整片隨機方塊,左半、右半各挖一個 4x3 的小房間給勇者與怪
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) {
      float p = frand(); uint8_t k = p < 0.2f ? WOOD : p < 0.5f ? STONE : p < 0.62f ? COAL + (int)(frand() * 4) % 4 : p < 0.68f ? CHEST : p < 0.72f ? IRON : p < 0.76f ? TNT : FLOOR;
      grid[r][c] = k; thp[r][c] = hits(k);
    }
    int hc = (int)(frand() * (COLS / 2 - 4)), hr = (int)(frand() * (ROWS - 3)), mc = COLS / 2 + 1 + (int)(frand() * (COLS / 2 - 5)), mr = (int)(frand() * (ROWS - 3));
    for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) grid[hr + r][hc + c] = grid[mr + r][mc + c] = FLOOR;
    memset(item, 0, sizeof item); memset(arrow, 0, sizeof arrow); memset(mob, 0, sizeof mob);
    int n = 3 + wave; if (n > MAXMOB - 1) n = MAXMOB - 1;
    for (int i = 0; i <= n; i++) {   // 最後一隻是 Boss
      bool boss = i == n; float a = frand() * 6.283f, hp = boss ? 40 + wave * 15 : 4 + wave;
      mob[i] = { (mc + 0.5f + frand() * 3) * CS, TOP + (mr + 0.5f + frand() * 2) * CS, cosf(a), sinf(a), hp, hp, 0, 2, -1, (int8_t)(boss ? BOSS : (int)(frand() * 3) % 3), true };
    }
    for (int i = 0; i < NH; i++) if (hero[i].live) { hero[i].x = (hc + 0.5f + frand() * 3) * CS; hero[i].y = TOP + (hr + 0.5f + frand() * 2) * CS; hero[i].vx = frand() < 0.5f ? -120 : 120; hero[i].vy = frand() < 0.5f ? -80 : 80; hero[i].hp = fminf(HP0, hero[i].hp + 10); }
  }
  void init() {
    wave = 0; rank = -1; over = false; animT = endT = showT = 0; alive = NH;
    for (auto& h : hero) { h = {}; h.live = true; h.hp = HP0; }
    if (!sprMade) makeSprites();
    genMap(); ringlib::reset(); cv.fillScreen(0);
  }
  // 撞球式:定速直線,傾斜把方向往傾斜那邊轉,撞牆與方塊依軸反射;dig 為真時敲方塊(smash 一下就碎,Boss 用)
  static void bounce(float& x, float& y, float& vx, float& vy, float r, float speed, bool dig, bool smash, const Ctx& c, bool steer) {
    if (steer && (fabsf(c.gx) > 0.15f || fabsf(c.gy) > 0.15f)) {
      float want = atan2f(c.gy, c.gx), have = atan2f(vy, vx), d = want - have; while (d > 3.1416f) d -= 6.2832f; while (d < -3.1416f) d += 6.2832f;
      float amt = fminf(1, sqrtf(c.gx * c.gx + c.gy * c.gy)), step = TURN * amt * c.dt; if (fabsf(d) < step) step = fabsf(d); have += d > 0 ? step : -step; vx = cosf(have); vy = sinf(have);
    }
    if (steer && (c.btnA || c.btnC)) { float a = atan2f(vy, vx) + (c.btnC - c.btnA) * AC_TURN * c.dt; vx = cosf(a); vy = sinf(a); }
    setSpeed(vx, vy, speed);
    x += vx * c.dt; y += vy * c.dt;
    static const int8_t DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
    for (int k = 0; k < 4; k++) {
      float sx = x + DX[k] * r, sy = y + DY[k] * r; if (!solid(sx, sy)) continue;
      int cc = (int)floorf(sx / CS), rr = (int)floorf((sy - TOP) / CS);
      if (dig && cc >= 0 && cc < COLS && rr >= 0 && rr < ROWS && grid[rr][cc] != IRON) { if (smash || --thp[rr][cc] == 0) breakTile(cc, rr); else { spark(sx, sy, rgb(150, 150, 155)); snd::click(); } }
      if (DX[k]) { x = DX[k] > 0 ? cc * CS - r - 0.5f : (cc + 1) * CS + r + 0.5f; vx = DX[k] > 0 ? -fabsf(vx) : fabsf(vx); }
      else { y = DY[k] > 0 ? TOP + rr * CS - r - 0.5f : TOP + (rr + 1) * CS + r + 0.5f; vy = DY[k] > 0 ? -fabsf(vy) : fabsf(vy); }
    }
    if (x < r) { x = r; vx = fabsf(vx); } if (x > W - r) { x = W - r; vx = -fabsf(vx); }
    if (y < TOP + r) { y = TOP + r; vy = fabsf(vy); } if (y > H - r) { y = H - r; vy = -fabsf(vy); }
    if (fabsf(vy) < speed * 0.15f) vy = (vy < 0 ? -1 : 1) * speed * 0.15f;
    if (fabsf(vx) < speed * 0.15f) vx = (vx < 0 ? -1 : 1) * speed * 0.15f;
  }
  static void hurtHero(Hero& h, float dmg, uint32_t sc) {
    if (h.hitT > 0) return; h.hitT = 0.6f; h.hp -= dmg; spark(h.x, h.y, sc); snd::note(180);
    if (h.hp <= 0) { h.live = false; for (int k = 0; k < 4; k++) spark(h.x, h.y, col((int)(&h - hero))); snd::note(100); if (--alive == 0) { over = true; rank = board.record(wave); } }
  }
  static void hurtMob(Mob& m, float dmg) {
    if (m.hitT > 0) return; m.hitT = 0.3f; m.hp -= dmg; spark(m.x, m.y, rgb(255, 255, 255)); snd::note(m.kind == BOSS ? 300 : 500);
    if (m.hp <= 0) { m.live = false; for (int k = 0; k < 4; k++) spark(m.x, m.y, rgb(200, 80, 200)); snd::note(650); buzz(60, 30); if (frand() < 0.3f) dropItem((int)(m.x / CS), (int)((m.y - TOP) / CS), APPLE); }
  }
  static void shoot(float x, float y, float tx, float ty, bool mine) { for (auto& a : arrow) if (!a.live) { float dx = tx - x, dy = ty - y, d = sqrtf(dx * dx + dy * dy) + 1e-3f; a = { x, y, dx / d * 170, dy / d * 170, 1.5f, true, mine }; return; } }
  static Hero* nearestHero(float x, float y, float& d2) { Hero* best = nullptr; d2 = 1e9f; for (auto& h : hero) if (h.live) { float dx = h.x - x, dy = h.y - y, q = dx * dx + dy * dy; if (q < d2) { d2 = q; best = &h; } } return best; }
  static Mob* nearestMob(float x, float y, float& d2) { Mob* best = nullptr; d2 = 1e9f; for (auto& m : mob) if (m.live) { float dx = m.x - x, dy = m.y - y, q = dx * dx + dy * dy; if (q < d2) { d2 = q; best = &m; } } return best; }
  static float mobR(const Mob& m) { return m.kind == BOSS ? BOSS_R : MOB_R; }
  static void explodeAt(float x, float y, bool hurtMobs) {   // 爆炸:炸掉周圍一圈方塊(連鎖引爆 TNT)、附近的勇者扣血;TNT 連怪一起炸
    int c0 = (int)(x / CS), r0 = (int)((y - TOP) / CS);
    for (int r = r0 - 1; r <= r0 + 1; r++) for (int c = c0 - 1; c <= c0 + 1; c++) if (c >= 0 && c < COLS && r >= 0 && r < ROWS && grid[r][c] != IRON) breakTile(c, r);
    for (auto& h : hero) if (h.live) { float dx = h.x - x, dy = h.y - y; if (dx * dx + dy * dy < 36 * 36) hurtHero(h, 15, rgb(255, 160, 40)); }
    if (hurtMobs) for (auto& m : mob) if (m.live) { float dx = m.x - x, dy = m.y - y; if (dx * dx + dy * dy < 36 * 36) { m.hitT = 0; hurtMob(m, 10); } }
    for (int k = 0; k < 8; k++) spark(x, y, rgb(255, 160, 40)); snd::note(120); buzz(200, 150);
  }
  static void explode(Mob& m) { m.live = false; explodeAt(m.x, m.y, false); }   // 苦力怕
  static void nextWave() { wave++; genMap(); showT = 0; }
  void step(const Ctx& c) {
    animT += c.dt;
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); stepSparks(c.dt); return; }
    if (showT > 0) { if ((showT += c.dt) > SHOW_T) nextWave(); stepSparks(c.dt); return; }
    for (int i = 0; i < NH; i++) { auto& h = hero[i]; if (!h.live) continue;
      h.hitT -= c.dt; h.bowT -= c.dt;
      if (c.tap) tapKick(c, h.x, h.y, h.vx, h.vy, SPEED);
      bool bow = false; for (int8_t w : h.w) bow |= w == BOW;
      bounce(h.x, h.y, h.vx, h.vy, HERO_R, SPEED, true, false, c, true);
      for (int j = i + 1; j < NH; j++) if (hero[j].live) { auto& o = hero[j]; float dx = o.x - h.x, dy = o.y - h.y, d2 = dx * dx + dy * dy; if (d2 < 4 * HERO_R * HERO_R && d2 > 1e-3f) { float d = sqrtf(d2), pen = (2 * HERO_R - d) / 2; h.x -= dx / d * pen; h.y -= dy / d * pen; o.x += dx / d * pen; o.y += dy / d * pen; } }   // 勇者互推
      for (int s = 0; s < MAXW; s++) if (h.w[s] == SWORD || h.w[s] == SPEAR) {   // 劍 / 矛:尖端掃到怪就扣血(矛長一點、痛一點)
        bool sp = h.w[s] == SPEAR; float a = animT * 4 + s * 1.5708f, tx = h.x + cosf(a) * (sp ? 20 : 14), ty = h.y + sinf(a) * (sp ? 20 : 14);
        for (auto& m : mob) if (m.live) { float dx = m.x - tx, dy = m.y - ty, rr = mobR(m) + 3; if (dx * dx + dy * dy < rr * rr) hurtMob(m, sp ? 3 : 2); }
      }
      if (bow && h.bowT <= 0) { float d2; Mob* m = nearestMob(h.x, h.y, d2); if (m && d2 < 110 * 110) { h.bowT = 1.5f; shoot(h.x, h.y, m->x, m->y, true); snd::note(900); } }
      for (auto& it : item) if (it.live) {   // 撿道具:蘋果回血,武器塞進空位
        float ix = it.c * CS + CS / 2.0f, iy = TOP + it.r * CS + CS / 2.0f, dx = h.x - ix, dy = h.y - iy; if (dx * dx + dy * dy > 10 * 10) continue;
        if (it.kind == APPLE) { h.hp = fminf(HP0, h.hp + 12); it.live = false; snd::note(750); }
        else for (auto& w : h.w) if (!w) { w = it.kind; it.live = false; snd::note(600); buzz(40, 15); break; }
      }
    }
    for (auto& m : mob) if (m.live) {
      m.hitT -= c.dt; m.cd -= c.dt; float d2; Hero* h = nearestHero(m.x, m.y, d2); float d = sqrtf(d2) + 1e-3f;
      if (h && m.kind != SKEL && d < (m.kind == BOSS ? 90 : 60)) { m.vx = (h->x - m.x) / d; m.vy = (h->y - m.y) / d; }   // 殭屍 / 苦力怕 / Boss 看到勇者就追
      bounce(m.x, m.y, m.vx, m.vy, mobR(m), m.kind == BOSS ? MOB_SPEED * 0.8f : m.kind == ZOMBIE && d < 60 ? MOB_SPEED * 1.5f : MOB_SPEED, true, m.kind == BOSS, c, false);   // 怪也挖,Boss 一下就碎
      for (auto& o : hero) if (o.live) {   // 勇者撞到怪:推開、兩邊沿法線反彈、勇者扣血
        float dx = o.x - m.x, dy = o.y - m.y, q = sqrtf(dx * dx + dy * dy) + 1e-3f, rr = mobR(m) + HERO_R; if (q >= rr) continue;
        float nx = dx / q, ny = dy / q; o.x = m.x + nx * rr; o.y = m.y + ny * rr;
        float vn = o.vx * nx + o.vy * ny; if (vn < 0) { o.vx -= 2 * vn * nx; o.vy -= 2 * vn * ny; }
        float mn = m.vx * nx + m.vy * ny; if (mn > 0) { m.vx -= 2 * mn * nx; m.vy -= 2 * mn * ny; }
        snd::click();
      }
      if (!h) continue;
      if (m.kind == SKEL && d < 130 && m.cd <= 0) { m.cd = 2; shoot(m.x, m.y, h->x, h->y, false); }
      if (m.kind == ZOMBIE && d < mobR(m) + HERO_R + 4 && m.cd <= 0) { m.cd = 1.2f; hurtHero(*h, 5, rgb(200, 80, 200)); }   // 咬
      if (m.kind == BOSS && m.cd <= 0) { m.cd = 2; snd::note(220); for (auto& o : hero) if (o.live) { float dx = o.x - m.x, dy = o.y - m.y; if (dx * dx + dy * dy < 30 * 30) hurtHero(o, 12 + wave * 2, rgb(255, 80, 80)); } }   // 揮爪
      if (m.kind == CREEPER) { if (m.fuse < 0 && d < 22) { m.fuse = 1; snd::note(1200); } if (m.fuse >= 0 && (m.fuse -= c.dt) <= 0) explode(m); }
    }
    for (auto& a : arrow) if (a.live) {   // 箭:直飛,撞方塊消失,打到對方扣血
      a.x += a.vx * c.dt; a.y += a.vy * c.dt; if ((a.life -= c.dt) <= 0 || solid(a.x, a.y)) { a.live = false; continue; }
      if (a.mine) { for (auto& m : mob) if (m.live) { float dx = m.x - a.x, dy = m.y - a.y, r = mobR(m) + 2; if (dx * dx + dy * dy < r * r) { hurtMob(m, 3); a.live = false; break; } } }
      else for (auto& h : hero) if (h.live) { float dx = h.x - a.x, dy = h.y - a.y; if (dx * dx + dy * dy < (HERO_R + 2) * (HERO_R + 2)) { hurtHero(h, 6, rgb(220, 220, 220)); a.live = false; break; } }
    }
    bool any = false; for (auto& m : mob) any |= m.live;
    if (!any && !over) { showT = 1e-3f; snd::note(800); buzz(120, 80); }   // 怪清光:結算這一波
    stepSparks(c.dt);
  }
  static void drawTile(int c, int r) {
    int x = c * CS, y = TOP + r * CS; uint8_t k = grid[r][c];
    if (k == FLOOR) { cv.fillRect(x, y, CS, CS, rgb(120, 85, 58)); cv.fillRect(x + 2, y + 3, 3, 2, rgb(95, 65, 42)); cv.fillRect(x + 10, y + 6, 2, 3, rgb(95, 65, 42)); cv.fillRect(x + 5, y + 11, 3, 2, rgb(95, 65, 42)); cv.fillRect(x + 12, y + 12, 2, 2, rgb(140, 100, 70)); return; }   // 泥土
    blit(k, x, y);
    int dmg = hits(k) - thp[r][c];   // 裂痕
    if (dmg > 0) { cv.drawLine(x + 3, y + 2, x + 8, y + 9, 0); cv.drawLine(x + 8, y + 9, x + 5, y + 14, 0); }
    if (dmg > 1) { cv.drawLine(x + 13, y + 3, x + 9, y + 8, 0); cv.drawLine(x + 2, y + 11, x + 7, y + 13, 0); }
    if (dmg > 2) { cv.drawLine(x + 1, y + 5, x + 6, y + 6, 0); cv.drawLine(x + 10, y + 12, x + 14, y + 15, 0); }
  }
  static void drawMob(const Mob& m) {
    int x = (int)m.x, y = (int)m.y;
    if (m.kind == BOSS) { blit(sprOf(S_WITHER), x - 12, y - 12); cv.fillRect(x - 14, y - 18, 28, 3, rgb(60, 20, 20)); cv.fillRect(x - 14, y - 18, (int)(28 * m.hp / m.maxHp), 3, rgb(230, 50, 50)); }
    else { blit(sprOf(S_ZOMBIE + m.kind), x - 8, y - 8); if (m.fuse >= 0 && (int)(m.fuse * 10) & 1) cv.drawRect(x - 8, y - 8, 16, 16, rgb(255, 255, 255)); }   // 苦力怕點燃閃白框
    if (m.hitT > 0.15f) cv.drawRect(x - mobR(m) - 1, y - mobR(m) - 1, mobR(m) * 2 + 3, mobR(m) * 2 + 3, rgb(255, 255, 255));
  }
  void draw() {
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) drawTile(c, r);
    for (auto& it : item) if (it.live) {
      static const int8_t S[5] = { 0, S_SWORD, S_BOW, S_SPEAR, S_APPLE }; blit(sprOf(S[it.kind]), it.c * CS, TOP + it.r * CS);
    }
    for (auto& a : arrow) if (a.live) { float v = sqrtf(a.vx * a.vx + a.vy * a.vy) + 1e-3f; cv.drawLine((int)a.x, (int)a.y, (int)(a.x - a.vx / v * 6), (int)(a.y - a.vy / v * 6), a.mine ? rgb(255, 255, 255) : rgb(170, 170, 160)); }
    for (auto& m : mob) if (m.live) drawMob(m);
    for (int i = 0; i < NH; i++) { auto& h = hero[i]; if (!h.live) continue;
      drawHead(i, (int)h.x, (int)h.y); if (h.hitT > 0.4f) cv.drawRect((int)h.x - 8, (int)h.y - 8, 16, 16, rgb(255, 255, 255));
      for (int s = 0; s < MAXW; s++) if (h.w[s]) {   // 武器繞著轉:跟地上同一張像素圖,轉到刀尖 / 弓背朝外(劍圖原本指右上、弓背原本朝左上)
        float a = animT * 4 + s * 1.5708f, deg = a * RAD_TO_DEG; int w = h.w[s]; float rr = w == SPEAR ? 16 : 13;
        spr[sprOf(w == SWORD ? S_SWORD : w == BOW ? S_BOW : S_SPEAR)].pushRotateZoom(&cv, h.x + cosf(a) * rr, h.y + sinf(a) * rr, w == BOW ? deg + 135 : deg + 45, 1, 1, (uint32_t)rgb(255, 0, 255));
      }
    }
    drawSparks();
    cv.fillRect(0, 0, W, TOP, rgb(20, 18, 24));   // HUD:8 色圓(血量扇形、死掉全暗)、第幾波、剩幾隻怪
    for (int i = 0; i < NH; i++) { int x = 8 + i * 14; cv.fillCircle(x, 8, 5, rgb(50, 50, 55)); if (hero[i].live) cv.fillArc(x, 8, 0, 5, -90, -90 + 360 * hero[i].hp / HP0, col(i)); }   // 血是扇形:滿血整圓、半血半圓
    int nm = 0; for (auto& m : mob) nm += m.live;
    char t[32]; snprintf(t, sizeof t, "W%d  mobs %d", wave + 1, nm);
    cv.setTextDatum(middle_left); cv.setTextSize(1); cv.setTextColor(rgb(220, 210, 180), rgb(20, 18, 24)); cv.drawString(t, 124, 8);
    if (showT > 0) {   // 波次結算:哪些顏色還活著
      cv.fillRect(60, 80, 200, 70, 0); cv.drawRoundRect(60, 80, 200, 70, 6, rgb(120, 120, 140));
      snprintf(t, sizeof t, "WAVE %d CLEAR", wave + 1); cv.setTextDatum(top_center); cv.setTextSize(2); cv.setTextColor(rgb(255, 230, 0), 0); cv.drawString(t, W / 2, 86);
      for (int i = 0; i < NH; i++) if (hero[i].live) drawHead(i, W / 2 - 63 + i * 18, 120);
      cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); cv.drawString("survived", W / 2, 134);
    }
    if (over) { snprintf(t, sizeof t, "WAVES %d", wave); board.draw(t, rank, "wiped out, tap"); }
  }
}
