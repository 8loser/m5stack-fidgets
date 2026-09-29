#pragma once
#include "common.h"

// ================= 修車廠 =================
// 180 秒內修好幾台車,前 5 名存 NVS。每台車開進來帶幾個故障,全景點零件進去修,A 回全景,C 試車。
//   輪胎(點輪子):沒氣就拉打氣筒(上下拉)進綠區,打過頭爆胎;扎釘子(打了也漏)或爆胎要換:按住螺帽用氣動扳手拆光 5 顆、撥掉輪胎,新胎再逐顆鎖回
//   油箱(點油箱蓋):往右傾或按住油桶倒油到綠線,太多會溢出來
//   電瓶(點引擎蓋):紅夾子拖到 +、黑夾子拖到 -,夾錯噴火花;接對就充電。沒電時前後燈都不亮
//   大燈(點大燈):燈泡破了就繞圈往左轉下來、把新燈泡拖進燈座、往右轉鎖緊。只有大燈不亮是燈泡、前後都不亮是電瓶
//   凹痕:在車門上直接點(鐵鎚敲),敲平;好好的車門亂敲會敲出新凹痕
//   髒污:手指在泥巴上來回擦
// 試車:每種沒修好的都有自己的樣子(發不動、輪胎啪啪顛、輪子掉下來擦出火花、漏氣、打太飽一路彈跳、沒油冒黑煙熄火、
//   車門掉下來、大燈壞了天黑撞到三角錐、髒到蒼蠅繞著飛),開回來再修;全部好了就開走,換下一台
namespace garage {
  enum { ARRIVE, SHOP, TIRE, FUEL, BATT, LAMP, DRIVE, OVER };
  enum { TIGHT, OUT, LOOSE };   // 燈泡
  constexpr int NN = 5, ND = 64, NM = 7;
  constexpr float ROUND = 180, DRIVE_T = 4, WY = 176, GROUND = 198, WR = 22, PSI_LO = 85, PSI_HI = 130, PSI_POP = 145, PSI_MAX = 160;
  constexpr float FX = 70, FY = 142, DX = 165, DY = 148, HX = 262, HY = 142;   // 全景:油箱蓋、車門凹痕、大燈
  constexpr float TX = 118, TY = 128, TR = 86, NR = 28;                        // 輪胎特寫
  constexpr float LX = 160, LY = 110;                                          // 燈座特寫
  static const float WX[2] = { 100, 222 };                                     // 後輪、前輪;車頭朝右
  static const float CRX[2] = { 250, 295 }, CRY = 70, TMX[2] = { 130, 190 }, TMY = 106;   // 電瓶:夾子的原位、+ / - 端子
  struct Tire { float psi; int dmg; bool nut[NN]; };   // dmg 1 = 釘子、2 = 爆胎;nut true = 鎖著
  static Tire tire[2];
  static float fuel, dent, batt, left, st, carX, road, hy, drillT, tickT, slideT, pour, spinA, grabX, hamT, lastX, lastY, twist, lastA, bx, by;
  static int state, cur, cars, rank, drillK, bulb, grab, clampAt[2]; static bool broken, armed, drilling, grabbing, twisting, pass;
  static float clX[2], clY[2];
  static struct { float x, y, r, a; } mud[NM];
  // 試車的道具:掉下來的車門、被撞飛的三角錐
  static bool dead, night, doorOff, coneHit; static float doorX, doorY, doorVy, doorA, coneX, coneY, coneVy, coneA, hop;
  static uint32_t body; static Board board = { "garage" };
  static struct { float x, y, vx, vy, g; uint32_t c; } drop[ND]; static int nd;

  static bool nutsOn(const Tire& w) { for (bool n : w.nut) if (!n) return false; return true; }
  static bool tireOk(const Tire& w) { return !w.dmg && w.psi >= PSI_LO && w.psi <= PSI_HI && nutsOn(w); }
  static bool clean() { for (auto& m : mud) if (m.a > 0.05f) return false; return true; }
  static bool lampOk() { return !broken && bulb == TIGHT; }
  static bool allOk() { return tireOk(tire[0]) && tireOk(tire[1]) && fuel >= 0.9f && dent < 0.05f && clean() && batt >= 0.95f && lampOk(); }
  static void spill(float x, float y, uint32_t c, float v, float g = 500) { if (nd < ND) { float a = frand() * 3.14f; drop[nd++] = { x, y, cosf(a) * v, -sinf(a) * v, g, c }; } }
  static void newCar() {
    for (auto& w : tire) { w.psi = 100 + frand() * 15; w.dmg = 0; for (bool& n : w.nut) n = true; }
    fuel = 0.95f; dent = 0; batt = 1; broken = false; bulb = TIGHT; body = hsv(frand());
    for (auto& m : mud) m.a = 0;
    int p[7] = { 0, 1, 2, 3, 4, 5, 6 }, nf = cars == 0 ? 1 : cars == 1 ? 2 : cars < 4 ? 2 + (int)(frand() * 2) : 2 + (int)(frand() * 3);
    for (int i = 6; i > 0; i--) { int j = (int)(frand() * (i + 1)); int t = p[i]; p[i] = p[j]; p[j] = t; }
    for (int k = 0; k < nf; k++) switch (p[k]) {
      case 0: case 1: tire[p[k]].psi = 35 + frand() * 25; if (frand() < 0.5f) tire[p[k]].dmg = 1; break;
      case 2: fuel = 0.05f + frand() * 0.2f; break;
      case 3: dent = 1; break;
      case 4: for (auto& m : mud) { do { m.x = 62 + frand() * 180; m.y = 132 + frand() * 16; } while (fabsf(m.x - FX) < 18 || fabsf(m.x - DX) < 16); m.r = 7 + frand() * 6; m.a = 1; } break;
      case 5: batt = 0.1f; break;
      case 6: broken = true; break;
    }
    state = ARRIVE; st = 0; carX = -240;
  }
  void init() { cars = 0; left = ROUND; rank = -1; nd = 0; grab = -1; slideT = 0; newCar(); }
  static void go(int s) { state = s; st = 0; drilling = grabbing = twisting = false; grab = -1; snd::click(); }
  static float nutX(int k) { return TX + cosf(k * 1.2566f - 1.5708f) * NR; }
  static float nutY(int k) { return TY + sinf(k * 1.2566f - 1.5708f) * NR; }
  static bool near(float x, float y, float px, float py, float r) { return (x - px) * (x - px) + (y - py) * (y - py) < r * r; }

  static void stepTire(const Ctx& c) {
    Tire& w = tire[cur];
    if (w.dmg == 1) w.psi = fmaxf(15, w.psi - 30 * c.dt);   // 釘子一直漏
    if (slideT > 0) {   // 舊胎滾出去、新胎滾進來,中途換成新胎(螺帽還沒鎖)
      float prev = slideT; slideT -= c.dt;
      if (prev > 0.5f && slideT <= 0.5f) { w.psi = 105; w.dmg = 0; }
      return;
    }
    if (c.tap) {
      drilling = grabbing = false;
      for (int k = 0; k < NN; k++) if (near(c.tx, c.ty, nutX(k), nutY(k), 18)) { drilling = true; drillK = k; drillT = 0; }
      if (!drilling && c.tx < 236 && near(c.tx, c.ty, TX, TY, TR)) { grabbing = true; grabX = c.tx; }
    }
    if (!c.touch) drilling = grabbing = false;
    if (drilling) {   // 氣動扳手:按住 0.45 秒拆下或鎖上
      drillT += c.dt; spinA += (w.nut[drillK] ? -30 : 30) * c.dt; buzz(90, 40);
      if ((tickT += c.dt) > 0.07f) { tickT = 0; snd::click(); }
      if (drillT >= 0.45f) { w.nut[drillK] = !w.nut[drillK]; drilling = false; snd::note(w.nut[drillK] ? 523 : 392); buzz(180, 60); }
    }
    bool allOff = true; for (bool n : w.nut) if (n) allOff = false;
    if (grabbing && allOff && fabsf(c.tx - grabX) > 50) { grabbing = false; slideT = 1; snd::note(262); buzz(120, 80); }
    if (c.touch && c.tx >= 236) {   // 打氣筒:把手拉到上面再壓到底算一下
      hy = fminf(200, fmaxf(100, (float)c.ty));
      if (hy < 120) armed = true;
      if (hy > 190 && armed) {
        armed = false; buzz(60, 20); snd::click();
        if (w.dmg != 2 && (w.psi += 14) > PSI_POP) { w.psi = 0; w.dmg = 2; buzz(255, 300); snd::note(130); for (int i = 0; i < 12; i++) spill(TX + 40, TY - 70, rgb(40, 40, 40), 150 + frand() * 150); }
      }
    }
  }

  static void stepFuel(const Ctx& c) {
    pour = fmaxf(0, fminf(0.5f, (c.gx - 0.25f) * 0.8f));   // 往右傾倒油
    if (c.touch && c.tx < 130 && c.ty < 110) pour = fmaxf(pour, 0.3f);   // 或按住油桶
    if (pour <= 0) return;
    fuel += pour * c.dt;
    if ((tickT += c.dt) > 0.18f) { tickT = 0; snd::note(200 + fuel * 500); }   // 咕嘟聲越裝越高
    if (fuel > 1) { fuel = 1; spill(150, 120, rgb(255, 170, 30), 60 + frand() * 80); buzz(40, 20); }
  }

  static void stepBatt(const Ctx& c) {
    if (c.tap) for (int i = 0; i < 2; i++) if (near(c.tx, c.ty, clX[i], clY[i], 22)) { grab = i; clampAt[i] = -1; }
    if (grab >= 0) {
      if (c.touch) { clX[grab] = c.tx; clY[grab] = c.ty; }
      else {   // 放手:靠近端子就夾上,夾錯噴火花彈回去
        int i = grab; grab = -1; clX[i] = CRX[i]; clY[i] = CRY;
        for (int t = 0; t < 2; t++) if (near(lastX, lastY, TMX[t], TMY, 22)) {
          if (t == i) { clampAt[i] = t; clX[i] = TMX[t]; clY[i] = TMY - 6; snd::click(); buzz(80, 30); }
          else { for (int k = 0; k < 14; k++) spill(TMX[t], TMY, frand() < 0.5f ? rgb(255, 230, 60) : rgb(255, 255, 255), 100 + frand() * 200); buzz(255, 250); snd::note(140); }
        }
      }
    }
    if (clampAt[0] == 0 && clampAt[1] == 1 && batt < 1) {
      batt += 0.35f * c.dt;
      if ((tickT += c.dt) > 0.2f) { tickT = 0; snd::note(300 + batt * 400); }
      if (batt >= 1) { batt = 1; for (int i = 0; i < 2; i++) { clampAt[i] = -1; clX[i] = CRX[i]; clY[i] = CRY; } snd::note(784); buzz(150, 100); }
    }
  }

  static void stepLamp(const Ctx& c) {
    if (c.tap) {
      if (bulb == OUT && near(c.tx, c.ty, bx, by, 26)) grab = 0;
      else if (bulb != OUT && near(c.tx, c.ty, LX, LY, 80)) { twisting = true; lastA = atan2f(c.ty - LY, c.tx - LX); }
    }
    if (grab == 0) {
      if (c.touch) { bx = c.tx; by = c.ty; }
      else { grab = -1; if (near(bx, by, LX, LY, 30)) { bulb = LOOSE; broken = false; twist = 0; snd::click(); } bx = LX; by = 212; }
    }
    if (!c.touch) twisting = false;
    if (!twisting) return;
    float a = atan2f(c.ty - LY, c.tx - LX), da = a - lastA; lastA = a;
    if (da > 3.1416f) da -= 6.2832f; if (da < -3.1416f) da += 6.2832f;
    float before = twist;
    if (bulb == TIGHT) twist = fminf(0, twist + da);   // 往左(逆時針)轉鬆
    else twist = fmaxf(0, twist + da);                 // 往右(順時針)鎖緊
    if (floorf(before * 2) != floorf(twist * 2)) { snd::click(); buzz(60, 20); }
    if (bulb == TIGHT && twist < -6.2832f) {   // 轉下來,舊燈泡掉下去碎掉
      bulb = OUT; twisting = false; bx = LX; by = 212; snd::note(880); buzz(150, 80);
      for (int k = 0; k < 10; k++) spill(LX, LY + 20, rgb(200, 230, 255), 60 + frand() * 120);
    }
    if (bulb == LOOSE && twist > 6.2832f) { bulb = TIGHT; twist = 0; twisting = false; snd::note(659); buzz(150, 60); }
  }

  static void shopTap(const Ctx& c) {
    if (near(c.tx, c.ty, HX, HY, 14)) { bx = LX; by = 212; twist = 0; go(LAMP); return; }
    if (c.tx > 216 && c.tx < 254 && c.ty > 116 && c.ty < 138) { for (int i = 0; i < 2; i++) { clampAt[i] = -1; clX[i] = CRX[i]; clY[i] = CRY; } go(BATT); return; }
    for (int i = 0; i < 2; i++) if (near(c.tx, c.ty, WX[i], WY, 28)) { cur = i; hy = 100; armed = false; go(TIRE); return; }
    if (near(c.tx, c.ty, FX, FY, 26)) { go(FUEL); return; }
    if (near(c.tx, c.ty, DX, DY, 20)) {   // 鐵鎚:敲凹痕變平,平的亂敲會敲凹
      dent = dent > 0.05f ? fmaxf(0, dent - 0.22f) : 0.3f; hamT = 0.15f; buzz(200, 40); snd::click();
    }
  }

  static void shopRub(const Ctx& c) {   // 手指來回擦泥巴,邊擦邊冒泡泡
    float mv = sqrtf((c.tx - lastX) * (c.tx - lastX) + (c.ty - lastY) * (c.ty - lastY));
    if (mv < 1 || mv > 60) return;
    for (auto& m : mud) if (m.a > 0 && near(c.tx, c.ty, m.x, m.y, m.r + 8)) {
      m.a = fmaxf(0, m.a - mv * 0.012f);
      if (frand() < 0.3f) spill(c.tx, c.ty, rgb(255, 255, 255), 30 + frand() * 40, -40);
      if ((tickT += c.dt) > 0.1f) { tickT = 0; snd::click(); }
    }
  }

  static void startDrive() {
    pass = allOk(); dead = batt < 0.95f; night = !lampOk(); doorOff = coneHit = false;
    coneX = W + 360; coneY = GROUND - 14; coneA = 0; hop = 0; road = 0; carX = 0; go(DRIVE);
  }

  static void stepDrive(const Ctx& c) {
    if (dead) {   // 電瓶沒電:發不動,喀喀喀
      if ((tickT += c.dt) > 0.15f) { tickT = 0; snd::click(); buzz(60, 30); }
      if (st >= DRIVE_T) { go(SHOP); }
      return;
    }
    bool flat = false, lost = false, bouncy = false;
    for (int i = 0; i < 2; i++) {
      const Tire& w = tire[i]; float x = WX[i] + carX;
      if (!nutsOn(w)) { if (st > 0.9f) { lost = true; if (frand() < 0.5f) spill(x, GROUND - 4, rgb(255, 200, 40), 80 + frand() * 80); } continue; }   // 輪子掉了,車底擦出火花
      if (w.dmg == 1 && st < 1.2f && frand() < 0.4f) spill(x, WY + 10, rgb(220, 220, 220), 40 + frand() * 40, -60);   // 釘子漏氣
      if (w.dmg == 2 || w.psi < PSI_LO || (w.dmg == 1 && st > 1)) flat = true;
      else if (w.psi > PSI_HI) bouncy = true;
    }
    bool stall = fuel < 0.9f && st > 1.2f;
    float v = flat || lost ? 110 : 200;
    if (stall) { v = fmaxf(0, v - (st - 1.2f) * 300); if (frand() < 0.3f) spill(carX + 48, 156, rgb(50, 50, 50), 30 + frand() * 30, -80); }   // 冒黑煙
    road += v * c.dt;
    float prevHop = hop; hop = bouncy ? -fabsf(sinf(st * 7)) * 18 : 0;
    if (bouncy && prevHop < -3 && hop >= -3) snd::note(600);   // 打太飽:一路彈跳
    if (flat && (tickT += c.dt) > 0.2f) { tickT = 0; snd::click(); buzz(80, 30); }   // 扁胎啪啪啪
    if (stall && (tickT += c.dt) > 0.3f) { tickT = 0; snd::click(); }
    if (dent > 0.05f && st > 1.5f && !doorOff) { doorOff = true; doorX = DX + carX; doorY = DY; doorVy = -140; doorA = 0; snd::note(150); buzz(160, 100); }   // 車門掉下來
    if (doorOff) { doorVy += 500 * c.dt; doorY += doorVy * c.dt; doorX -= v * c.dt; doorA += 4 * c.dt; if (doorY > GROUND - 8) { doorY = GROUND - 8; doorVy *= -0.3f; } }
    if (night) {   // 大燈壞了:天黑看不到三角錐,撞飛它
      if (!coneHit) { coneX -= v * c.dt; if (coneX < carX + 272) { coneHit = true; coneVy = -320; snd::note(200); buzz(200, 80); } }
      else { coneVy += 500 * c.dt; coneY += coneVy * c.dt; coneX += 250 * c.dt; coneA += 10 * c.dt; }
    }
    if (pass && st > 2.5f) carX += 400 * c.dt;
    if (st >= DRIVE_T) {
      if (pass) { cars++; snd::note(523); buzz(200, 150); newCar(); }
      else { carX = 0; go(SHOP); }
    }
  }

  void step(const Ctx& c) {
    st += c.dt; hamT -= c.dt;
    for (int i = 0; i < nd; i++) { auto& d = drop[i]; d.vy += d.g * c.dt; d.x += d.vx * c.dt; d.y += d.vy * c.dt; if (d.y > H || d.y < 0) drop[i--] = drop[--nd]; }
    if (state == OVER) { if (st > 1.5f && c.tap) init(); return; }
    if ((left -= c.dt) <= 0) { left = 0; state = OVER; st = 0; rank = board.record(cars); buzz(150, 150); snd::note(300); return; }
    if (c.tap) { lastX = c.tx; lastY = c.ty; }
    bool busy = slideT > 0 || grab >= 0;
    if (c.tapC && state >= SHOP && state <= LAMP && !busy) { startDrive(); return; }
    if (c.tapA && state > SHOP && state <= LAMP && !busy) { go(SHOP); return; }
    switch (state) {
      case ARRIVE: {
        float u = fminf(st, 1); carX = -240 * (1 - u) * (1 - u);
        if (fuel < 0.5f && st < 1 && (tickT += c.dt) > 0.12f) { tickT = 0; snd::click(); }   // 沒油:噗噗噗
        if (st > 1.1f) { carX = 0; state = SHOP; st = 0; }
        break;
      }
      case SHOP: if (c.tap) shopTap(c); else if (c.touch) shopRub(c); break;
      case TIRE: stepTire(c); break;
      case FUEL: stepFuel(c); break;
      case BATT: stepBatt(c); break;
      case LAMP: stepLamp(c); break;
      case DRIVE: stepDrive(c); break;
    }
    if (c.touch) { lastX = c.tx; lastY = c.ty; }
  }

  // ---- 畫 ----
  static void fillRot(float cx, float cy, float w, float h, float a, uint32_t col) {
    float ca = cosf(a), sa = sinf(a), px[4], py[4]; static const float u[4] = { -1, 1, 1, -1 }, v[4] = { -1, -1, 1, 1 };
    for (int k = 0; k < 4; k++) { px[k] = cx + u[k] * w / 2 * ca - v[k] * h / 2 * sa; py[k] = cy + u[k] * w / 2 * sa + v[k] * h / 2 * ca; }
    cv.fillTriangle((int)px[0], (int)py[0], (int)px[1], (int)py[1], (int)px[2], (int)py[2], col);
    cv.fillTriangle((int)px[0], (int)py[0], (int)px[2], (int)py[2], (int)px[3], (int)py[3], col);
  }
  static void drawWheel(float x, float y, const Tire& w, float rot) {
    float sq = w.dmg == 2 ? 9 : w.psi < PSI_LO ? (PSI_LO - w.psi) / PSI_LO * 9 : 0;   // 沒氣就扁
    cv.fillEllipse((int)x, (int)(y + sq), (int)(WR + sq * 0.5f), (int)(WR - sq), rgb(25, 25, 25));
    cv.fillCircle((int)x, (int)(y + sq), (int)(WR * 0.55f), rgb(170, 170, 180));
    cv.drawLine((int)x, (int)(y + sq), (int)(x + cosf(rot) * WR * 0.55f), (int)(y + sq + sinf(rot) * WR * 0.55f), rgb(90, 90, 100));
  }
  static void drawCar(float ox, float oy, float rot, const float* lost) {
    uint32_t glass = rgb(150, 210, 255), dark = rgb(40, 40, 45); bool power = batt >= 0.95f;
    cv.fillRoundRect((int)(ox + 50), (int)(oy + 128), 220, 36, 8, body);
    cv.fillRoundRect((int)(ox + 105), (int)(oy + 96), 110, 38, 10, body);
    cv.fillRoundRect((int)(ox + 113), (int)(oy + 102), 44, 26, 4, glass); cv.fillRoundRect((int)(ox + 163), (int)(oy + 102), 44, 26, 4, glass);
    if (doorOff && state == DRIVE) cv.fillRect((int)(ox + DX - 22), (int)(oy + 132), 44, 28, dark);   // 車門掉了的洞
    else {
      cv.drawRect((int)(ox + DX - 22), (int)(oy + 132), 44, 28, rgb(0, 0, 0));
      if (dent > 0.05f) { cv.fillEllipse((int)(ox + DX), (int)(oy + DY), (int)(4 + dent * 10), (int)(3 + dent * 6), rgb(0, 0, 0)); cv.drawEllipse((int)(ox + DX), (int)(oy + DY), (int)(6 + dent * 12), (int)(5 + dent * 8), rgb(90, 90, 90)); }
    }
    cv.drawLine((int)(ox + 218), (int)(oy + 131), (int)(ox + 254), (int)(oy + 131), rgb(0, 0, 0));   // 引擎蓋縫
    for (auto& m : mud) if (m.a > 0.05f) cv.fillCircle((int)(ox + m.x), (int)(oy + m.y), (int)(m.r * (0.4f + 0.6f * m.a)), m.a > 0.5f ? rgb(100, 65, 25) : rgb(150, 110, 60));
    cv.fillCircle((int)(ox + HX), (int)(oy + HY), 6, power && lampOk() ? rgb(255, 240, 120) : rgb(90, 90, 80));   // 大燈
    cv.fillRect((int)(ox + 50), (int)(oy + 134), 5, 10, power ? rgb(255, 30, 30) : rgb(80, 30, 30));            // 尾燈
    cv.fillCircle((int)(ox + FX), (int)(oy + FY), 6, rgb(60, 60, 60)); cv.drawCircle((int)(ox + FX), (int)(oy + FY), 7, rgb(200, 200, 200));
    for (int i = 0; i < 2; i++) drawWheel(ox + WX[i] + lost[i], (lost[i] > 0 ? 0 : oy) + WY, tire[i], rot + lost[i] / WR);
  }

  static void drawTire() {
    const Tire& w = tire[cur]; uint32_t bg = rgb(30, 30, 40);
    float ox = slideT > 0.5f ? -(1 - (slideT - 0.5f) * 2) * 230 : slideT > 0 ? -slideT * 2 * 230 : 0, x = TX + ox;
    cv.fillCircle((int)x, (int)TY, (int)TR, rgb(28, 28, 28));
    for (int k = 0; k < 28; k++) { float a = k * 0.2244f; cv.drawLine((int)(x + cosf(a) * 76), (int)(TY + sinf(a) * 76), (int)(x + cosf(a) * TR), (int)(TY + sinf(a) * TR), rgb(60, 60, 60)); }
    cv.fillCircle((int)x, (int)TY, 56, rgb(170, 170, 180)); cv.fillCircle((int)x, (int)TY, 42, rgb(120, 120, 130)); cv.fillCircle((int)x, (int)TY, 14, rgb(90, 90, 100));
    if (w.dmg == 1) { float a = -0.7f; cv.drawWideLine((int)(x + cosf(a) * 70), (int)(TY + sinf(a) * 70), (int)(x + cosf(a) * (TR + 8)), (int)(TY + sinf(a) * (TR + 8)), 2, rgb(200, 200, 210)); cv.fillCircle((int)(x + cosf(a) * (TR + 8)), (int)(TY + sinf(a) * (TR + 8)), 5, rgb(220, 220, 230)); }
    if (w.dmg == 2) cv.fillArc((int)x, (int)TY, 60, (int)TR + 1, -80, -50, bg);   // 爆開的破口
    for (int k = 0; k < NN; k++) {
      float nx = nutX(k) + ox, ny = nutY(k);
      if (w.nut[k]) { cv.fillCircle((int)nx, (int)ny, 7, rgb(210, 210, 220)); cv.drawCircle((int)nx, (int)ny, 4, rgb(120, 120, 130)); }
      else cv.drawCircle((int)nx, (int)ny, 7, rgb(70, 70, 80));
      if (drilling && k == drillK) {   // 氣動扳手
        cv.drawWideLine((int)nx, (int)ny, (int)(nx + 30), (int)(ny + 40), 6, rgb(240, 180, 30));
        cv.drawLine((int)nx, (int)ny, (int)(nx + cosf(spinA) * 10), (int)(ny + sinf(spinA) * 10), rgb(0, 0, 0));
      }
    }
    // 壓力表:上半圓 0..PSI_MAX,綠區 PSI_LO..PSI_HI
    int gx = 272, gy = 46;
    cv.fillArc(gx, gy, 22, 30, 180, 180 + PSI_LO / PSI_MAX * 180, rgb(200, 60, 60));
    cv.fillArc(gx, gy, 22, 30, 180 + PSI_LO / PSI_MAX * 180, 180 + PSI_HI / PSI_MAX * 180, rgb(60, 200, 90));
    cv.fillArc(gx, gy, 22, 30, 180 + PSI_HI / PSI_MAX * 180, 360, rgb(200, 60, 60));
    float na = 3.1416f * (1 + fminf(w.psi, PSI_MAX) / PSI_MAX);
    cv.drawWideLine(gx, gy, (int)(gx + cosf(na) * 28), (int)(gy + sinf(na) * 28), 1.5f, rgb(255, 255, 255));
    char s[8]; snprintf(s, sizeof s, "%d", (int)w.psi); cv.setTextDatum(top_center); cv.setTextSize(1); cv.setTextColor(rgb(220, 220, 220), bg); cv.drawString(s, gx, gy + 4);
    // 打氣筒
    cv.fillRect(264, 140, 24, 90, rgb(80, 120, 200));
    cv.drawWideLine(276, (int)hy, 276, 140, 2, rgb(200, 200, 200));
    cv.fillRoundRect(250, (int)hy - 5, 52, 10, 3, rgb(240, 140, 40));
    cv.drawLine(264, 225, (int)(TX + 60), (int)(TY + 70), rgb(20, 20, 20));
  }

  static void drawFuel() {
    uint32_t bg = rgb(30, 30, 40);
    cv.fillRect(0, 110, 230, 130, body); cv.fillCircle(150, 132, 13, rgb(20, 20, 20)); cv.drawCircle(150, 132, 15, rgb(200, 200, 200));
    float a = pour > 0 ? 0.5f + pour * 1.2f : 0.2f, cx = 70, cy = 60;
    fillRot(cx, cy, 64, 46, a, rgb(210, 40, 40));
    float sx = cx + 32 * cosf(a) + 23 * sinf(a), sy = cy + 32 * sinf(a) - 23 * cosf(a);   // 壺嘴在右上角
    cv.drawWideLine((int)sx, (int)sy, (int)(sx + 14 * cosf(a - 0.8f)), (int)(sy + 14 * sinf(a - 0.8f)), 3, rgb(160, 30, 30));
    if (pour > 0) cv.drawWideLine((int)(sx + 14 * cosf(a - 0.8f)), (int)(sy + 14 * sinf(a - 0.8f)), 150, 132, 1 + pour * 6, rgb(255, 170, 30));
    int top = 40, h = 160; cv.drawRect(262, top, 24, h, rgb(200, 200, 200));
    int fh = (int)(fuel * (h - 4)); cv.fillRect(264, top + h - 2 - fh, 20, fh, fuel >= 0.9f ? rgb(60, 200, 90) : rgb(255, 170, 30));
    cv.drawFastHLine(256, top + h - 2 - (int)(0.9f * (h - 4)), 36, rgb(60, 255, 90));
    cv.setTextDatum(top_center); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 170), bg); cv.drawString("tilt right or hold the can", W / 2, 16);
  }

  static void drawBatt() {
    uint32_t bg = rgb(30, 30, 40);
    cv.fillRoundRect(90, 112, 140, 90, 6, rgb(50, 50, 60));
    cv.fillRect(TMX[0] - 8, TMY - 6, 16, 12, rgb(220, 40, 40)); cv.fillRect(TMX[1] - 8, TMY - 6, 16, 12, rgb(90, 90, 90));
    cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(rgb(255, 255, 255));
    cv.drawString("+", TMX[0], 130); cv.drawString("-", TMX[1], 130);
    int bw = (int)(batt * 100); cv.drawRect(109, 160, 102, 20, rgb(200, 200, 200)); cv.fillRect(110, 161, bw, 18, batt >= 0.95f ? rgb(60, 200, 90) : batt > 0.4f ? rgb(255, 200, 40) : rgb(220, 50, 50));
    cv.fillRoundRect(245, 175, 70, 50, 6, rgb(110, 110, 120));   // 充電器
    for (int i = 0; i < 2; i++) {
      uint32_t cc = i == 0 ? rgb(220, 40, 40) : rgb(25, 25, 25), edge = i == 0 ? rgb(255, 120, 120) : rgb(150, 150, 150);
      cv.drawWideLine(262 + i * 36, 175, (int)clX[i], (int)clY[i], 5, edge);   // 淺色外框,黑線在深色背景上才看得到
      cv.drawWideLine(262 + i * 36, 175, (int)clX[i], (int)clY[i], 3, cc);
      cv.fillRoundRect((int)clX[i] - 9, (int)clY[i] - 6, 18, 12, 3, cc); cv.drawRoundRect((int)clX[i] - 9, (int)clY[i] - 6, 18, 12, 3, edge);
    }
    cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 170), bg); cv.drawString("red to +   black to -", W / 2, 20);
  }

  static void drawLamp() {
    uint32_t bg = rgb(30, 30, 40); bool lit = lampOk() && batt >= 0.95f;
    cv.fillCircle((int)LX, (int)LY, 70, rgb(60, 60, 70)); cv.fillCircle((int)LX, (int)LY, 58, lit ? rgb(255, 250, 200) : rgb(170, 170, 180));
    cv.fillCircle((int)LX, (int)LY, 14, rgb(40, 40, 40));
    if (bulb != OUT) {
      cv.fillCircle((int)LX, (int)LY, 26, lit ? rgb(255, 240, 120) : rgb(210, 220, 230));
      float a = twist + (bulb == LOOSE ? 1.0f : 0); cv.drawWideLine((int)LX, (int)LY, (int)(LX + cosf(a) * 20), (int)(LY + sinf(a) * 20), 2, rgb(120, 120, 130));   // 轉的記號
      if (broken) { cv.drawLine((int)LX - 14, (int)LY - 16, (int)LX + 2, (int)LY + 4, rgb(60, 60, 60)); cv.drawLine((int)LX + 2, (int)LY + 4, (int)LX + 16, (int)LY - 6, rgb(60, 60, 60)); cv.drawLine((int)LX + 2, (int)LY + 4, (int)LX - 4, (int)LY + 20, rgb(60, 60, 60)); }
    } else cv.fillCircle((int)bx, (int)by, 22, rgb(230, 240, 250));   // 新燈泡
    const char* hint = bulb == OUT ? "drag the new bulb in" : bulb == LOOSE ? "twist right to tighten" : broken ? "twist left to take it out" : "";
    cv.setTextDatum(top_center); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 170), bg); cv.drawString(hint, W / 2, 16);
  }

  void draw() {
    static const float none[2] = { 0, 0 };
    uint32_t bg = state == DRIVE && night && !dead ? rgb(5, 5, 25) : rgb(30, 30, 40); cv.fillScreen(bg);
    if (state == TIRE) drawTire();
    else if (state == FUEL) drawFuel();
    else if (state == BATT) drawBatt();
    else if (state == LAMP) drawLamp();
    else if (state == DRIVE) {
      cv.fillRect(0, (int)GROUND, W, H - (int)GROUND, night && !dead ? rgb(25, 25, 35) : rgb(60, 60, 65));
      for (int x = -(int)road % 60; x < W; x += 60) cv.fillRect(x, (int)GROUND + 18, 30, 4, rgb(230, 230, 230));
      float lost[2], oy = hop; bool flat = false;
      for (int i = 0; i < 2; i++) { lost[i] = !dead && !nutsOn(tire[i]) && st > 0.9f ? (st - 0.9f) * 260 : 0; flat |= !dead && (tire[i].dmg || tire[i].psi < PSI_LO); }
      if (flat && sinf(st * 40) > 0.6f) oy -= 4;
      if (lost[0] > 0 || lost[1] > 0) oy += 8;
      if (dead) oy += sinf(st * 60) * 1.5f;
      drawCar(carX, oy, road / WR, lost);
      if (doorOff) fillRot(doorX, doorY, 44, 28, doorA, body);
      if (night && !dead) { fillRot(coneX, coneY, 14, 28, coneA, rgb(255, 120, 20)); cv.fillRect((int)coneX - 7, (int)coneY - 2, 14, 4, rgb(255, 255, 255)); }
      if (!clean()) for (int k = 0; k < 6; k++) {   // 蒼蠅
        float a = st * (5 + k) + k * 1.1f; cv.fillCircle((int)(carX + 160 + cosf(a) * (60 + k * 12)), (int)(oy + 120 + sinf(a * 1.3f) * 30), 2, rgb(10, 10, 10));
      }
      if (st > 3) { cv.setTextDatum(middle_center); cv.setTextSize(3); cv.setTextColor(pass ? rgb(60, 255, 90) : rgb(255, 80, 80)); cv.drawString(pass ? "OK!" : "?!", W / 2, 60); }
    } else {
      cv.fillRect(0, (int)GROUND, W, H - (int)GROUND, rgb(70, 70, 80));
      drawCar(carX, 0, 0, none);
      if (hamT > 0) fillRot(DX + 14, DY - 16, 10, 34, -0.8f, rgb(150, 100, 50));   // 鐵鎚
      if (cars == 0 && state == SHOP) { cv.setTextDatum(bottom_center); cv.setTextSize(1); cv.setTextColor(rgb(200, 200, 210)); cv.drawString("tap wheels, fuel cap, hood, light   C = test drive", W / 2, H - 4); }
    }
    for (int i = 0; i < nd; i++) cv.fillRect((int)drop[i].x, (int)drop[i].y, 4, 4, drop[i].c);
    if (state > SHOP && state <= LAMP) { cv.setTextDatum(bottom_left); cv.setTextSize(1); cv.setTextColor(rgb(160, 160, 170), bg); cv.drawString("A back  C test drive", 4, H - 4); }
    char t[24]; snprintf(t, sizeof t, "time %d  cars %d", (int)ceilf(left), cars);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(220, 220, 220), bg); cv.drawString(t, 4, 4);
    if (state == OVER) { snprintf(t, sizeof t, "CARS %d", cars); board.draw(t, rank); }
  }
}
