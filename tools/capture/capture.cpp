// 電腦版截圖:不開視窗,每個遊戲用腳本輸入(左右晃的重力、定時點螢幕、輪流按 A / C)跑一段,
// 畫布(RGB332)每 2 幀接著寫進 <out>/<key>.raw,run.sh 再用 ffmpeg(pix_fmt rgb8)組成 GIF
// 用法:capture <out> [key...](沒給 key 就跑全部)
#include "../../src/main.cpp"
#include <cstdio>
#include <string>
#include <sys/stat.h>

constexpr float DT = 1 / 30.0f, WARM = 2, REC = 6;   // 預設先跑 2 秒再錄 6 秒,GIF 15 fps

// 個別調整:畫面要累積的遊戲跑久一點再錄;盪繩只留左右晃的重力(NOTAP);切割只在開局點一下正中放切割點(ONCE);
// 圓球溢出開局按 C 把缺口從 -45 度轉到正下方(每秒 120 度)再放手,球就一路掉出去(SPIN);很快結束的從第 0 秒錄。
// shake:從開跑第 0.7 秒起每 2 秒搖一下(暖機期間也搖,線之類會累積的才鋪得開),只開給「搖一下噴開」是玩法之一的遊戲(木頭人搖了會被開槍,不開)。
// bot:亂按會錯過核心玩法的遊戲(要看時機、要答對、要點對的空位),讀遊戲內部狀態照玩家的方式輸入;bot 有給就蓋掉腳本輸入
enum { SCRIPT, NOTAP, ONCE, SPIN };
static void press(Ctx& c, int k) {   // Numberblocks 三選一:A 左、C 右、點中間那格
  if (k == 0) c.tapA = c.btnA = true; else if (k == 2) c.tapC = c.btnC = true; else { c.tap = c.touch = true; c.tx = 160; c.ty = 210; }
}
static bool every(float& acc, float s) { if ((acc += DT) < s) return false; acc = 0; return true; }   // 每 s 秒 true 一次,讓動作看得清楚

static void botGears(Ctx& c) {   // 依序把每個空位點到正解的大小(陷阱留空),接通後按住 C 開馬達
  static float w; using namespace gearsim;
  if (gears::winT > 0) return;
  for (int i = 2; i < ng; i++) if (g[i].ans && g[i].n != g[i].ans) { if (every(w, 0.35f)) { c.tap = c.touch = true; c.tx = (int)g[i].x; c.ty = (int)g[i].y; } return; }
  c.btnC = true;
}
static void botSwing(Ctx& c) {   // 跟著擺動方向全力傾斜加力,盪到接近水平(1.40 rad,約 80 度)、還在往上往前時放手(掃過門檻:1.35 以下與 1.45 以上都抓不到,1.40 連抓 3 個);到最右邊就掉頭往左
  using namespace swing; static int dir = 1;
  if (anchor < 0) return;
  if (anchor == NA - 1) dir = -1; if (anchor == 0) dir = 1;
  auto& q = ragdoll::p[hand]; float vx = q.x - q.ox, vy = q.y - q.oy, ang = atan2f(q.x - AX[anchor], q.y - AY);
  c.gx = vx > 0 ? 1 : -1;
  if (freeT > 0.5f && ang * dir > 1.40f && vy < -0.5f && vx * dir > 0) { c.tap = c.touch = true; c.tx = 160; c.ty = 120; }
}
static void botDino(Ctx& c) {   // 仙人掌快到就跳,低的翼龍就蹲,高的不理
  using namespace dino;
  if (dead) return;
  float v = speed;
  for (auto& o : ob) if (o.live) {
    float d = o.x - (DX + 20); if (d < -30 || d > v * 0.5f) continue;
    if (o.kind <= 2 && d > 0 && d < v * 0.15f) c.tap = c.touch = true, c.tx = 160, c.ty = 60;
    if (o.kind == 4) c.btnC = true;
  }
}
static void botRedlight(Ctx& c) {   // 綠燈時左右腳交替走,最後兩個音前收腳;紅燈不動
  using namespace redlight; static float w;
  if (red || dead || noteI >= NNOTE - 2) return;
  if (every(w, 0.18f)) { if (foot) c.tapA = c.btnA = true; else c.tapC = c.btnC = true; }
}
static void botGuess(Ctx& c) {   // 看 1 秒再點對的
  using namespace nbguess; static float w;
  if (picked >= 0 || over) { w = 0; return; }
  if (every(w, 1)) for (int k = 0; k < 3; k++) if (opt[k] == n) press(c, k);
}
static void botBuild(Ctx& c) {   // 先加 10 的柱、再加 1 塊,湊到就點目標框送出
  using namespace nbbuild; static float w;
  if (msgT > 0 || over || !every(w, 0.3f)) return;
  if (n + 10 <= target) c.tapA = c.btnA = true;
  else if (n < target) c.tapC = c.btnC = true;
  else { c.tap = c.touch = true; c.tx = 160; c.ty = 30; }
}
static void botCompare(Ctx& c) {   // 比大小往大的那邊傾斜(核心玩法),加法題點對的選項
  using namespace nbcompare; static float w;
  if (picked >= 0 || over) { w = 0; return; }
  if ((w += DT) < 1) return;
  if (!sum) c.gx = ans == 0 ? -0.7f : 0.7f;
  else for (int k = 0; k < 3; k++) if (opt[k] == ans) { press(c, k); w = 0; }
}
static void botMathrun(Ctx& c) {   // 門靠近一半才傾斜到對的那邊
  using namespace mathrun;
  if (!over && z < 0.6f) c.gx = leftIsAns ? -0.6f : 0.6f;
}

// 修車廠:上層看狀態排一串手勢(點 / 拖 / 繞圈 / 來回擦 / 按 A C),下層逐幀播放;手勢播完才排下一串
namespace gbot {
  enum { TAP, DRAG, CIRCLE, RUB, BA, BC, WAIT };
  struct G { int k; float x0, y0, x1, y1, dur; };   // DRAG:x0,y0 → x1,y1;CIRCLE:圓心 x0,y0、半徑 x1、總轉角 y1(正 = 畫面上順時針);RUB:中心 x0,y0、幅度 x1
  static G q[32]; static int qh, qt; static float e;
  static void add(int k, float x0 = 0, float y0 = 0, float x1 = 0, float y1 = 0, float dur = 0.1f) { q[qt++] = { k, x0, y0, x1, y1, dur }; }
  static void touch(Ctx& c, float x, float y, bool first) { c.touch = true; c.tap = first; c.tx = (int)x; c.ty = (int)y; }
  static void play(Ctx& c) {   // 每個手勢之後留一幀放開手指
    G& g = q[qh]; float u = fminf(e / g.dur, 1); bool first = e == 0;
    if (e < g.dur) switch (g.k) {
      case TAP: touch(c, g.x0, g.y0, first); break;
      case DRAG: touch(c, g.x0 + (g.x1 - g.x0) * u, g.y0 + (g.y1 - g.y0) * u, first); break;
      case CIRCLE: { float a = -1.5708f + g.y1 * u; touch(c, g.x0 + cosf(a) * g.x1, g.y0 + sinf(a) * g.x1, first); break; }
      case RUB: touch(c, g.x0 + sinf(u * 6.2832f * 3) * g.x1, g.y0, false); break;   // 擦泥巴不算點:泥巴可能落在引擎蓋的點擊區,點下去會跑進電瓶
      case BA: if (first) c.tapA = c.btnA = true; break;
      case BC: if (first) c.tapC = c.btnC = true; break;
    }
    if ((e += DT) > g.dur + DT) { e = 0; qh++; }
  }
  static void tire() {   // 有釘子或爆胎:拆 5 顆螺帽、撥掉舊胎;新胎鎖回;沒氣就打氣
    using namespace garage; Tire& w = garage::tire[garage::cur];
    bool allOff = true; for (bool n : w.nut) if (n) allOff = false;
    if (w.dmg) {
      if (!allOff) { for (int k = 0; k < NN; k++) if (w.nut[k]) add(DRAG, nutX(k), nutY(k), nutX(k), nutY(k), 0.55f); }
      else add(DRAG, TX - 50, TY, TX + 30, TY, 0.35f);
    } else if (!nutsOn(w)) { for (int k = 0; k < NN; k++) if (!w.nut[k]) add(DRAG, nutX(k), nutY(k), nutX(k), nutY(k), 0.55f); }
    else if (w.psi < 95) { add(DRAG, 270, 195, 270, 105, 0.25f); add(DRAG, 270, 105, 270, 198, 0.2f); }
    else add(BA);
  }
  static void plan() {
    using namespace garage;
    switch (state) {
      case SHOP:
        for (int i = 0; i < 2; i++) if (!tireOk(garage::tire[i])) { add(TAP, WX[i], WY); return; }
        if (fuel < 0.9f) { add(TAP, FX, FY); return; }
        if (batt < 0.95f) { add(TAP, 235, 127); return; }
        if (!lampOk()) { add(TAP, HX, HY); return; }
        if (dent > 0.05f) { add(TAP, DX, DY); add(WAIT, 0, 0, 0, 0, 0.15f); return; }
        for (auto& m : mud) if (m.a > 0.05f) { add(RUB, m.x, m.y, 10, 0, 0.6f); return; }
        add(WAIT, 0, 0, 0, 0, 0.3f); add(BC); return;
      case TIRE: tire(); return;
      case FUEL: if (fuel >= 0.93f) add(BA); return;   // 倒油用傾斜,在 botGarage 逐幀給
      case BATT:
        if (batt >= 1) { add(BA); return; }
        for (int i = 0; i < 2; i++) if (clampAt[i] != i) { add(DRAG, CRX[i], CRY, TMX[i], TMY, 0.5f); return; }
        return;
      case LAMP:
        if (bulb == TIGHT && broken) add(CIRCLE, LX, LY, 50, -7.5f, 1.5f);      // 逆時針轉下破燈泡
        else if (bulb == OUT) add(DRAG, LX, 212, LX, LY, 0.5f);                   // 新燈泡拖進燈座
        else if (bulb == LOOSE) add(CIRCLE, LX, LY, 50, 7.5f, 1.5f);             // 順時針鎖緊
        else add(BA);
        return;
    }
  }
}
static void botGarage(Ctx& c) {
  using namespace gbot;
  if (qh == qt) { qh = qt = 0; plan(); add(WAIT, 0, 0, 0, 0, 0.25f); }
  play(c);
  if (garage::state == garage::FUEL && garage::fuel < 0.93f) c.gx = 0.9f;   // 往右傾倒油
}

struct Tune { const char* key; float warm; int mode; bool shake; void (*bot)(Ctx&); float rec; };   // rec 0 = 預設 REC
static const Tune tunes[] = {
  { "stringart", 30, NOTAP, true }, { "overflow", 12, SPIN }, { "merge", 15, SCRIPT, true }, { "stack", 15, SCRIPT },
  { "slicer", 0, ONCE, true }, { "balls", WARM, SCRIPT, true }, { "balloon", WARM, SCRIPT, true }, { "split", WARM, SCRIPT, true },
  { "colormerge", WARM, SCRIPT, true },
  { "gears", 0, SCRIPT, false, botGears }, { "swing", WARM, SCRIPT, false, botSwing }, { "dino", WARM, SCRIPT, false, botDino },
  { "redlight", WARM, SCRIPT, false, botRedlight }, { "nbguess", 0, SCRIPT, false, botGuess }, { "nbbuild", 0, SCRIPT, false, botBuild },
  { "nbcompare", 0, SCRIPT, false, botCompare }, { "mathrun", 0, SCRIPT, false, botMathrun },
  { "garage", 0, SCRIPT, false, botGarage, 20 },   // 一台車要修好幾個零件,6 秒只夠修一個
};

static void capture(int g, const std::string& path) {
  FILE* fp = fopen(path.c_str(), "wb");
  rng = 0x9E3779B9; srand(1);
  games[g].init();
  float warm = WARM; int mode = SCRIPT; bool shk = false; void (*bot)(Ctx&) = nullptr; float rec = REC;
  for (auto& u : tunes) if (!strcmp(u.key, games[g].key)) { warm = u.warm; mode = u.mode; shk = u.shake; bot = u.bot; if (u.rec) rec = u.rec; }
  bool prevA = false, prevC = false;
  for (int f = 0; f < (warm + rec) / DT; f++) {
    float t = f * DT;
    Ctx c{}; c.dt = DT;
    c.gx = 0.35f * sinf(t * 1.3f); c.gy = 1;
    float tp = fmodf(t, 1.2f);   // 每 1.2 秒點一下,按住 3 幀
    c.touch = tp < 0.1f; c.tap = c.touch && tp < DT;
    if (c.touch) { uint32_t s = rng; rng = (uint32_t)(t / 1.2f) * 2654435761u + 1; c.tx = 40 + frand() * 240; c.ty = 30 + frand() * 180; rng = s; }
    float ph = fmodf(t, 4);      // 每 4 秒:A 按 1 秒、停 1 秒、C 按 1 秒、停 1 秒
    c.btnA = ph < 1; c.btnC = ph >= 2 && ph < 3;
    c.tapA = c.btnA && !prevA; c.tapC = c.btnC && !prevC; prevA = c.btnA; prevC = c.btnC;
    if (mode != SCRIPT || bot) { c.touch = c.tap = c.btnA = c.btnC = c.tapA = c.tapC = false; if (mode == SPIN || bot) c.gx = 0; }
    if (bot) bot(c);
    if (mode == SPIN) c.btnC = t < 135 / 120.0f;
    float st = t - 0.7f; c.shake = shk && st >= 0 && fmodf(st, 2) < DT ? 1.5f : 0;
    if (mode == ONCE && f < 3) { c.touch = true; c.tap = f == 0; c.tx = W / 2; c.ty = H / 2; }
    if (!games[g].acOwn) c.gx += (c.btnC - c.btnA) * 0.7f;
    games[g].step(c); games[g].draw();
    if (t >= warm && f % 2 == 0) fwrite(cv.getBuffer(), 1, W * H, fp);
  }
  fclose(fp);
}

int main(int argc, char** argv) {
  if (argc < 2) { fprintf(stderr, "usage: %s <out> [key...]\n", argv[0]); return 1; }
  muted = true;
  snd::init();
  cv.setColorDepth(8); cv.createSprite(W, H);
  buildFade();
  mkdir(argv[1], 0755);
  for (int g = 0; g < NG; g++) {
    bool want = argc == 2;
    for (int i = 2; i < argc; i++) want |= std::string(argv[i]) == games[g].key;
    if (!want) continue;
    printf("%2d %s\n", g + 1, games[g].key); fflush(stdout);
    capture(g, std::string(argv[1]) + "/" + games[g].key + ".raw");
  }
}
