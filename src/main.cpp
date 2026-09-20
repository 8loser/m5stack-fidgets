// M5Stack Core2 物理模擬小遊戲合集
// 底部三個觸控鍵:選單 A/C 換卡片、B 進入;遊戲內 A/C 是遊戲操作(有東西在轉的遊戲用來轉它,其他預設當往左 / 往右傾斜)、B 長按回選單、雙擊重置
// 新遊戲:寫一個 xxx.h(namespace 內提供 init/step/draw),include 後在 games[] 加一行
#include "common.h"
#include <Preferences.h>
#include "balls.h"
#include "plinko.h"
#include "expand.h"
#include "ttt.h"
#include "slicer.h"
#include "gaprings.h"
#include "tri.h"
#include "merge.h"
#include "grow.h"
#include "balloon.h"
#include "split.h"
#include "paint.h"
#include "beat.h"
#include "rubble.h"
#include "penta.h"
#include "war.h"
#include "shatter.h"
#include "pet.h"
#include "swing.h"
#include "dino.h"
#include "seed.h"
#include "colormerge.h"
#include "inkring.h"
#include "spawnring.h"
#include "arcring.h"
#include "seesaw.h"
#include "wheels.h"
#include "spikebowl.h"
#include "penalty.h"
#include "dig.h"
#include "stack.h"
#include "nbguess.h"
#include "nbbuild.h"
#include "nbcompare.h"
#include "mathrun.h"
#include "dungeon.h"
#include "brawl.h"
#include "redlight.h"

// ================= 選單(卡片輪播 + 活的預覽)與主迴圈 =================
struct Game { float hue; void (*init)(); void (*step)(const Ctx&); void (*draw)(); bool acOwn; };   // acOwn:遊戲自己用 A/C(轉東西),主迴圈就不把它們當虛擬傾斜
static const Game games[] = {
  { 0.55f, balls::init,  balls::step,  balls::draw  },
  { 0.33f, plinko::init, plinko::step, plinko::draw },
  { 0.15f, expand::init,  expand::step,  expand::draw,  true },
  { 0.50f, ttt::init,     ttt::step,     ttt::draw,     true },
  { 0.70f, slicer::init,  slicer::step,  slicer::draw  },
  { 0.05f, gaprings::initIn,  gaprings::step, gaprings::draw, true },
  { 0.90f, tri::init,     tri::step,     tri::draw     },
  { 0.40f, merge::init,   merge::step,   merge::draw   },
  { 0.78f, grow::init,    grow::step,    grow::draw    },
  { 0.12f, balloon::init, balloon::step, balloon::draw },
  { 0.98f, split::init,   split::step,   split::draw   },
  { 0.60f, paint::init,   paint::step,   paint::draw   },
  { 0.85f, gaprings::initOut, gaprings::step, gaprings::draw, true },
  { 0.30f, beat::init,    beat::step,    beat::draw    },
  { 0.08f, rubble::init,  rubble::step,  rubble::draw,  true },
  { 0.45f, penta::init,   penta::step,   penta::draw,   true },
  { 0.62f, war::init,     war::step,     war::draw     },
  { 0.20f, shatter::init, shatter::step, shatter::draw },
  { 0.00f, pet::init,     pet::step,     pet::draw     },
  { 0.10f, swing::init,   swing::step,   swing::draw   },
  { 0.66f, dino::init,    dino::step,    dino::draw    },
  { 0.75f, seed::init,    seed::step,    seed::draw    },
  { 0.28f, colormerge::init, colormerge::step, colormerge::draw, true },
  { 0.00f, inkring::init, inkring::step, inkring::draw },
  { 0.52f, spawnring::init, spawnring::step, spawnring::draw, true },
  { 0.95f, arcring::init, arcring::step, arcring::draw, true },
  { 0.35f, seesaw::init,  seesaw::step,  seesaw::draw,  true },
  { 0.18f, wheels::init,  wheels::step,  wheels::draw,  true },
  { 0.08f, spikebowl::init, spikebowl::step, spikebowl::draw, true },
  { 0.33f, penalty::init, penalty::step, penalty::draw },
  { 0.10f, dig::init,     dig::step,     dig::draw     },
  { 0.60f, stack::init,   stack::step,   stack::draw   },
  { 0.02f, nbguess::init, nbguess::step, nbguess::draw, true },
  { 0.12f, nbbuild::init, nbbuild::step, nbbuild::draw, true },
  { 0.22f, nbcompare::init, nbcompare::step, nbcompare::draw, true },
  { 0.58f, mathrun::init, mathrun::step, mathrun::draw, true },
  { 0.30f, dungeon::init, dungeon::step, dungeon::draw, true },
  { 0.42f, brawl::init,   brawl::step,   brawl::draw   },
  { 0.93f, redlight::init, redlight::step, redlight::draw, true },
};
constexpr int NG = sizeof games / sizeof games[0];
static int cur = -1, sel = 0, started = -1;   // cur<0 = 在選單;started = 已經 init 過的遊戲(進遊戲第一幀保險重置用)
static uint16_t plays[NG]; static Preferences playPrefs;   // 各遊戲進入次數,存 NVS;games[] 順序改了計數就會對不上
static void enterGame() { cur = sel; plays[sel]++; playPrefs.putBytes("n", plays, sizeof plays); games[sel].init(); }   // 進遊戲一律重置,不接著預覽玩

// 選單用第二層畫布;被選中的遊戲照常畫在 cv,再縮小貼進卡片
static M5Canvas menu(&M5.Display);
constexpr int CW = 288, CH = 216, CX = W / 2, CY = 2 + CH / 2;

static float idleT = 0;   // 沒操作就自動輪播
static void drawMenu(const Ctx& c) {
  static int shown = -1;
  if ((idleT += c.dt) > 10) { idleT = 0; sel = (sel + 1) % NG; }
  if (shown != sel) { shown = sel; games[sel].init(); }
  Ctx pc = c; pc.touch = pc.tap = false;   // 預覽只吃重力,不吃觸控
  games[sel].step(pc); games[sel].draw();

  menu.fillScreen(0);
  cv.pushRotateZoom(&menu, CX, CY, 0, (float)CW / W, (float)CH / H);
  menu.drawRoundRect(CX - CW / 2, CY - CH / 2, CW, CH, 6, rgb(255, 255, 255));
  menu.setTextSize(1); menu.setTextColor(rgb(160, 160, 160), 0);
  menu.setTextDatum(middle_left);  menu.drawString("<", 6, H - 9);
  menu.setTextDatum(middle_right); menu.drawString(">", W - 6, H - 9);
  char buf[32]; snprintf(buf, sizeof buf, "%d / %d   played %u", sel + 1, NG, plays[sel]);
  menu.setTextDatum(middle_center); menu.setTextColor(rgb(255, 255, 255), 0); menu.drawString(buf, CX, H - 9);
  menu.pushSprite(0, 0);
}

void setup() {
  auto cfg = M5.config(); M5.begin(cfg);
  M5.Speaker.setVolume(96); snd::init();   // click 的 500 Hz 在小喇叭上出力弱,主音量開大(整體太吵就調這裡)、note 的 AMP 已等比例壓低
  cv.setColorDepth(8); cv.createSprite(W, H);
  menu.setColorDepth(8);
  if (!menu.createSprite(W, H)) { menu.setPsram(true); menu.createSprite(W, H); }
  buildFade();
  playPrefs.begin("plays", false); playPrefs.getBytes("n", plays, sizeof plays);
}

void loop() {
  M5.update();
  static uint32_t last = millis(); uint32_t now = millis();
  Ctx c{}; c.dt = fminf((now - last) / 1000.0f, 0.04f); last = now;
  if (vibUntil && now > vibUntil) { M5.Power.setVibration(0); vibUntil = 0; }

  auto t = M5.Touch.getDetail(0);
  bool onScreen = t.y < H;              // y>=H 是底部 A/B/C 鍵區
  // M5Unified 把鍵區三等分,B 佔中間 107 px;這裡再縮成正中 70 px,靠邊的算是按 A/C 擦到。只看按下那一刻的 x,長按時手指滑動不影響
  // 另外 M5Unified 只要有鍵按著,手指滾到哪格那格也算按下,所以 A/C 按著或剛放開 300 ms 內一律不認 B
  static int bX = 160; static uint32_t lastAC = 0; if (M5.BtnB.wasPressed()) bX = t.x;
  if (M5.BtnA.isPressed() || M5.BtnC.isPressed()) lastAC = now;
  bool bCenter = abs(bX - 160) < 35 && now - lastAC > 300;
  c.touch = t.isPressed() && onScreen;
  c.tap = t.wasPressed() && onScreen;
  c.tx = t.x; c.ty = t.y;

  static float gx = 0, gy = 1, shake = 0;   // IMU 沒新資料時沿用上一幀
  float ax, ay, az;
  if (M5.Imu.update() && M5.Imu.getAccel(&ax, &ay, &az)) { gx = TILT_X * ax * TILT_GAIN; gy = TILT_Y * ay * TILT_GAIN; shake = fabsf(sqrtf(ax * ax + ay * ay + az * az) * TILT_GAIN - 1); }
  c.gx = gx; c.gy = gy; c.shake = shake;

  if (cur < 0) {
    if (onScreen && t.wasReleased()) {
      bool inCard = abs(t.x - CX) < CW / 2 && abs(t.y - CY) < CH / 2;
      if (abs(t.distanceX()) > 40) sel = (sel + (t.distanceX() < 0 ? 1 : NG - 1)) % NG;
      else if (t.wasClicked() && inCard) enterGame();
    }
    if (M5.BtnA.wasClicked()) sel = (sel + NG - 1) % NG;
    if (M5.BtnC.wasClicked()) sel = (sel + 1) % NG;
    if (M5.BtnB.wasClicked() && bCenter) enterGame();
    if (t.wasReleased() || M5.BtnA.wasClicked() || M5.BtnC.wasClicked()) idleT = 0;
    muted = true;
    drawMenu(c);
  } else {
    // 遊戲內:A/C 交給遊戲(沒特別用的遊戲就是虛擬傾斜);B 長按回選單、雙擊重置。短按不做事,按 A/C 時擦到 B 才不會跳出
    if (M5.BtnB.wasHold() && bCenter) { cur = -1; started = -1; idleT = 0; return; }
    if (M5.BtnB.wasDoubleClicked() && bCenter) games[cur].init();
    c.btnA = M5.BtnA.isPressed(); c.btnC = M5.BtnC.isPressed(); c.tapA = M5.BtnA.wasPressed(); c.tapC = M5.BtnC.wasPressed();
    if (!games[cur].acOwn) c.gx += (c.btnC - c.btnA) * 0.7f;
    muted = false;
    if (started != cur) { started = cur; games[cur].init(); }   // 進遊戲的第一幀再保險重置一次(不管從哪條路進來)
    games[cur].step(c); games[cur].draw();
    cv.setTextDatum(top_right); cv.setTextSize(1); cv.setTextColor(rgb(120, 120, 120), 0);
    char buf[12]; snprintf(buf, sizeof buf, "%d / %d", cur + 1, NG); cv.drawString(buf, W - 2, 2);
    cv.pushSprite(0, 0);
  }
}
