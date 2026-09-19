#pragma once
#include "ringlib.h"
#include "face.h"

// ================= 30. 罰球 =================
// 球門在上面,守門員 Kevin(黑球)在門線左右移動;你是 Seed,傾斜(或 A/C)瞄準,點螢幕射門。
// 每進一球 Kevin 就變大一圈,越來越難進。10 球結束,分數是進球數,前 5 名存 NVS
namespace penalty {
  using ringlib::spark; using ringlib::stepSparks; using ringlib::drawSparks;
  constexpr int GX0 = 70, GX1 = 250, GY = 40, GH = 28, SHOTS = 10, BALL_R = 6, KR0 = 12, KRMAX = 44; constexpr float SPEED = 520, KSPEED = 140;
  static float bx, by, bvx, bvy, aim, kx, kr, kdir, waitT, endT; static int shot, goals, rank, lastRes; static int8_t hist[SHOTS]; static bool flying, over; static face::Mood kmood;
  static Board board = { "penalty" };
  static void resetBall() { bx = 160; by = 205; bvx = bvy = 0; flying = false; waitT = 0; }
  void init() { memset(hist, 0, sizeof hist); shot = goals = 0; rank = -1; lastRes = 0; over = false; endT = 0; kx = 160; kr = KR0; kdir = 1; aim = 0; kmood = face::CALM; resetBall(); ringlib::reset(); cv.fillScreen(0); }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); stepSparks(c.dt); return; }
    kx += kdir * KSPEED * c.dt; if (kx < GX0 + kr) { kx = GX0 + kr; kdir = 1; } if (kx > GX1 - kr) { kx = GX1 - kr; kdir = -1; }   // Kevin 來回走
    if (flying && by < 150) { kx += (bx > kx ? 1 : -1) * KSPEED * 0.8f * c.dt; }   // 球接近時撲向球
    if (!flying) {
      aim += (c.gx - aim) * 5 * c.dt; aim = aim < -1 ? -1 : aim > 1 ? 1 : aim;   // 傾斜瞄準
      if (waitT > 0) waitT -= c.dt;
      else if (c.tap || c.tapA || c.tapC) { flying = true; bvx = aim * SPEED * 0.55f; bvy = -SPEED; snd::note(400); buzz(40, 15); }
    } else {
      bx += bvx * c.dt; by += bvy * c.dt;
      if (bx < BALL_R) { bx = BALL_R; bvx = fabsf(bvx); } if (bx > W - BALL_R) { bx = W - BALL_R; bvx = -fabsf(bvx); }
      float dx = bx - kx, dy = by - GY, d = sqrtf(dx * dx + dy * dy);
      int res = 0;
      if (d < kr + BALL_R) { res = -1; kmood = face::HAPPY; spark(bx, by, rgb(255, 255, 255)); snd::click(); buzz(60, 30); }   // 被 Kevin 擋下
      else if (by < GY && bx > GX0 && bx < GX1) { res = 1; goals++; kr = fminf(kr + 4, KRMAX); kmood = face::SAD; spark(bx, by, rgb(255, 230, 0)); snd::note(700); buzz(120, 80); }   // 進了
      else if (by < GY - 10) { res = -1; kmood = face::HAPPY; snd::note(200); }   // 射偏
      if (res) {
        lastRes = res; hist[shot++] = (int8_t)res; resetBall(); waitT = 1.0f;
        if (shot >= SHOTS) { over = true; endT = 0; rank = board.record(goals); }
      }
    }
    stepSparks(c.dt);
  }
  void draw() {
    cv.fillScreen(rgb(30, 90, 40));
    cv.fillRect(0, 0, W, GY + 4, rgb(20, 40, 60));   // 觀眾席
    for (int i = 0; i < 40; i++) cv.drawPixel((i * 53) % W, (i * 31) % (GY - 4), hsv(i / 40.0f));
    cv.drawRect(GX0, GY - GH, GX1 - GX0, GH, rgb(255, 255, 255)); cv.drawRect(GX0 + 1, GY - GH + 1, GX1 - GX0 - 2, GH - 2, rgb(255, 255, 255));
    for (int x = GX0 + 8; x < GX1; x += 8) cv.drawFastVLine(x, GY - GH + 2, GH - 2, rgb(120, 120, 130));
    for (int y = GY - GH + 8; y < GY; y += 8) cv.drawFastHLine(GX0 + 2, y, GX1 - GX0 - 4, rgb(120, 120, 130));
    cv.drawFastHLine(0, GY + 4, W, rgb(220, 220, 220));
    cv.fillCircle((int)kx, GY, (int)kr, rgb(25, 25, 30)); cv.drawCircle((int)kx, GY, (int)kr, rgb(255, 60, 60)); face::draw(kx, GY, 1, 0, 0, 1, kmood, kr / 14.0f);   // Kevin
    if (!flying) { cv.drawLine((int)bx, (int)by - 10, (int)(bx + aim * 60), (int)by - 60, rgb(255, 255, 255)); }   // 瞄準線
    cv.fillCircle((int)bx, (int)by, BALL_R, rgb(240, 240, 240)); cv.drawCircle((int)bx, (int)by, BALL_R, rgb(40, 40, 40));
    cv.fillCircle(160, 226, 8, rgb(220, 120, 30)); face::draw(160, 226, 1, 0, 0, 1, lastRes > 0 ? face::HAPPY : lastRes < 0 ? face::SAD : face::CALM, 1);   // Seed
    drawSparks();
    for (int i = 0; i < SHOTS; i++) cv.fillCircle(100 + i * 13, 56, 4, hist[i] > 0 ? rgb(60, 220, 80) : hist[i] < 0 ? rgb(230, 60, 60) : rgb(70, 70, 70));   // 每球結果
    char t[32]; snprintf(t, sizeof t, "goals %d  shot %d/%d", goals, shot, SHOTS);
    cv.setTextDatum(top_left); cv.setTextSize(1); cv.setTextColor(rgb(220, 220, 220), rgb(20, 40, 60)); cv.drawString(t, 4, 4);
    if (over) { snprintf(t, sizeof t, "GOALS %d", goals); board.draw(t, rank); }
  }
}
