#pragma once
#include "ringlib.h"
#include "face.h"
#include <Preferences.h>
#include <time.h>

// ================= 19. 火柴人電子雞 =================
// 養一隻火柴人:飽食 / 心情 / 精神三條值隨真實時間下降,狀態存 NVS、用 RTC 算離線時間,關機也繼續活。
// 蛋 1 分鐘孵化,1 小時後長大。下排四個鍵:餵食(掉一顆蘋果,牠走過去吃)、玩(丟球,傾斜滾球讓牠追)、
// 睡 / 叫醒(睡覺回精神)、清理(便便放著心情會掉)。點牠會跳一下,睡覺時點牠會不爽。
// 傾斜太斜牠會滑倒站不穩、久了頭暈;搖裝置搖久了也會頭暈、心情掉。任一值歸零太久會生病,病一小時就死,死了點一下重新孵蛋
namespace pet {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int FLOOR = 186, BTN_Y = 202, NBTN = 4; constexpr uint32_t MAGIC = 0x50455433;
  constexpr float HUNGER_DEC = 100.0f / 7200, HAPPY_DEC = 100.0f / 10800, ENERGY_DEC = 100.0f / 14400, ENERGY_REC = 100.0f / 1200, POOP_EVERY = 2400, SICK_DIE = 3600, WALK = 45;
  struct Save { uint32_t magic, born, last; float hunger, happy, energy, sick, poopT; uint8_t poops; bool sleeping, recorded; } static sv;
  static Board board = { "pet" }; static int rank;   // 排行:活了幾分鐘
  static Preferences prefs; static bool loaded;
  // 動畫 / 互動狀態(不存)
  static float px, dir, phase, target, idleT, eatT, jumpY, jumpV, saveT, annoyT, dizzyT, tiltT, hatchT, lean, shakeT;
  static struct { float x; bool live; } food; static struct { float x, vx, t; int catches; bool live; } ball;
  static face::Mood mood;

  static uint32_t now() {   // RTC 秒數;沒 RTC 就退回開機秒數(離線時間就算不出來)
    if (M5.Rtc.isEnabled()) { auto t = M5.Rtc.getDateTime().get_tm(); return (uint32_t)mktime(&t); }
    return millis() / 1000;
  }
  static bool dead() { return sv.sick >= SICK_DIE; }
  static uint32_t age() { uint32_t n = now(); return n > sv.born ? n - sv.born : 0; }
  static int stage() { uint32_t a = age(); return a < 60 ? 0 : a < 3600 ? 1 : 2; }   // 0 蛋、1 幼、2 成
  static float clamp(float v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
  static void tick(float dt) {   // 真實時間推進(離線補算也走這裡)
    if (dead() || stage() == 0) return;
    sv.hunger = clamp(sv.hunger - HUNGER_DEC * dt * (sv.sleeping ? 0.5f : 1));
    sv.happy = clamp(sv.happy - HAPPY_DEC * dt * (1 + sv.poops));
    sv.energy = clamp(sv.energy + (sv.sleeping ? ENERGY_REC : -ENERGY_DEC) * dt);
    if (!sv.sleeping && (sv.poopT += dt) > POOP_EVERY && sv.poops < 3) { sv.poopT = 0; sv.poops++; }
    if (sv.hunger <= 0 || sv.happy <= 0 || sv.energy <= 0) sv.sick += dt; else if (sv.hunger > 20 && sv.happy > 20 && sv.energy > 20) sv.sick = fmaxf(0, sv.sick - dt * 2);
    if (sv.sleeping && sv.energy >= 100) sv.sleeping = false;   // 睡飽自己醒
    if (dead() && !sv.recorded) { sv.recorded = true; rank = board.record(age() / 60); }   // 下一次 save() 會把 recorded 存起來
  }
  static void save() { sv.last = now(); prefs.putBytes("s", &sv, sizeof sv); saveT = 0; }
  static void newEgg() { uint32_t n = now(); sv = { MAGIC, n, n, 80, 80, 100, 0, 0, 0, false, false }; save(); rank = -1; hatchT = 0; }
  static void load() {
    prefs.begin("pet", false);
    if (prefs.getBytes("s", &sv, sizeof sv) != sizeof sv || sv.magic != MAGIC) newEgg();
    uint32_t n = now(), gap = n > sv.last ? n - sv.last : 0; if (gap > 12 * 3600) gap = 12 * 3600;   // 離線補算,最多算 12 小時
    for (; gap > 60; gap -= 60) tick(60); tick(gap);
    loaded = true; save();
  }
  void init() {
    if (!loaded) load(); else save();
    px = 160; dir = 1; phase = idleT = eatT = jumpY = jumpV = saveT = annoyT = dizzyT = tiltT = shakeT = 0; target = px; food.live = ball.live = false; mood = face::CALM;
    ringlib::reset(); cv.fillScreen(0);
  }
  static void action(int k) {   // 下排按鍵
    if (dead()) { newEgg(); spark(160, 120, rgb(255, 255, 255)); snd::note(500); return; }
    if (stage() == 0) return;
    switch (k) {
      case 0: if (sv.hunger >= 95) { annoyT = 1.5f; snd::note(200); } else if (!food.live) { food = { 60 + frand() * 200, true }; snd::note(400); } break;
      case 1: if (sv.sleeping) break; if (!ball.live) { ball = { 60 + frand() * 200, 0, 20, 0, true }; sv.energy = clamp(sv.energy - 8); snd::note(450); } break;
      case 2: sv.sleeping = !sv.sleeping; food.live = ball.live = false; snd::note(sv.sleeping ? 260 : 520); break;
      case 3: if (sv.poops) { sv.poops = 0; sv.happy = clamp(sv.happy + 5); spark(260, FLOOR - 4, rgb(120, 220, 120)); snd::note(600); } break;
    }
    save();
  }
  void step(const Ctx& c) {
    tick(c.dt); if ((saveT += c.dt) > 30) save();
    if (c.tap && c.ty >= BTN_Y) { action(c.tx * NBTN / W); return; }
    if (dead()) { mood = face::DEAD; if (c.tap) action(0); return; }
    if (stage() == 0) {   // 蛋:點一下搖一搖、提早 10 秒孵
      if (c.tap) { if (sv.born >= 10) sv.born -= 10; hatchT = 0.5f; snd::click(); }
      hatchT -= c.dt; if (stage() != 0) { spark(px, FLOOR - 20, rgb(255, 255, 200)); snd::note(700); buzz(120, 80); }
      return;
    }
    float s = stage() == 1 ? 0.75f : 1.0f;
    bool tilted = fabsf(c.gx) > 0.35f && !sv.sleeping;
    lean = tilted ? c.gx * 0.6f : 0;
    shakeT = c.shake > 0.5f ? shakeT + c.dt * 3 : fmaxf(0, shakeT - c.dt);   // 搖晃累積,停了慢慢消
    if (shakeT > 2 && dizzyT <= 0) { dizzyT = 3; sv.happy = clamp(sv.happy - 5); snd::note(180); buzz(120, 60); }
    if (tilted) { px += c.gx * 220 * c.dt; tiltT += c.dt; if (tiltT > 2 && dizzyT <= 0) { dizzyT = 2.5f; snd::note(180); } } else tiltT = 0;
    if (c.tap && fabsf(c.tx - px) < 22 && c.ty > FLOOR - 60 * s && c.ty < FLOOR) {   // 點牠
      if (sv.sleeping) { sv.sleeping = false; annoyT = 2.5f; snd::note(220); buzz(60, 30); }
      else if (jumpY >= 0) { jumpV = -220; sv.happy = clamp(sv.happy + 1); snd::note(520); }
    }
    jumpV += 900 * c.dt; jumpY += jumpV * c.dt; if (jumpY > 0) { jumpY = 0; jumpV = 0; }
    // 走路目標:食物 > 球 > 閒晃
    if (!sv.sleeping && dizzyT <= 0 && !tilted) {
      if (food.live) target = food.x; else if (ball.live) target = ball.x;
      else if ((idleT -= c.dt) <= 0) { idleT = 2 + frand() * 4; target = frand() < 0.5f ? px : 50 + frand() * 220; }
      float d = target - px;
      if (fabsf(d) > 4 && eatT <= 0) { dir = d > 0 ? 1 : -1; px += dir * WALK * c.dt; phase += c.dt * 10; }
    }
    if (px < 20) px = 20; if (px > 300) px = 300;
    if (food.live && fabsf(food.x - px) <= 6) {   // 吃:1.5 秒,每 0.5 秒咬一口
      if (eatT <= 0) eatT = 1.5f;
      float before = eatT; eatT -= c.dt;
      if ((int)(before * 2) != (int)(eatT * 2)) snd::click();
      if (eatT <= 0) { food.live = false; sv.hunger = clamp(sv.hunger + 30); sv.happy = clamp(sv.happy + 3); spark(px, FLOOR - 30 * s, rgb(255, 120, 120)); snd::note(500); save(); }
    } else eatT = 0;
    if (ball.live) {   // 球:傾斜滾動、撞牆反彈;追到就踢飛
      ball.vx += c.gx * 500 * c.dt; ball.vx *= 0.995f; ball.x += ball.vx * c.dt;
      if (ball.x < 12) { ball.x = 12; ball.vx = fabsf(ball.vx); } if (ball.x > 308) { ball.x = 308; ball.vx = -fabsf(ball.vx); }
      if (fabsf(ball.x - px) < 12 && fabsf(ball.vx) < 150) { ball.vx = (frand() < 0.5f ? -1 : 1) * (180 + frand() * 120); ball.catches++; sv.happy = clamp(sv.happy + 10); jumpV = -180; spark(ball.x, FLOOR - 6, rgb(255, 230, 0)); snd::note(400 + ball.catches * 40); buzz(40, 15); }
      if ((ball.t -= c.dt) <= 0 || ball.catches >= 6) { ball.live = false; save(); }
    }
    annoyT -= c.dt; dizzyT -= c.dt;
    mood = sv.sick > 600 ? face::SICK : sv.sleeping ? face::SLEEPY : eatT > 0 ? face::EATING : dizzyT > 0 ? face::DIZZY : annoyT > 0 ? face::ANNOYED : tilted ? face::SCARED
         : sv.hunger < 25 ? face::HUNGRY : sv.happy < 25 ? face::SAD : (jumpY < 0 || ball.live) ? face::HAPPY : sv.energy < 25 ? face::SLEEPY : sv.happy > 70 ? face::HAPPY : face::CALM;
    stepSparks(c.dt);
  }
  static void drawPet() {
    float s = stage() == 1 ? 0.75f : 1.0f; bool lie = sv.sleeping || dead() || dizzyT > 0;
    float ang = lie ? -1.5708f : lean, cs = cosf(ang), sn = sinf(ang);   // 傾斜時身體往滑的方向倒
    float hx = px, hy = lie ? FLOOR - 8 * s : FLOOR - 26 * s + jumpY;   // 腰的位置
    auto W2 = [&](float lx, float ly, int& x, int& y) { x = (int)(hx + (lx * cs - ly * sn) * s); y = (int)(hy + (lx * sn + ly * cs) * s); };
    uint32_t body = dead() ? rgb(120, 120, 120) : mood == face::SICK ? rgb(140, 220, 140) : sv.sleeping ? rgb(150, 150, 170) : rgb(255, 255, 255);
    float sw = (eatT > 0 || sv.sleeping || fabsf(target - px) <= 4) ? 0 : sinf(phase) * 5;   // 走路擺腿
    bool arms = mood == face::HAPPY && !lie;
    int ax, ay, bx, by;
    auto line = [&](float x0, float y0, float x1, float y1) { W2(x0, y0, ax, ay); W2(x1, y1, bx, by); cv.drawLine(ax, ay, bx, by, body); cv.drawLine(ax + 1, ay, bx + 1, by, body); };
    line(0, -22, 0, 0);                                                        // 脊椎
    line(0, -20, -8 * dir, arms ? -30 : -12); line(-8 * dir, arms ? -30 : -12, -14 * dir, arms ? -34 : -4);   // 後手
    line(0, -20, 8 * dir, arms ? -30 : -12);  line(8 * dir, arms ? -30 : -12, 14 * dir, arms ? -34 : -4);      // 前手
    line(0, 0, -6 + sw, 12); line(-6 + sw, 12, -8 + sw * 2, 26); line(0, 0, 6 - sw, 12); line(6 - sw, 12, 8 - sw * 2, 26);   // 腿
    int cx, cy; W2(0, -30, cx, cy); int r = (int)(8 * s);
    cv.fillCircle(cx, cy, r, rgb(30, 30, 40)); cv.drawCircle(cx, cy, r, body);
    face::draw(cx, cy, cs * dir, sn * dir, -sn, cs, mood, s);
  }
  void draw() {
    cv.fillScreen(sv.sleeping ? rgb(0, 0, 20) : 0);
    cv.drawFastHLine(0, FLOOR + 1, W, rgb(90, 110, 140));
    if (sv.sleeping) { cv.fillCircle(280, 40, 12, rgb(255, 240, 180)); cv.fillCircle(274, 36, 11, rgb(0, 0, 20)); for (int i = 0; i < 8; i++) cv.drawPixel(30 + i * 37, 20 + (i * 53) % 60, rgb(200, 200, 255)); }
    for (int i = 0; i < sv.poops; i++) { int x = 250 + i * 22; cv.fillCircle(x, FLOOR - 3, 4, rgb(140, 90, 40)); cv.fillCircle(x, FLOOR - 8, 3, rgb(140, 90, 40)); cv.fillCircle(x, FLOOR - 12, 2, rgb(140, 90, 40)); }
    if (food.live) { cv.fillCircle((int)food.x, FLOOR - 5, 5, rgb(255, 60, 60)); cv.drawLine((int)food.x, FLOOR - 10, (int)food.x + 2, FLOOR - 13, rgb(120, 200, 80)); }
    if (ball.live) { cv.fillCircle((int)ball.x, FLOOR - 6, 6, rgb(255, 200, 0)); cv.drawCircle((int)ball.x, FLOOR - 6, 6, rgb(255, 255, 255)); }
    if (stage() == 0) {   // 蛋
      int wob = hatchT > 0 ? (int)(sinf(hatchT * 40) * 3) : 0;
      cv.fillEllipse((int)px + wob, FLOOR - 16, 13, 17, rgb(240, 230, 200)); cv.fillCircle((int)px - 4 + wob, FLOOR - 20, 3, rgb(255, 250, 230));
      cv.setTextDatum(top_center); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0); char t[24]; snprintf(t, sizeof t, "hatching in %ds", (int)(60 - age())); cv.drawString(t, 160, 60);
    } else drawPet();
    if (dead()) { char s[20]; snprintf(s, sizeof s, "LIVED %lum", (unsigned long)(age() / 60)); board.draw(s, rank, "tap for a new egg"); }
    drawSparks();
    // 三條值 + 年齡
    static const char* LBL[3] = { "food", "mood", "rest" }; const float* val[3] = { &sv.hunger, &sv.happy, &sv.energy };
    static const uint32_t COL[3] = { rgb(255, 160, 0), rgb(255, 90, 160), rgb(80, 160, 255) };
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 160), 0);
    for (int i = 0; i < 3; i++) { cv.drawString(LBL[i], 4, 4 + i * 11); cv.drawRect(34, 5 + i * 11, 62, 7, rgb(80, 80, 90)); cv.fillRect(35, 6 + i * 11, (int)(*val[i] * 0.6f), 5, COL[i]); }
    char t[32]; uint32_t a = age(); snprintf(t, sizeof t, "%s  age %luh%02lum  %s", stage() == 1 ? "baby" : "adult", (unsigned long)(a / 3600), (unsigned long)(a / 60 % 60), face::NAME[mood]);
    cv.drawString(t, 4, 40);
    static const char* BTN[NBTN] = { "FEED", "PLAY", "SLEEP", "CLEAN" };
    for (int k = 0; k < NBTN; k++) {   // 下排按鍵
      int x = k * W / NBTN; bool on = (k == 0 && food.live) || (k == 1 && ball.live) || (k == 2 && sv.sleeping) || (k == 3 && sv.poops);
      cv.drawRoundRect(x + 4, BTN_Y, W / NBTN - 8, H - BTN_Y - 4, 5, on ? rgb(255, 230, 0) : rgb(120, 120, 140));
      cv.setTextDatum(middle_center); cv.setTextColor(on ? rgb(255, 230, 0) : rgb(200, 200, 200), 0); cv.drawString(k == 2 && sv.sleeping ? "WAKE" : BTN[k], x + W / NBTN / 2, (BTN_Y + H - 4) / 2);
    }
  }
}
