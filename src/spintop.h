#pragma once
#include "ringlib.h"

// ================= 39. 戰鬥陀螺 =================
// Beyblade X 式的戰鬥盤(畫成橫向橢圓碗,配合橫螢幕):前方(螢幕下緣)左右兩角是出場口袋(2 分)、正中間是 3 分洞(3 分),
// 碗邊一整圈加速軌,碰到就沿軌道衝刺後甩向中央。先到 4 分贏一場,輸一場結束,比連勝場數,前 5 名排行
// 發射:上方力量條的指標來回跑,點螢幕停住決定轉速(右端是完美區),點的位置就是落點(限中央發射口內,電腦落在對稱的另一側)
// 對戰:傾斜推自己的陀螺(以發射當下的姿勢為平)、點螢幕朝手指衝撞、能量滿按 A 或 C 放旋風衝刺
// 轉速就是血量:自然衰減、被撞掉更多,歸零就倒(對手 +1);兩邊同時結束算平手重打
// 爆裂:每顆 3 格鬆脫格,被重擊可能鬆一格(撞越大力、自己轉速越低越容易;旋風衝刺中不會鬆),扣完就裂成三片(對手 +2)
namespace spintop {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int CX = 160, CY = 128, RX = 120, RY = 96, TR = 11, HUD = 20, BX0 = 28, BY0 = 22, BW = 264, BH = 216, WIN_PTS = 4;
  constexpr float K_BOWL = 3.5f, LIP = 170, BAR_SPD = 1.6f, FLAT = 0.38f, TILT = 260, DRAG = 0.5f, DECAY = 2.2f, DASH = 260, RAIL = 330, RUSH_T = 3;
  // 出口:角度(度,螢幕座標 atan2,90 = 正下方)、半寬、分數。ponytail: 大小是從官方俯視圖目測推估的
  struct Exit { float ang, half; int pts; };
  static const Exit EXITS[3] = { { 45, 8, 2 }, { 90, 15, 3 }, { 135, 8, 2 } };
  static float rim(float nx, float ny) { return 1 / sqrtf(nx * nx / (RX * RX) + ny * ny / (RY * RY)); }   // 單位方向 (nx, ny) 上中心到碗邊的距離
  enum Phase { AIM, FIGHT, SCORED, WON, LOST };

  struct Top { float x, y, vx, vy, spin, phase, cool, railCool, rush; uint32_t col; bool out; int exitPts, loose; bool burst; };
  struct Frag { float x, y, vx, vy, a, va, life; uint32_t col; } static frags[6];   // 爆裂碎片
  static Top me, cpu;
  static Phase ph; static float bar, barDir, t, g0x, g0y, energy, aiT, msgT, cheerT; static bool perfect;
  static int scMe, scCpu, wins, rank; static char msg[20]; static uint32_t msgCol;
  static float hop[2][16];   // 左右看台每排觀眾的跳躍高度
  static Board board = { "spintop" };

  static void cheer(float amt, int side = -1) {   // side: 0 左、1 右、-1 兩邊
    for (int s = 0; s < 2; s++) if (side < 0 || side == s) for (auto& h : hop[s]) h = fmaxf(h, amt * (0.6f + frand() * 0.4f));
  }
  static void place() {
    me = { CX - 45, CY - 55, 0, 0, 0, 0, 0, 0, 0, rgb(60, 140, 255), false, 0, 0, false };
    cpu = { CX + 45, CY - 55, 0, 0, 0, 0, 0, 0, 0, rgb(255, 70, 60), false, 0, 0, false };
    memset(frags, 0, sizeof frags); ph = AIM; bar = 0; barDir = 1; perfect = false; energy = 0; aiT = 2;
  }
  static void newMatch() { scMe = scCpu = 0; place(); }
  void init() { wins = 0; rank = -1; t = msgT = cheerT = 0; memset(hop, 0, sizeof hop); ringlib::reset(); newMatch(); cv.fillScreen(0); }

  static int exitAt(float x, float y) {   // 在哪個出口的角度內,不在回 -1
    float a = atan2f(y - CY, x - CX) * RAD_TO_DEG;
    for (int i = 0; i < 3; i++) if (fabsf(fmodf(a - EXITS[i].ang + 540, 360) - 180) < EXITS[i].half) return i;
    return -1;
  }
  static void move(Top& o, float ax, float ay, float dt) {
    if (o.out || o.burst) return;
    float dx = o.x - CX, dy = o.y - CY, d = sqrtf(dx * dx + dy * dy) + 1e-3f, nx = dx / d, ny = dy / d;
    float f = FLAT * rim(nx, ny); if (d > f) { ax -= nx * K_BOWL * (d - f); ay -= ny * K_BOWL * (d - f); }   // 碗面:離中間越遠往回拉越強,中間一圈是平的
    if (o.spin < 30) { float a = frand() * 6.283f, w = (30 - o.spin) * 4; ax += cosf(a) * w; ay += sinf(a) * w; }   // 轉速低開始晃
    float drag = 1 - DRAG * dt; o.vx = (o.vx + ax * dt) * drag; o.vy = (o.vy + ay * dt) * drag;
    o.x += o.vx * dt; o.y += o.vy * dt;
    o.phase += (0.3f + o.spin / 100) * 25 * dt; o.cool -= dt; o.railCool -= dt; if (o.rush > 0) o.rush -= dt;
    o.spin -= DECAY * dt;
    dx = o.x - CX; dy = o.y - CY; d = sqrtf(dx * dx + dy * dy) + 1e-3f; nx = dx / d; ny = dy / d; float R = rim(nx, ny);
    int e = exitAt(o.x, o.y);
    if (e >= 0 && d > R + TR) { o.out = true; o.exitPts = EXITS[e].pts; return; }   // 整顆出去才算
    if (d < R - TR) return;
    float wx = dx / (RX * RX), wy = dy / (RY * RY), wl = sqrtf(wx * wx + wy * wy); wx /= wl; wy /= wl;   // 橢圓在這點的外法向
    float vn = o.vx * wx + o.vy * wy;
    if (e >= 0 && (d > R - TR + 3 || vn > LIP)) return;   // 出口有唇邊:往外衝夠快才翻得過去,翻過了就擋不住
    o.x = CX + nx * (R - TR); o.y = CY + ny * (R - TR);
    if (e < 0 && o.railCool <= 0 && o.spin > 10) {   // 加速軌:沿軌道(逆時針)衝一下再斜斜甩向中央
      float tx = wy, ty = -wx, k = 0.5f;   // 切線與往內的混合
      o.vx = (tx * k - wx * (1 - k)) * RAIL; o.vy = (ty * k - wy * (1 - k)) * RAIL; setSpeed(o.vx, o.vy, RAIL);
      o.railCool = 1.2f; spark(o.x, o.y, rgb(255, 230, 120)); snd::note(880); buzz(50, 20); cheer(0.4f, o.x < CX ? 0 : 1);
    } else if (vn > 0) { o.vx -= 1.8f * vn * wx; o.vy -= 1.8f * vn * wy; }
  }
  static void loosen(Top& o, float power) {   // 被撞的力道 power,可能鬆一格;扣完就爆
    if (o.rush > 0 || power < 80) return;
    float p = fminf(0.9f, power / 400 * (1.5f - fmaxf(o.spin, 0) / 100));
    if (frand() > p) return;
    snd::note(1400); spark(o.x, o.y, rgb(255, 255, 255));
    if (++o.loose < 3) return;
    o.burst = true; buzz(255, 400); cheer(1.5f); cheerT = 1.5f;
    int n = 0; for (auto& f : frags) if (f.life <= 0 && n < 3) {
      float a = frand() * 6.283f + n * 2.094f, v = 180 + frand() * 120;
      f = { o.x, o.y, cosf(a) * v + o.vx * 0.5f, sinf(a) * v + o.vy * 0.5f, a, (frand() - 0.5f) * 20, 2.0f, o.col }; n++;
    }
    for (int k = 0; k < 4; k++) spark(o.x, o.y, k & 1 ? o.col : rgb(255, 230, 120));
  }
  static void collide(Top& a, Top& b) {
    if (a.out || b.out || a.burst || b.burst) return;
    float ra = TR * (a.rush > 0 ? 1.3f : 1), rb = TR * (b.rush > 0 ? 1.3f : 1);
    float dx = b.x - a.x, dy = b.y - a.y, d = sqrtf(dx * dx + dy * dy), rr = ra + rb;
    if (d >= rr || d < 1e-3f) return;
    float nx = dx / d, ny = dy / d, pen = rr - d;
    a.x -= nx * pen / 2; a.y -= ny * pen / 2; b.x += nx * pen / 2; b.y += ny * pen / 2;
    float rv = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny; if (rv > 0) return;
    float ka = (0.5f + a.spin / 100) * (a.rush > 0 ? 2 : 1), kb = (0.5f + b.spin / 100) * (b.rush > 0 ? 2 : 1);   // 轉速越高撞越大力
    float j = -rv + 60;
    a.vx -= nx * j * kb; a.vy -= ny * j * kb; b.vx += nx * j * ka; b.vy += ny * j * ka;
    float hit = -rv; a.spin -= (2 + hit * 0.025f) * kb; b.spin -= (2 + hit * 0.025f) * ka;
    energy = fminf(100, energy + 20);
    loosen(a, hit * kb); loosen(b, hit * ka);
    float mx = (a.x + b.x) / 2, my = (a.y + b.y) / 2;
    spark(mx, my, rgb(255, 255, 200)); spark(mx, my, a.rush > 0 || b.rush > 0 ? rgb(120, 220, 255) : rgb(255, 160, 40));
    snd::click(); snd::note(500 + hit * 1.5f); buzz(hit > 200 ? 150 : 80, 25);
    if (hit > 200) cheer(0.5f, mx < CX ? 0 : 1);
  }
  static void finish(int toMe, int toCpu, const char* what) {
    scMe += toMe; scCpu += toCpu; ph = SCORED; msgT = 2;
    if (!toMe && !toCpu) { snprintf(msg, sizeof msg, "DRAW"); msgCol = rgb(220, 220, 220); }
    else { snprintf(msg, sizeof msg, "%s +%d", what, toMe + toCpu); msgCol = toMe ? me.col : cpu.col; }
    int big = toMe + toCpu; cheer(big >= 3 ? 1.6f : 1.0f); cheerT = big >= 3 ? 2.0f : 1.2f;
    snd::note(toMe ? 700 : 300); buzz(200, big >= 3 ? 300 : 150);
  }
  static void launch(float lx, float ly) {
    float p = bar; perfect = p > 0.92f;
    me.spin = perfect ? 100 : 35 + 60 * p; cpu.spin = 70 + frand() * 20;
    float dx = lx - CX, dy = ly - CY, q = sqrtf(sq(dx / (RX * FLAT)) + sq(dy / (RY * FLAT)));
    if (q > 1) { dx /= q; dy /= q; }   // 點在發射口外就拉回口內
    float d = sqrtf(dx * dx + dy * dy); if (d < 14) { dx = d > 1 ? dx / d * 14 : -14; dy = d > 1 ? dy / d * 14 : 0; }   // 太靠中心就錯開,免得兩顆疊在一起
    me.x = CX + dx; me.y = CY + dy; cpu.x = CX - dx; cpu.y = CY - dy;
    for (Top* o : { &me, &cpu }) { float ox = o->x - CX, oy = o->y - CY, ol = sqrtf(ox * ox + oy * oy); o->vx = oy / ol * 120; o->vy = -ox / ol * 120; }   // 逆時針繞圈的初速,不會一落地就對撞
    ph = FIGHT; msgT = 1; snprintf(msg, sizeof msg, perfect ? "PERFECT!" : "GO SHOOT!"); msgCol = perfect ? rgb(255, 230, 0) : rgb(255, 255, 255);
    snd::note(perfect ? 1000 : 600); buzz(perfect ? 180 : 100, 60); if (perfect) cheer(0.8f);
  }

  static int downPts(const Top& o) { return o.burst ? 2 : o.out ? o.exitPts : o.spin <= 0 ? 1 : 0; }   // 這顆倒了對手得幾分,沒倒回 0
  static const char* downName(const Top& o) { return o.burst ? "BURST" : o.out ? (o.exitPts == 3 ? "XTREME" : "OVER") : "SPIN"; }
  void step(const Ctx& c) {
    float dt = fminf(c.dt, 0.05f); t += dt;
    if (msgT > 0) msgT -= dt; if (cheerT > 0) { cheerT -= dt; if (frand() < 0.3f) cheer(0.8f); }
    for (auto& s : hop) for (auto& h : s) h = fmaxf(0, h - dt * 2);
    stepSparks(dt);
    for (auto& f : frags) if (f.life > 0) { f.life -= dt; f.x += f.vx * dt; f.y += f.vy * dt; f.vx *= 1 - 1.5f * dt; f.vy *= 1 - 1.5f * dt; f.a += f.va * dt; f.va *= 1 - dt; }
    switch (ph) {
      case AIM:
        bar += barDir * BAR_SPD * dt; if (bar > 1) { bar = 1; barDir = -1; } if (bar < 0) { bar = 0; barDir = 1; }   // 來回跑,不是跑到底重來
        if (c.tap) { g0x = c.gx; g0y = c.gy; launch(c.tx, c.ty); }   // 發射當下的姿勢當作平
        return;
      case FIGHT: break;
      case SCORED:
        if (msgT <= 0) {
          if (scMe >= WIN_PTS) { ph = WON; msgT = 2; wins++; snprintf(msg, sizeof msg, "YOU WIN!"); msgCol = rgb(255, 230, 0); cheer(1.6f); cheerT = 2; snd::note(1000); }
          else if (scCpu >= WIN_PTS) { ph = LOST; msgT = 1.5f; rank = board.record(wins); snd::note(200); }
          else place();
        }
        return;
      case WON: if (msgT <= 0) newMatch(); return;
      case LOST: if (msgT <= 0 && c.tap) init(); return;
    }
    // ---- 對戰 ----
    if (c.tap && me.cool <= 0) {   // 朝手指衝撞
      float dx = c.tx - me.x, dy = c.ty - me.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f;
      me.vx = dx / d * DASH; me.vy = dy / d * DASH; me.cool = 0.6f; me.spin -= 1.5f; snd::note(450);
    }
    if ((c.tapA || c.tapC) && energy >= 100) { energy = 0; me.rush = RUSH_T; me.spin = fminf(100, me.spin + 15); snd::note(1200); buzz(200, 120); cheer(1.0f); }
    float ax = (c.gx - g0x) * TILT, ay = (c.gy - g0y) * TILT;
    // 電腦:平常沿碗壁繞圈、每隔一陣子衝向你,轉速低就躲到中間;連勝越多衝得越勤
    float cax, cay;
    if (cpu.spin < 25) { cax = (CX - cpu.x) * 3; cay = (CY - cpu.y) * 3; }
    else { float a = atan2f(cpu.y - CY, cpu.x - CX) - 0.6f, tx = CX + cosf(a) * 60, ty = CY + sinf(a) * 60; cax = (tx - cpu.x) * 4; cay = (ty - cpu.y) * 4; }
    if ((aiT -= dt) <= 0) {
      float dx = me.x - cpu.x, dy = me.y - cpu.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f;
      cpu.vx = dx / d * DASH; cpu.vy = dy / d * DASH; cpu.spin -= 1.5f;
      float base = fmaxf(0.8f, 3.0f - wins * 0.3f); aiT = base * (0.7f + frand() * 0.6f);
    }
    move(me, ax, ay, dt); move(cpu, cax, cay, dt); collide(me, cpu);
    int mePts = downPts(me), cpuPts = downPts(cpu);
    if (mePts && cpuPts) finish(0, 0, "DRAW");   // 同時結束:平手重打
    else if (cpuPts) finish(cpuPts, 0, downName(cpu));
    else if (mePts) finish(0, mePts, downName(me));
  }

  static void drawTop(const Top& o) {
    if (o.out || o.burst || ph == AIM) return;   // 發射前還在發射器上
    float wob = ph != AIM && o.spin < 30 ? (30 - fmaxf(o.spin, 0)) / 8 : 0;
    int x = (int)(o.x + sinf(t * 23) * wob), y = (int)(o.y + cosf(t * 19) * wob), r = (int)(TR * (o.rush > 0 ? 1.3f : 1));
    if (o.rush > 0) cv.fillCircle(x, y, r + 3, rgb(120, 220, 255));
    cv.fillCircle(x, y, r, o.col); cv.fillCircle(x, y, r / 3, rgb(230, 230, 230));
    for (int k = 0; k < 3; k++) { float a = o.phase + k * 2.094f; cv.drawLine(x, y, x + (int)(cosf(a) * r), y + (int)(sinf(a) * r), rgb(20, 20, 30)); }
    float s = fmaxf(0, o.spin) / 100;
    if (s > 0) cv.fillArc(x, y, r + 2, r + 4, -90, -90 + 360 * s, s > 0.5f ? rgb(80, 230, 90) : s > 0.25f ? rgb(255, 200, 0) : rgb(255, 60, 60));   // 轉速環
    for (int k = 0; k < 3; k++) cv.fillCircle(x - 6 + k * 6, y - r - 9, 2, k < 3 - o.loose ? rgb(80, 230, 90) : rgb(110, 30, 30));   // 鬆脫格
  }
  static void band(float a0, float a1, float s0, float s1, uint32_t col) {   // 碗邊一段環帶(內外比例 s0..s1),用三角形拼
    for (float a = a0; a < a1; a += 3) {
      float b = fminf(a + 3, a1), ca = cosf(a * DEG_TO_RAD), sa = sinf(a * DEG_TO_RAD), cb = cosf(b * DEG_TO_RAD), sb = sinf(b * DEG_TO_RAD), ra = rim(ca, sa), rb = rim(cb, sb);
      int x0 = CX + ca * ra * s0, y0 = CY + sa * ra * s0, x1 = CX + ca * ra * s1, y1 = CY + sa * ra * s1, x2 = CX + cb * rb * s0, y2 = CY + sb * rb * s0, x3 = CX + cb * rb * s1, y3 = CY + sb * rb * s1;
      cv.fillTriangle(x0, y0, x1, y1, x3, y3, col); cv.fillTriangle(x0, y0, x3, y3, x2, y2, col);
    }
  }
  void draw() {
    cv.fillScreen(rgb(18, 18, 28));
    for (int s = 0; s < 2; s++) for (int row = 0; row < 16; row++) for (int col = 0; col < 2; col++) {   // 觀眾席
      int i = s * 32 + row * 2 + col, x = (s ? BX0 + BW + 6 : 6) + col * 12 + (row & 1) * 4, y = HUD + 10 + row * 13;
      float h = hop[s][row] * (4 + (i * 7 % 5)) + (cheerT > 0 ? 0 : sinf(t * 2 + i) * 0.8f);
      uint32_t c = cheerT > 0 && ((int)(t * 8) + i) % 3 == 0 ? msgCol : hsv(i * 0.137f);
      cv.fillCircle(x, y - (int)h, 4, c); cv.fillRect(x - 4, y + 3 - (int)h, 8, 4, rgb(60, 60, 80));
    }
    cv.fillRoundRect(BX0, BY0, BW, BH, 30, rgb(55, 60, 80));   // 盤身
    cv.fillEllipse(CX, CY, RX, RY, rgb(170, 175, 190)); cv.drawEllipse(CX, CY, (int)(RX * FLAT), (int)(RY * FLAT), rgb(150, 155, 170));   // 碗面與中間平台
    if (ph == AIM && (int)(t * 4) % 2) cv.fillEllipse(CX, CY, (int)(RX * FLAT), (int)(RY * FLAT), rgb(200, 220, 255));   // 發射口閃爍提示
    for (auto& e : EXITS) band(e.ang - e.half, e.ang + e.half, 0.97f, e.pts == 3 ? 1.12f : 1.35f, e.pts == 3 ? rgb(90, 0, 20) : rgb(20, 20, 30));   // 出口
    for (int a = 0; a < 360; a += 6) {   // 加速軌齒紋
      float c = cosf(a * DEG_TO_RAD), sn = sinf(a * DEG_TO_RAD), R = rim(c, sn); if (exitAt(CX + c * R, CY + sn * R) >= 0) continue;
      cv.drawLine(CX + (int)(c * (R - 5)), CY + (int)(sn * (R - 5)), CX + (int)(c * R), CY + (int)(sn * R), rgb(240, 200, 60));
    }
    drawTop(me); drawTop(cpu);
    for (auto& f : frags) if (f.life > 0) {   // 碎片:三分之一圈的弧片
      int x = (int)f.x, y = (int)f.y; float a0 = f.a * RAD_TO_DEG;
      cv.fillArc(x, y, TR - 4, TR, a0, a0 + 110, f.col); cv.drawLine(x, y, x + (int)(cosf(f.a) * TR), y + (int)(sinf(f.a) * TR), rgb(230, 230, 230));
    }
    drawSparks();
    // 上方資訊列
    cv.fillRect(0, 0, W, HUD, 0); cv.setTextSize(1); cv.setTextDatum(middle_left); char s[24];
    cv.setTextColor(me.col, 0); snprintf(s, sizeof s, "YOU %d", scMe); cv.drawString(s, 4, HUD / 2);
    cv.setTextColor(rgb(200, 200, 200), 0); cv.drawString(":", 46, HUD / 2);
    cv.setTextColor(cpu.col, 0); snprintf(s, sizeof s, "%d CPU", scCpu); cv.drawString(s, 54, HUD / 2);
    cv.setTextColor(rgb(160, 160, 160), 0); snprintf(s, sizeof s, "WIN %d", wins); cv.drawString(s, 100, HUD / 2);
    if (ph == AIM) {   // 力量條
      int x0 = 150, w = 164; cv.drawRect(x0, 4, w, 12, rgb(200, 200, 200));
      for (int i = 0; i < w - 2; i++) cv.drawFastVLine(x0 + 1 + i, 5, 10, hsv(0.33f - 0.33f * i / (w - 2)));   // 綠到紅
      cv.fillRect(x0 + (int)(w * 0.92f), 5, w - 1 - (int)(w * 0.92f), 10, rgb(255, 230, 0));   // 完美區
      cv.fillRect(x0 + (int)(bar * (w - 3)), 1, 3, 18, rgb(255, 255, 255));
      cv.setTextDatum(middle_center); cv.setTextColor(rgb(255, 255, 255)); cv.setTextSize(2); cv.drawString("TAP!", CX, CY);
    } else {   // 必殺能量
      int x0 = 230, w = 86; bool full = energy >= 100;
      cv.drawRect(x0, 5, w, 10, rgb(200, 200, 200)); cv.fillRect(x0 + 1, 6, (int)((w - 2) * energy / 100), 8, full && (int)(t * 6) % 2 ? rgb(255, 255, 255) : rgb(120, 220, 255));
      cv.setTextDatum(middle_right); cv.setTextColor(full ? rgb(255, 255, 255) : rgb(120, 120, 140), 0); cv.drawString(full ? "A/C!" : "RUSH", x0 - 4, HUD / 2);
    }
    if (msgT > 0 && ph != LOST) { cv.setTextDatum(middle_center); cv.setTextSize(3); cv.setTextColor(msgCol); cv.drawString(msg, CX, CY); }
    if (ph == LOST) { char tt[20]; snprintf(tt, sizeof tt, "WINS %d", wins); board.draw(tt, rank); }
  }
}
