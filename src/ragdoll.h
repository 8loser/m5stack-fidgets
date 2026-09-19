#pragma once
#include "ringlib.h"
#include "face.h"

// ================= 火柴人布娃娃骨架(盪繩用)=================
// 11 個點 + 距離約束的 Verlet 布娃娃:積分、約束求解、推出障礙、撞擊反應與表情
namespace ragdoll {
  using ringlib::spark;
  constexpr int NP = 11, ITER = 5, HEAD_R = 7; constexpr float G = 900, DAMP = 0.995f, FRICTION = 0.6f;
  enum { HEAD, NECK, HIP, LELB, LHAND, RELB, RHAND, LKNEE, LFOOT, RKNEE, RFOOT };
  struct Pt { float x, y, ox, oy; } static p[NP];
  struct Link { uint8_t a, b; float len; };
  static const Link links[] = { {HEAD,NECK,8},{NECK,HIP,22},{NECK,LELB,12},{LELB,LHAND,12},{NECK,RELB,12},{RELB,RHAND,12},
                                {HIP,LKNEE,14},{LKNEE,LFOOT,14},{HIP,RKNEE,14},{RKNEE,RFOOT,14},{HEAD,HIP,30} };   // 頭-腰撐直脊椎
  static float hurtT, dizzyT, restT, headV; static face::Mood mood;
  static void place(float cx, float cy) {
    const float ox[NP] = { 0, 0, 0, -8, -14, 8, 14, -6, -8, 6, 8 }, oy[NP] = { -30, -22, 0, -14, -4, -14, -4, 12, 26, 12, 26 };
    for (int i = 0; i < NP; i++) p[i] = { cx + ox[i], cy + oy[i], cx + ox[i], cy + oy[i] };
    hurtT = dizzyT = restT = 0; mood = face::CALM;
  }
  // 把點推出障礙:回傳沿法向的穿入速度(> 0 表示撞上),並把切向速度乘 FRICTION
  static float pushOut(Pt& q, float nx, float ny, float pen) {
    if (pen <= 0) return 0;
    float vx = q.x - q.ox, vy = q.y - q.oy, vn = vx * nx + vy * ny, tx = vx - vn * nx, ty = vy - vn * ny;
    q.x += nx * pen; q.y += ny * pen;
    q.ox = q.x - tx * FRICTION; q.oy = q.y - ty * FRICTION;   // 法向速度歸零、切向摩擦
    return -vn;
  }
  static void integrate(const Ctx& c, float g = G) {
    for (auto& q : p) { float vx = (q.x - q.ox) * DAMP, vy = (q.y - q.oy) * DAMP; q.ox = q.x; q.oy = q.y; q.x += vx + c.gx * g * c.dt * c.dt; q.y += vy + c.gy * g * c.dt * c.dt; }
  }
  static void solveLinks() {
    for (auto& l : links) {
      auto& a = p[l.a]; auto& b = p[l.b]; float dx = b.x - a.x, dy = b.y - a.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f, k = (d - l.len) / d * 0.5f;
      a.x += dx * k; a.y += dy * k; b.x -= dx * k; b.y -= dy * k;
    }
  }
  static void react(float maxV, int maxI) {   // 撞擊:痛;頭重擊:暈
    if (maxV <= 150) return;
    if (maxI == HEAD && maxV > 300) { dizzyT = 1.8f; snd::note(180); buzz(150, 60); spark(p[HEAD].x, p[HEAD].y, rgb(255, 230, 0)); }
    else { hurtT = 0.4f; snd::click(); buzz(40, 15); }
  }
  static void updateMood(float dt, bool excited) {
    float hx = p[HEAD].x - p[HEAD].ox, hy = p[HEAD].y - p[HEAD].oy; headV = sqrtf(hx * hx + hy * hy) / dt;
    restT = headV < 15 ? restT + dt : 0; hurtT -= dt; dizzyT -= dt;
    mood = dizzyT > 0 ? face::DIZZY : hurtT > 0 ? face::HURT : headV > 400 ? face::SCARED : (excited || headV > 120) ? face::HAPPY : restT > 4 ? face::SLEEPY : face::CALM;
  }
  static void thick(float x0, float y0, float x1, float y1, uint32_t col) { cv.drawLine((int)x0, (int)y0, (int)x1, (int)y1, col); cv.drawLine((int)x0 + 1, (int)y0, (int)x1 + 1, (int)y1, col); cv.drawLine((int)x0, (int)y0 + 1, (int)x1, (int)y1 + 1, col); }
  static void drawBody() {
    uint32_t body = mood == face::DIZZY ? rgb(255, 230, 0) : mood == face::SCARED ? rgb(120, 200, 255) : mood == face::HURT ? rgb(255, 120, 120) : rgb(255, 255, 255);
    for (auto& l : links) if (!(l.a == HEAD && l.b == HIP)) thick(p[l.a].x, p[l.a].y, p[l.b].x, p[l.b].y, body);
    cv.fillCircle((int)p[HEAD].x, (int)p[HEAD].y, HEAD_R, rgb(30, 30, 40)); cv.drawCircle((int)p[HEAD].x, (int)p[HEAD].y, HEAD_R, body);
    float dx = p[NECK].x - p[HEAD].x, dy = p[NECK].y - p[HEAD].y, d = sqrtf(dx * dx + dy * dy) + 1e-3f;   // 頭→頸當「下」
    face::draw(p[HEAD].x, p[HEAD].y, -dy / d, dx / d, dx / d, dy / d, mood);
  }
}
