#pragma once
#include "common.h"

// ================= 公路賽車 =================
// OutRun 式偽 3D 公路:路由一段段帶彎度與高度的路段組成,每幀把前方 DRAW 段投影成梯形畫出來(彎道靠累加橫向位移、坡靠路段高度)。
// 開局隨機生一圈路(頭尾高度接回 0),跑完一圈接著跑。自動加速,最高時速隨距離從 150 漸增到 300 km/h;
// 傾斜轉向(A / C 是虛擬傾斜),按住螢幕煞車(亮煞車燈),彎道有離心力往外甩,開出路面會減速;路上車流越跑越多,撞到就結束,比距離,前 5 名排行
// 選單預覽沒有觸控,改成自動駕駛閃車
namespace racer {
  constexpr int NSEG = 1600, DRAW = 80, NCAR = 40;
  constexpr float SEG = 200, RW = 2000, CAMH = 1000, DEPTH = 0.84f, PZ = CAMH * DEPTH, HOR = H / 2;   // DEPTH = 1 / tan(視角 100° / 2);PZ:玩家在鏡頭前多遠(剛好畫在螢幕底)
  constexpr float KMH = 3.6f / SEG, CARW = 500, TREE_H = 3000, LANE = 0.66f;   // 一段算 1 m,速度(單位 / 秒)乘 KMH 就是 km/h
  struct Seg { float curve, y; };   // y:這段終點的高度,起點用前一段的
  struct Car { float z, x, v; uint8_t pal; bool truck; };
  static const uint8_t PAL[8][3] = { {235,235,235}, {170,175,182}, {35,35,40}, {190,30,30}, {40,80,170}, {225,185,40}, {30,90,60}, {110,25,35} };   // 常見車色:白、銀、黑、紅、藍、黃、墨綠、酒紅
  struct Proj { float x, y, w, s; };
  static Seg seg[NSEG]; static int ns;
  static Car cars[NCAR]; static int active;
  static Proj p1[DRAW], p2[DRAW]; static float clipY[DRAW]; static bool front[DRAW];
  static float pos, speed, px, steer, yaw, dist, bgX, camY, endT; static int rank; static bool over, brake;
  static Board board = { "racer" };

  static float len() { return ns * SEG; }
  static float wrapZ(float z) { float L = len(); z = fmodf(z, L); return z < 0 ? z + L : z; }
  static float rel(const Car& c) { float d = wrapZ(c.z - pos - PZ); return d > len() / 2 ? d - len() : d; }   // 車在玩家前方多遠(負的是在後面)
  static float yStart(int i) { return seg[(i + ns - 1) % ns].y; }
  static void addRoad(int enter, int hold, int leave, float curve, float hill) {   // 彎度進出線性漸變,高度用 cos 緩入緩出
    float y0 = ns ? seg[ns - 1].y : 0, y1 = y0 + hill * SEG; int n = enter + hold + leave;
    for (int k = 0; k < n && ns < NSEG; k++) {
      float c = k < enter ? curve * k / enter : k < enter + hold ? curve : curve * (n - k) / leave, t = (k + 1.0f) / n;
      seg[ns++] = { c, y0 + (y1 - y0) * (0.5f - cosf(t * 3.14159f) / 2) };
    }
  }
  static void build() {
    ns = 0; addRoad(0, 40, 0, 0, 0);   // 起跑直線
    while (ns < NSEG - 220) {   // 每段最多 120,加上收尾 90,不會超過 NSEG
      int n = 20 + (int)(frand() * 40);
      addRoad(n / 2, n, n / 2, frand() < 0.3f ? 0 : (frand() * 2 - 1) * 5, frand() < 0.4f ? 0 : (frand() * 2 - 1) * 40);
    }
    addRoad(30, 30, 30, 0, -seg[ns - 1].y / SEG);   // 收尾回到高度 0,頭尾接得起來
  }
  static void spawn(Car& c, float ahead) { c.z = wrapZ(pos + PZ + ahead * SEG); c.x = ((int)(frand() * 3) % 3 - 1) * LANE; c.v = (60 + frand() * 60) / KMH; c.pal = (int)(frand() * 8) % 8; c.truck = frand() < 0.2f; }
  void init() {
    build(); brake = false; pos = speed = px = steer = yaw = dist = bgX = endT = 0; over = false; rank = -1; active = 12;
    for (auto& c : cars) spawn(c, 30 + frand() * (ns - 60));
  }
  static float autopilot() {   // 前方 40 段內同線有車就換到另一邊
    float target = 0;
    for (int k = 0; k < active; k++) { float d = rel(cars[k]); if (d > 0 && d < 40 * SEG && fabsf(cars[k].x - px) < 0.5f) target = cars[k].x > 0 ? -LANE : LANE; }
    return fminf(fmaxf((target - px) * 2, -1), 1);
  }
  void step(const Ctx& c) {
    if (over) { if ((endT += c.dt) > 1.5f && c.tap) init(); return; }
    float dt = c.dt, vmax = fminf(150 + dist / 40, 300) / KMH, pct = speed / (300 / KMH), acc = vmax / 5;
    const Seg& ps = seg[(int)((pos + PZ) / SEG) % ns];
    steer = muted ? autopilot() : c.gx; px += steer * dt * 5 * pct;
    yaw += (fminf(fmaxf(steer * 1.5f, -1), 1) - yaw) * fminf(1, dt * 8);   // 車身轉向的畫面角度,平滑跟著方向盤
    brake = c.touch;
    px -= dt * 2 * pct * pct * ps.curve * 0.2f;   // 離心力
    bool off = fabsf(px) > 1;
    speed = brake ? fmaxf(0, speed - 3 * acc * dt) : off && speed > 80 / KMH ? speed - 2 * acc * dt : fminf(speed + acc * dt, vmax);
    px = fminf(fmaxf(px, -1.5f), 1.5f);
    bgX += ps.curve * pct * dt * 20;

    int want = 12 + (int)(dist / 300); if (want > NCAR) want = NCAR;
    while (active < want) spawn(cars[active++], DRAW + frand() * 50);
    float before[NCAR]; for (int k = 0; k < active; k++) before[k] = rel(cars[k]);
    pos = wrapZ(pos + speed * dt); dist += speed * dt / SEG;
    for (int k = 0; k < active; k++) {
      Car& a = cars[k]; a.z = wrapZ(a.z + a.v * dt);
      if (before[k] > 0 && rel(a) <= 0 && fabsf(a.x - px) < CARW / RW) {   // 這幀追過它的車尾,橫向又重疊就是撞上
        if (muted) { init(); return; }
        over = true; rank = board.record(dist > 65535 ? 65535 : (int)dist); buzz(200, 300); snd::note(150); return;
      }
    }
    if (off && frand() < 0.2f) buzz(40, 20);
  }

  static void project(Proj& p, float wy, float cz, float cx) { p.s = DEPTH / cz; p.x = W / 2 - p.s * cx * W / 2; p.y = HOR - p.s * (wy - camY) * H / 2; p.w = p.s * RW * W / 2; }
  static void quad(const Proj& a, const Proj& b, float off, float wk, uint32_t col) {   // 路面上以 off(路寬倍數)為中心、半寬 wk 倍路寬的梯形
    float x1 = a.x + off * a.w, w1 = a.w * wk, x2 = b.x + off * b.w, w2 = b.w * wk;
    cv.fillTriangle(x1 - w1, a.y, x1 + w1, a.y, x2 + w2, b.y, col); cv.fillTriangle(x1 - w1, a.y, x2 + w2, b.y, x2 - w2, b.y, col);
  }
  static uint32_t shade(const uint8_t* c, float k) { return rgb(fminf(c[0] * k, 255), fminf(c[1] * k, 255), fminf(c[2] * k, 255)); }
  static void trap(float cx0, float y0, float w0, float cx1, float y1, float w1, uint32_t col) {   // 上下兩條水平邊的梯形,w 是半寬
    cv.fillTriangle(cx0 - w0, y0, cx0 + w0, y0, cx1 + w1, y1, col); cv.fillTriangle(cx0 - w0, y0, cx1 + w1, y1, cx1 - w1, y1, col);
  }
  // 車尾視角,by 是車底;lean 是轉向時車頂往內偏(-1..1)。太小就只畫一塊色塊
  static void drawCar(float cx, float by, float w, const uint8_t* c, float lean = 0) {
    if (w < 2) return;
    float h = w * 0.55f, L = cx - w / 2;
    if (w < 10) { cv.fillRect(L, by - h * 0.8f, w, h * 0.8f, shade(c, 1)); return; }
    cv.fillEllipse(cx, by, w * 0.55f, w * 0.05f + 1, rgb(35, 35, 40));   // 影子
    cv.fillRoundRect(L + w * 0.06f, by - h * 0.24f, w * 0.18f, h * 0.24f, 2, rgb(15, 15, 15)); cv.fillRoundRect(L + w * 0.76f, by - h * 0.24f, w * 0.18f, h * 0.24f, 2, rgb(15, 15, 15));
    float y0 = by - h * 0.56f, y1 = by - h, lx = lean * w * 0.05f;
    trap(cx, y0, w * 0.42f, cx + lx, y1, w * 0.3f, shade(c, 0.8f));   // 車頂
    trap(cx, y0 - h * 0.04f, w * 0.36f, cx + lx, y1 + h * 0.07f, w * 0.25f, rgb(45, 60, 85));   // 後擋風玻璃
    cv.drawLine(cx + lx - w * 0.18f, y1 + h * 0.1f, cx - w * 0.28f, y0 - h * 0.06f, rgb(110, 130, 160));   // 玻璃反光
    cv.fillRoundRect(L, by - h * 0.6f, w, h * 0.42f, fmaxf(1, w * 0.06f), shade(c, 1));
    cv.drawFastHLine(L + w * 0.05f, by - h * 0.58f, w * 0.9f, shade(c, 1.3f));   // 行李箱邊緣亮線
    cv.fillRect(L + w * 0.03f, by - h * 0.26f, w * 0.94f, h * 0.1f, rgb(45, 45, 50));   // 保險桿
    for (int sd = 0; sd < 2; sd++) {   // 尾燈:暗紅外框 + 亮芯
      float lx0 = sd ? L + w * 0.73f : L + w * 0.05f;
      cv.fillRect(lx0, by - h * 0.52f, w * 0.22f, h * 0.13f, rgb(170, 15, 15)); cv.fillRect(lx0 + w * 0.04f, by - h * 0.49f, w * 0.14f, h * 0.06f, rgb(255, 90, 80));
    }
    cv.fillRect(cx - w * 0.1f, by - h * 0.46f, w * 0.2f, h * 0.1f, rgb(235, 235, 215));   // 車牌
  }
  static void quad4(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t col) { cv.fillTriangle(x0, y0, x1, y1, x2, y2, col); cv.fillTriangle(x0, y0, x2, y2, x3, y3, col); }
  // 玩家的超跑:低寬車身、後輪拱外擴、橫貫 LED 尾燈、擾流器 + 雙管排氣、鴨尾擾流板。
  // yaw(-1..1):往哪邊轉就露出那一側的側身(往遠處收,所以側面往上斜),車尾同時變窄、往反方向挪;brake 時尾燈變亮加光暈
  static void wheelSide(float x, float y, float rx, float ry) {   // 側面看到的輪子:胎 + 銀色輪圈
    cv.fillEllipse(x, y, rx, ry, rgb(30, 30, 30)); if (rx >= 3) cv.fillEllipse(x, y, rx * 0.55f, ry * 0.6f, rgb(160, 160, 168));
  }
  static void drawSuper(float cx0, float by, float w0, const uint8_t* c, float yaw, bool brake) {
    float a = fabsf(yaw), dir = yaw < 0 ? -1 : 1, sw = w0 * 0.34f * a, rise = sw * 0.35f;
    float w = w0 * (1 - 0.18f * a), cx = cx0 - dir * sw * 0.45f, h = w0 * 0.4f, L = cx - w / 2; uint32_t dark = rgb(28, 28, 32), tire = rgb(38, 38, 40);
    float yb = by - h * 0.26f, yt = by - h * 0.64f, yc = by - h;   // 車身底(離地)、腰線、車頂
    cv.fillEllipse(cx0, by, w0 * 0.55f, w0 * 0.04f + 1, rgb(25, 25, 28));
    if (sw >= 1) {
      float xb = cx + dir * w * 0.5f, xt = cx + dir * w * 0.44f, cb = cx + dir * w * 0.3f, ct = cx + dir * w * 0.19f;
      quad4(xb, yb, xt, yt, xt + dir * sw, yt - rise, xb + dir * sw, yb - rise, shade(c, 0.65f));   // 側身
      quad4(cb, yt, ct, yc, ct + dir * sw * 0.7f, yc - rise * 0.7f, cb + dir * sw * 0.8f, yt - rise * 0.8f, shade(c, 0.5f));   // 車艙側面
      quad4(cb + dir * sw * 0.15f, yt - h * 0.05f - rise * 0.15f, ct + dir * sw * 0.15f, yc + h * 0.07f - rise * 0.15f,
            ct + dir * sw * 0.6f, yc + h * 0.07f - rise * 0.6f, cb + dir * sw * 0.7f, yt - h * 0.05f - rise * 0.7f, rgb(45, 60, 85));   // 側窗
      cv.fillTriangle(xb + dir * sw * 0.35f, yb - rise * 0.35f, xt + dir * sw * 0.5f, yt + h * 0.06f - rise * 0.5f, xb + dir * sw * 0.6f, yb - rise * 0.6f, dark);   // 側面進氣口
      float rx = fmaxf(2, sw * 0.17f), ry = h * 0.23f;
      wheelSide(xb + dir * sw * 0.18f, by - ry - rise * 0.18f, rx, ry); wheelSide(xb + dir * sw * 0.84f, by - ry - rise * 0.84f, rx, ry);
    }
    for (int sd = -1; sd <= 1; sd += 2) {   // 後輪背面:先畫、上半被車身蓋住,車底到地面那段露出來,外側凸出車身一點;深灰加胎紋
      float tx = cx + sd * w * 0.4f - w * 0.13f, tw = w * 0.26f, th = h * 0.44f;
      cv.fillRoundRect(tx, by - th, tw, th, 3, tire);
      for (int k = 1; k < 4; k++) cv.drawFastHLine(tx + 2, by - th * k / 4, tw - 4, rgb(75, 75, 78));
    }
    trap(cx, yb, w * 0.5f, cx, yt, w * 0.44f, shade(c, 1));   // 下寬上窄的後輪拱
    trap(cx, yt, w * 0.3f, cx, yc, w * 0.19f, shade(c, 0.75f));   // 引擎蓋 / 車艙
    trap(cx, by - h * 0.9f, w * 0.2f, cx, by - h * 0.97f, w * 0.17f, rgb(45, 60, 85));   // 扁長後窗
    for (int k = 0; k < 3; k++) cv.drawFastHLine(cx - w * (0.22f - k * 0.02f), by - h * (0.76f + k * 0.05f), w * (0.44f - k * 0.04f), dark);   // 引擎蓋百葉
    trap(cx, by - h * 0.62f, w * 0.43f, cx, by - h * 0.7f, w * 0.41f, dark); cv.drawFastHLine(L + w * 0.09f, by - h * 0.7f, w * 0.82f, rgb(80, 80, 85));   // 鴨尾擾流板
    float ly = by - h * 0.52f, lh = fmaxf(2, h * (brake ? 0.1f : 0.07f));   // 橫貫 LED 尾燈,兩端 Y 形尖角;煞車時變亮變粗加光暈
    uint32_t led = brake ? rgb(255, 40, 40) : rgb(150, 15, 15), core = brake ? rgb(255, 210, 190) : rgb(210, 50, 40);
    if (brake) cv.fillRoundRect(L + w * 0.04f, ly - 3, w * 0.92f, lh + 6, 3, rgb(255, 120, 120));
    cv.fillRect(L + w * 0.08f, ly, w * 0.84f, lh, led); cv.drawFastHLine(L + w * 0.1f, ly + lh / 2, w * 0.8f, core);
    for (int sd = -1; sd <= 1; sd += 2) { float ex = cx + sd * w * 0.42f; cv.fillTriangle(ex, ly - h * 0.1f, ex, ly + lh + h * 0.1f, ex - sd * w * 0.07f, ly + lh / 2, led); }
    trap(cx, yb, w * 0.3f, cx, by - h * 0.4f, w * 0.26f, dark);   // 擾流器
    for (int k = -2; k <= 2; k++) cv.drawFastVLine(cx + k * w * 0.09f, by - h * 0.38f, h * 0.12f, rgb(70, 70, 75));
    for (int sd = -1; sd <= 1; sd += 2) { cv.fillCircle(cx + sd * w * 0.06f, by - h * 0.32f, w * 0.03f, rgb(110, 110, 115)); cv.fillCircle(cx + sd * w * 0.06f, by - h * 0.32f, w * 0.018f, rgb(10, 10, 10)); }   // 排氣管
    cv.fillRect(cx - w * 0.08f, by - h * 0.46f, w * 0.14f, h * 0.07f, rgb(235, 235, 215));
  }
  static void drawTruck(float cx, float by, float w, const uint8_t* c) {   // 貨車車尾:高箱 + 雙門
    if (w < 2) return;
    float h = w * 1.05f, L = cx - w / 2;
    if (w < 10) { cv.fillRect(L, by - h, w, h, shade(c, 1)); return; }
    cv.fillEllipse(cx, by, w * 0.55f, w * 0.05f + 1, rgb(35, 35, 40));
    cv.fillRect(L + w * 0.04f, by - h * 0.14f, w * 0.26f, h * 0.14f, rgb(15, 15, 15)); cv.fillRect(L + w * 0.7f, by - h * 0.14f, w * 0.26f, h * 0.14f, rgb(15, 15, 15));
    cv.fillRect(L, by - h, w, h * 0.84f, shade(c, 1)); cv.drawRect(L, by - h, w, h * 0.84f, shade(c, 0.6f));
    cv.drawFastVLine(cx, by - h * 0.97f, h * 0.78f, shade(c, 0.6f));   // 對開門縫
    for (float k : { 0.3f, 0.7f }) { cv.fillRect(L + w * 0.08f, by - h * (0.2f + k * 0.6f), w * 0.05f, h * 0.04f, rgb(90, 90, 95)); cv.fillRect(L + w * 0.87f, by - h * (0.2f + k * 0.6f), w * 0.05f, h * 0.04f, rgb(90, 90, 95)); }   // 鉸鏈
    cv.fillRect(L + w * 0.02f, by - h * 0.2f, w * 0.96f, h * 0.06f, rgb(45, 45, 50));
    cv.fillRect(L + w * 0.03f, by - h * 0.28f, w * 0.1f, h * 0.07f, rgb(230, 30, 30)); cv.fillRect(L + w * 0.87f, by - h * 0.28f, w * 0.1f, h * 0.07f, rgb(230, 30, 30));
    cv.fillRect(cx - w * 0.08f, by - h * 0.13f, w * 0.16f, h * 0.06f, rgb(235, 235, 215));
  }
  static void drawTree(float cx, float by, float h) {
    if (h < 3) return;
    cv.fillRect(cx - h * 0.05f, by - h * 0.35f, h * 0.1f, h * 0.35f, rgb(110, 70, 30));
    cv.fillTriangle(cx - h * 0.3f, by - h * 0.3f, cx + h * 0.3f, by - h * 0.3f, cx, by - h, rgb(30, 130, 50));
  }
  void draw() {
    uint32_t g1 = rgb(40, 160, 60), g2 = rgb(30, 140, 50), road = rgb(100, 100, 105);
    cv.fillScreen(rgb(90, 160, 235));
    for (int k = 0; k < 6; k++) { float bx = fmodf(k * 96 - bgX, 576); if (bx < 0) bx += 576; bx -= 128; cv.fillTriangle(bx, HOR, bx + 64, HOR - 30 - (k % 3) * 12, bx + 128, HOR, rgb(90, 110, 150)); }
    cv.fillRect(0, HOR, W, H - HOR, g1);

    int base = (int)(pos / SEG) % ns, pi = (int)((pos + PZ) / SEG) % ns;
    float pp = fmodf(pos + PZ, SEG) / SEG;
    camY = yStart(pi) + (seg[pi].y - yStart(pi)) * pp + CAMH;
    float x = 0, dx = -seg[base].curve * fmodf(pos, SEG) / SEG, maxy = H;
    for (int n = 0; n < DRAW; n++) {
      int i = (base + n) % ns; float z1 = (base + n) * SEG - pos;
      project(p1[n], yStart(i), z1, px * RW - x); project(p2[n], seg[i].y, z1 + SEG, px * RW - x - dx);
      x += dx; dx += seg[i].curve;
      clipY[n] = maxy; front[n] = z1 > DEPTH;
      if (!front[n] || p2[n].y >= p1[n].y || p2[n].y >= maxy) continue;
      bool alt = (i / 3) & 1;
      cv.fillRect(0, (int)p2[n].y, W, (int)p1[n].y - (int)p2[n].y + 1, alt ? g1 : g2);
      quad(p1[n], p2[n], 0, 1.15f, alt ? rgb(220, 40, 40) : rgb(240, 240, 240));
      quad(p1[n], p2[n], 0, 1, road);
      if (alt) { quad(p1[n], p2[n], -1 / 3.0f, 0.02f, rgb(240, 240, 240)); quad(p1[n], p2[n], 1 / 3.0f, 0.02f, rgb(240, 240, 240)); }
      maxy = p2[n].y;
    }
    for (int n = DRAW - 1; n > 0; n--) {   // 樹與車由遠到近畫,每段只露出比它近的路面沒蓋住的部分
      if (!front[n]) continue;
      int i = (base + n) % ns; const Proj& a = p1[n]; const Proj& b = p2[n];
      cv.setClipRect(0, 0, W, (int)clipY[n]);
      uint32_t h = i * 2654435761u;
      if ((h >> 20) % 5 == 0) { float ox = ((h >> 8) & 1 ? 1 : -1) * (1.7f + ((h >> 4) & 7) * 0.15f); drawTree(a.x + a.s * ox * RW * W / 2, a.y, a.s * TREE_H * W / 2); }
      for (int k = 0; k < active; k++) {
        const Car& c = cars[k]; if ((int)(c.z / SEG) % ns != i) continue;
        float t = fmodf(c.z, SEG) / SEG, s = a.s + (b.s - a.s) * t, cx = a.x + (b.x - a.x) * t + s * c.x * RW * W / 2;
        float cy = a.y + (b.y - a.y) * t, cw = s * CARW * W / 2;
        if (c.truck) drawTruck(cx, cy, cw * 1.1f, PAL[c.pal]); else drawCar(cx, cy, cw, PAL[c.pal]);
      }
    }
    cv.clearClipRect();
    float bounce = fabsf(px) > 1 && speed > 0 ? (frand() * 3) : 0;
    static const uint8_t ME[3] = { 220, 25, 25 };
    drawSuper(W / 2, H - 2 - bounce, CARW / CAMH * W / 2 * 1.1f, ME, yaw, brake);

    char t[24]; cv.setTextDatum(top_left); cv.setTextSize(2); cv.setTextColor(rgb(255, 255, 255));
    snprintf(t, sizeof t, "%3d km/h", (int)(speed * KMH)); cv.drawString(t, 4, 4);
    cv.setTextSize(1); snprintf(t, sizeof t, "%d m   best %u", (int)dist, board.best()); cv.drawString(t, 4, 24);
    if (over) { snprintf(t, sizeof t, "%d m", (int)dist); board.draw(t, rank); }
  }
}
