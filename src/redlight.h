#pragma once
#include "common.h"

// ================= 39. 一二三木頭人 =================
// 魷魚遊戲的木頭人,第三人稱背後視角:你(綠色運動服 456)站在畫面下方,前面散著一群其他玩家,兩側是掛喇叭的高牆,
// 終點線後的枯樹前站著娃娃,兩側各一個粉紅裁判;越走越大。全部用圓與圓角矩形畫,沒有貼圖。
// A 左腳、C 右腳交替按才前進一步,腳跟著抬、鏡頭跟著晃;同一隻腳連按不前進還會絆一下(晃更久)。每步後 STEP_T 秒算「還在動」。
// 綠燈:娃娃背對,唱「一二三木頭人」六個音,節奏隨機忽快忽慢、最後一音有時拉長;唱完就轉頭。其他玩家綠燈時各走各的,
// 紅燈時偶爾有人動了被開槍倒地。
// 紅燈(1 到 3 秒):按了 A / C、還在收腳、或機器在晃(Ctx::shake,要把整台機器拿穩)就被開槍,結束。
// 限時走到終點線進下一關:路更長、時間更短、節奏更快。分數是過幾關,前 5 名存 NVS
namespace redlight {
  static constexpr float SHAKE_LIM = 0.25f;   // 校正旋鈕:紅燈時 shake 超過這個算動了;實機手抖抓不住就調大
  constexpr int HZ = 96, K = 120, NNOTE = 6, MAXN = 12;   // 地平線 y、透視係數(y = HZ + K / z,1 單位 = 相機高度)
  constexpr float STEP_T = 0.3f, STUMBLE_T = 0.8f, WALL = 3.0f, SHOT_RATE = 0.06f;   // 牆在左右 ±WALL 單位;紅燈時每個玩家每秒被槍斃的機率
  static const float CHANT[NNOTE] = { 392, 440, 523, 523, 392, 330 };
  struct Npc { float x, z, v, ph; bool dead; } static npc[MAXN];   // x 橫向、z 沿跑道的絕對位置(單位),ph 走路相位
  static float dist, len, timeLeft, stepT, amp, noteT, redT, endT; static int lvl, foot, noteI, rank; static bool red, dead;
  static Board board = { "redlight" };
  static void playNote() {   // 唱下一個音:節奏隨機、關數越高越快;最後一音一半機率拉長
    float T = 0.15f + frand() * fmaxf(0.5f - 0.04f * lvl, 0.1f);
    noteT = T * (0.7f + 0.6f * frand()) * (noteI == NNOTE - 1 && frand() < 0.5f ? 2.5f : 1); snd::note(CHANT[noteI]);
  }
  static void chant() { red = false; noteI = 0; playNote(); }
  static void newRound() {
    len = 24 + 6 * lvl; timeLeft = fmaxf(40 - 2 * lvl, 20); dist = 0; stepT = 0; foot = -1; chant();
    for (auto& n : npc) n = { (frand() * 2 - 1) * (WALL - 0.5f), 0.6f + frand() * 7, 1.5f + frand() * 2.5f, frand() * 6.28f, false };
  }
  void init() { lvl = 1; dead = false; endT = 0; newRound(); }
  static void die() { dead = true; endT = 0; rank = board.record(lvl - 1); snd::click(); buzz(255, 300); }
  void step(const Ctx& c) {
    if (dead) { if ((endT += c.dt) > 1.5f && (c.tap || c.tapA)) init(); return; }
    if ((timeLeft -= c.dt) <= 0) { die(); return; }
    if (stepT > 0) stepT -= c.dt;
    if (c.tapA || c.tapC) {
      int f = c.tapC;
      if (f != foot) { foot = f; dist += 1; stepT = STEP_T; amp = 6; buzz(20, 8); }
      else { stepT = STUMBLE_T; amp = 14; buzz(60, 40); }   // 同腳連按:絆一下
    }
    if (red) {
      if (c.tapA || c.tapC || stepT > 0 || c.shake > SHAKE_LIM) { die(); return; }
      if ((redT -= c.dt) <= 0) chant();
      for (auto& n : npc) if (!n.dead && n.z < len && frand() < SHOT_RATE * c.dt) { n.dead = true; snd::click(); }
    } else {
      if ((noteT -= c.dt) <= 0) {
        if (++noteI < NNOTE) playNote();
        else { red = true; redT = 1 + frand() * 2; snd::note(196); buzz(40, 20); }
      }
      for (auto& n : npc) if (!n.dead && n.z < len) { n.z += n.v * c.dt; n.ph += n.v * 4 * c.dt; }
    }
    if (dist >= len) { lvl++; snd::note(880); buzz(80, 60); newRound(); }
  }
  // 玩家人形(背影):cx 腳底中心、gy 腳底 y、h 身高 px、lift 哪隻腳抬多高(-1..1,負左正右);dead 就畫成躺著的一條
  static void figure(int cx, int gy, float h, float lift, bool dead, bool me) {
    uint32_t suit = rgb(30, 110, 100), suitD = rgb(20, 80, 75), hair = rgb(25, 20, 20), skin = rgb(236, 195, 154), wht = rgb(255, 255, 255);
    auto RR = [&](float x, float y, float w, float hh, uint32_t c) { int r = (int)(fminf(w, hh) * 0.4f); cv.fillRoundRect(cx + (int)(x * h), gy + (int)(y * h), (int)(w * h) + 1, (int)(hh * h) + 1, r, c); };
    if (dead) { RR(-0.45f, -0.14f, 0.9f, 0.14f, suit); cv.fillCircle(cx + (int)(0.5f * h), gy - (int)(0.07f * h), (int)(0.08f * h) + 1, hair); return; }
    float lL = 0.34f - fmaxf(-lift, 0) * 0.12f, rL = 0.34f - fmaxf(lift, 0) * 0.12f;
    RR(-0.15f, -lL, 0.13f, lL, suit); RR(0.02f, -rL, 0.13f, rL, suit);
    RR(-0.15f, -lL, 0.13f, 0.05f, wht); RR(0.02f, -rL, 0.13f, 0.05f, wht);   // 鞋(背影看到的是腳跟)
    RR(-0.18f, -0.7f, 0.36f, 0.4f, suit); RR(-0.18f, -0.7f, 0.36f, 0.03f, suitD);   // 上衣、肩線
    RR(-0.27f, -0.68f + lift * 0.06f, 0.09f, 0.32f, suit); RR(0.18f, -0.68f - lift * 0.06f, 0.09f, 0.32f, suit);   // 手臂前後擺
    RR(-0.04f, -0.75f, 0.08f, 0.06f, skin);
    cv.fillCircle(cx, gy - (int)(0.85f * h), (int)(0.11f * h) + 1, hair);
    if (me && h > 60) { cv.setTextDatum(middle_center); cv.setTextSize(1); cv.setTextColor(wht, suit); cv.drawString("456", cx, gy - (int)(0.55f * h)); }
  }
  void draw() {
    float ph = stepT > 0 ? stepT / STEP_T : 0, sw = sinf(ph * 3.14159f), bob = amp * sw;   // 收腳進度 1 到 0;絆倒時 ph 超過 1,多晃幾下
    int side = foot == 1 ? 1 : -1, vx = W / 2 + (int)(side * bob), hz = HZ + (int)(amp * 0.5f * sinf(ph * 6.2832f));   // 鏡頭跟著步伐左右、上下晃
    uint32_t sky = rgb(150, 200, 235), sand = rgb(214, 190, 140), sandD = rgb(190, 165, 115), wall = rgb(150, 150, 150), wallD = rgb(110, 110, 110), spk = rgb(40, 40, 40);
    cv.fillRect(0, 0, W, hz, sky); cv.fillRect(0, hz - 8, W, 8, rgb(215, 175, 85)); cv.fillRect(0, hz, W, H - hz, sand);   // 天、地平線上一條麥田、沙地
    float f = fmodf(dist, 2);
    for (int k = 0; k < 24; k++) { float z = k * 2 - f + 0.5f; if (z < 0.3f) continue; cv.drawFastHLine(0, hz + (int)(K / z), W, sandD); }
    float zf = len - dist; if (zf > 0.2f) { int y1 = hz + (int)(K / (zf + 0.5f)), y2 = hz + (int)(K / zf); cv.fillRect(0, y1, W, y2 - y1 < 1 ? 1 : y2 - y1, rgb(255, 255, 255)); }
    // 兩側高牆(3 單位高)往消失點收,牆上每 4 單位一個喇叭
    for (int sd = -1; sd <= 1; sd += 2) {
      float zn = 0.5f, zfar = 60; int xn = vx + (int)(sd * WALL * K / zn), xf = vx + (int)(sd * WALL * K / zfar);
      int yn = hz + (int)(K / zn), yf = hz + (int)(K / zfar), tn = hz - (int)(3 * K / zn), tf = hz - (int)(3 * K / zfar);
      cv.fillTriangle(xn, yn, xf, yf, xf, tf, wall); cv.fillTriangle(xn, yn, xf, tf, xn, tn, wall); cv.drawLine(xn, yn, xf, yf, wallD);
      float f4 = fmodf(dist, 4);
      for (int k = 0; k < 15; k++) { float z = k * 4 - f4 + 1; if (z < 0.6f) continue; int s = (int)(0.35f * K / z) + 1; cv.fillRoundRect(vx + (int)(sd * WALL * K / z) - s / 2, hz - (int)(1.2f * K / z) - s, s, s + s / 2, s / 4, spk); }
    }
    // 娃娃(全身 184 單位,胸像下緣 b 往上 64 是頭、往下 120 是身體)、樹 170、裁判 90;z 是一單位幾像素
    float zd = zf + 4, z = 3.2f / zd; int gy = hz + (int)(K / zd);
    auto R = [&](int cx, int base, float x, float y, float w, float h, uint32_t c) { int r = (int)(fminf(w, h) * z * 0.4f); cv.fillRoundRect(cx + (int)(x * z), base + (int)(y * z), (int)ceilf(w * z), (int)ceilf(h * z), r, c); };
    auto C = [&](int cx, int base, float x, float y, float r, uint32_t c) { cv.fillCircle(cx + (int)(x * z), base + (int)(y * z), (int)(r * z), c); };
    uint32_t skin = rgb(236, 195, 154), yel = rgb(251, 230, 43), org = rgb(208, 95, 27), orgD = rgb(170, 70, 20), wht = rgb(255, 255, 255), blk = rgb(20, 20, 20), pink = rgb(230, 60, 100), bark = rgb(70, 45, 30), hair = rgb(40, 25, 20), purple = rgb(150, 60, 170);
    R(vx, gy, -8, -170, 16, 170, bark);   // 樹:主幹 + 幾根枯枝
    for (int i = -2; i <= 2; i++) if (i) cv.drawLine(vx, gy - (int)(160 * z), vx + (int)(i * 45 * z), gy - (int)((215 - abs(i) * 15) * z), bark);
    float zSave = z; z *= 0.65f;   // 裁判比娃娃小
    for (int sd = -1; sd <= 1; sd += 2) {   // 裁判:黑頭套白圓、粉紅連身衣、黑腰帶手套靴子
      int cx = vx + (int)(sd * 110 * z), b = gy - (int)(90 * z);
      R(cx, b, -9, 54, 8, 30, pink); R(cx, b, 1, 54, 8, 30, pink); R(cx, b, -10, 84, 10, 6, blk); R(cx, b, 0, 84, 10, 6, blk);
      R(cx, b, -10, 18, 20, 36, pink); R(cx, b, -10, 38, 20, 3, blk);
      R(cx, b, -15, 18, 5, 34, pink); R(cx, b, 10, 18, 5, 34, pink); R(cx, b, -15, 52, 5, 6, blk); R(cx, b, 10, 52, 5, 6, blk);
      C(cx, b, 0, 9, 9, blk); cv.drawCircle(cx, b + (int)(9 * z), (int)(5 * z), wht);
    }
    z = zSave;
    int b = gy - (int)(120 * z);
    R(vx, b, -16, 72, 11, 18, skin); R(vx, b, 5, 72, 11, 18, skin);   // 腿、白襪、黑鞋
    R(vx, b, -17, 90, 13, 22, wht); R(vx, b, 4, 90, 13, 22, wht);
    R(vx, b, -19, 112, 16, 8, blk); R(vx, b, 3, 112, 16, 8, blk);
    R(vx, b, -36, 0, 14, 22, yel); R(vx, b, 22, 0, 14, 22, yel);   // 黃短袖、手臂微張
    R(vx, b, -40, 22, 9, 40, skin); R(vx, b, 31, 22, 9, 40, skin); C(vx, b, -36, 66, 6, skin); C(vx, b, 36, 66, 6, skin);
    cv.fillTriangle(vx - (int)(22 * z), b, vx + (int)(22 * z), b, vx - (int)(34 * z), b + (int)(72 * z), org); cv.fillTriangle(vx + (int)(22 * z), b, vx + (int)(34 * z), b + (int)(72 * z), vx - (int)(34 * z), b + (int)(72 * z), org);   // 橘裙
    R(vx, b, -34, 66, 68, 8, orgD); R(vx, b, -16, -4, 32, 8, yel);   // 裙襬、領子
    C(vx, b, -30, -14, 9, hair); C(vx, b, 30, -14, 9, hair); C(vx, b, -24, -18, 3, purple); C(vx, b, 24, -18, 3, purple);   // 雙馬尾與髮圈
    R(vx, b, -5, -8, 10, 8, skin);   // 脖子
    if (red) {   // 正面:髮帽、圓臉、眼睛、腮紅、嘴、髮夾
      C(vx, b, 0, -36, 28, hair); C(vx, b, 0, -28, 25, skin);
      C(vx, b, -9, -30, 4, wht); C(vx, b, 9, -30, 4, wht); C(vx, b, -9, -29, 2.5f, hair); C(vx, b, 9, -29, 2.5f, hair);
      C(vx, b, -15, -20, 4, rgb(231, 171, 164)); C(vx, b, 15, -20, 4, rgb(231, 171, 164));
      R(vx, b, -5, -13, 10, 4, rgb(200, 60, 60)); R(vx, b, 16, -58, 6, 4, purple);
    } else {   // 背面:一整顆頭髮
      C(vx, b, 0, -36, 28, hair); R(vx, b, -25, -40, 50, 34, hair);
    }
    // 其他玩家:遠的先畫(身高 1 單位)
    int ord[MAXN]; for (int i = 0; i < MAXN; i++) ord[i] = i;
    for (int i = 1; i < MAXN; i++) for (int j = i; j > 0 && npc[ord[j]].z > npc[ord[j - 1]].z; j--) { int q = ord[j]; ord[j] = ord[j - 1]; ord[j - 1] = q; }
    for (int i : ord) { const Npc& n = npc[i]; float zr = n.z - dist; if (zr < 1) continue; figure(vx + (int)(n.x * K / zr), hz + (int)(K / zr), K / zr, n.dead || red ? 0 : sinf(n.ph), n.dead, false); }
    // 你:畫面下方,腳跟著 A / C 抬、身體跟著步伐上下,絆倒時左右晃
    figure(W / 2 + (int)(side * bob * 0.6f), H - 6 + (int)(fabsf(bob) * 0.5f), 100, foot < 0 ? 0 : side * sw, false, true);
    char s[24]; snprintf(s, sizeof s, "R%d  %2ds  %d/%d", lvl, (int)ceilf(timeLeft), (int)dist, (int)len);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(40, 40, 40), sky); cv.drawString(s, 4, 4);
    cv.setTextDatum(top_center); cv.setTextSize(2); cv.setTextColor(red ? rgb(220, 30, 30) : rgb(30, 160, 60), sky); cv.drawString(red ? "STOP" : "GO", W / 2, 4);
    if (dead) {
      if (endT < 0.25f) { cv.fillScreen(rgb(200, 0, 0)); return; }
      snprintf(s, sizeof s, "ROUND %d", lvl - 1); board.draw(s, rank);
    }
  }
}
