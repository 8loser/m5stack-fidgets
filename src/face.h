#pragma once
#include "common.h"

// ================= 火柴人的臉(布娃娃、盪繩、電子雞共用)=================
// 在頭心 (cx,cy) 畫眼睛和嘴;rx,ry 是臉的「右」方向,dx,dy 是「下」方向(單位向量),頭歪了臉跟著轉
namespace face {
  enum Mood { CALM, HAPPY, SCARED, HURT, DIZZY, SLEEPY, SAD, HUNGRY, EATING, SICK, DEAD, ANNOYED, NMOOD };
  static const char* NAME[NMOOD] = { "calm", "happy", "scared", "ouch", "dizzy", "zzz", "sad", "hungry", "yum", "sick", "dead", "hmph" };
  static void draw(float cx, float cy, float rx, float ry, float dx, float dy, Mood m, float s = 1) {
    auto at = [&](float r, float d, int& x, int& y) { x = (int)(cx + (rx * r + dx * d) * s); y = (int)(cy + (ry * r + dy * d) * s); };
    uint32_t wh = rgb(255, 255, 255), pk = rgb(255, 80, 120), gr = rgb(120, 220, 120);
    int ex, ey, ax, ay, bx, by, mx, my;
    for (int k = -1; k <= 1; k += 2) {   // 眼睛
      at(k * 3, -1.5f, ex, ey);
      switch (m) {
        case SCARED: cv.drawCircle(ex, ey, 2, wh); break;
        case DIZZY: case DEAD: cv.drawLine(ex - 1, ey - 1, ex + 1, ey + 1, wh); cv.drawLine(ex - 1, ey + 1, ex + 1, ey - 1, wh); break;
        case SLEEPY: case EATING: cv.drawLine(ex - 1, ey, ex + 1, ey, wh); break;
        case HURT: case ANNOYED: cv.drawLine(ex - k, ey - 1, ex + k, ey + 1, wh); break;
        case SAD: case HUNGRY: cv.drawLine(ex + k, ey - 1, ex - k, ey + 1, wh); cv.drawPixel(ex, ey + 1, wh); break;
        case SICK: cv.drawCircle(ex, ey, 1, gr); break;
        default: cv.fillRect(ex, ey, 2, 2, wh);
      }
    }
    at(0, 3, mx, my);   // 嘴
    switch (m) {
      case HAPPY: at(-3, 2, ax, ay); at(3, 2, bx, by); cv.drawLine(ax, ay, mx, my + 1, wh); cv.drawLine(mx, my + 1, bx, by, wh); break;
      case SCARED: cv.fillCircle(mx, my, 2, pk); break;
      case EATING: cv.fillCircle(mx, my, 2, pk); cv.drawCircle(mx, my, 3, wh); break;
      case HURT: case DIZZY: case SAD: case HUNGRY: case SICK: at(-3, 4, ax, ay); at(3, 4, bx, by); cv.drawLine(ax, ay, mx, my - 1, wh); cv.drawLine(mx, my - 1, bx, by, wh); break;
      case DEAD: at(-3, 3, ax, ay); at(3, 3, bx, by); cv.drawLine(ax, ay, bx, by, wh); at(0, 4, ax, ay); cv.drawLine(mx - 1, my, mx + 1, my + 2, pk); break;
      case SLEEPY: { cv.drawPixel(mx, my, wh); int zx, zy; at(9, -8, zx, zy); cv.setTextDatum(middle_center); cv.setTextSize(1); cv.setTextColor(rgb(150, 150, 255), 0); cv.drawString("z", zx, zy); break; }
      case ANNOYED: at(-2, 3, ax, ay); at(2, 3, bx, by); cv.drawLine(ax, ay, bx, by + 1, wh); break;
      default: at(-2, 3, ax, ay); at(2, 3, bx, by); cv.drawLine(ax, ay, bx, by, wh);
    }
  }
}
