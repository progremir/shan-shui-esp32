#include "cases.h"
#include "shanshui/tree.h"

using namespace ss;

SS_CASE("tree01_a", "a") {
  TreeArgs a;
  return tree01(100, 300, a).svg();
}

SS_CASE("tree01_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree01(100, 300, a).svg();
}

SS_CASE("tree01_1234", "1234") {
  TreeArgs a;
  return tree01(100, 300, a).svg();
}

SS_CASE("tree02_a", "a") {
  TreeArgs a;
  return tree02(200, 300, a).svg();
}

SS_CASE("tree02_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree02(200, 300, a).svg();
}

SS_CASE("tree02_1234", "1234") {
  TreeArgs a;
  return tree02(200, 300, a).svg();
}

SS_CASE("tree03_a", "a") {
  TreeArgs a;
  return tree03(100, 300, a).svg();
}

SS_CASE("tree03_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree03(100, 300, a).svg();
}

SS_CASE("tree03_1234", "1234") {
  TreeArgs a;
  return tree03(100, 300, a).svg();
}

SS_CASE("tree04_a", "a") {
  TreeArgs a;
  return tree04(300, 500, a).svg();
}

SS_CASE("tree04_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree04(300, 500, a).svg();
}

SS_CASE("tree04_1234", "1234") {
  TreeArgs a;
  return tree04(300, 500, a).svg();
}

SS_CASE("tree05_a", "a") {
  TreeArgs a;
  return tree05(300, 500, a).svg();
}

SS_CASE("tree05_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree05(300, 500, a).svg();
}

SS_CASE("tree05_1234", "1234") {
  TreeArgs a;
  return tree05(300, 500, a).svg();
}

SS_CASE("tree06_a", "a") {
  TreeArgs a;
  return tree06(300, 500, a).svg();
}

SS_CASE("tree06_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree06(300, 500, a).svg();
}

SS_CASE("tree06_1234", "1234") {
  TreeArgs a;
  return tree06(300, 500, a).svg();
}

SS_CASE("tree07_a", "a") {
  TreeArgs a;
  return tree07(200, 400, a).svg();
}

SS_CASE("tree07_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree07(200, 400, a).svg();
}

SS_CASE("tree07_1234", "1234") {
  TreeArgs a;
  return tree07(200, 400, a).svg();
}

SS_CASE("tree08_a", "a") {
  TreeArgs a;
  return tree08(200, 400, a).svg();
}

SS_CASE("tree08_wallpaper", "wallpaper") {
  TreeArgs a;
  return tree08(200, 400, a).svg();
}

SS_CASE("tree08_1234", "1234") {
  TreeArgs a;
  return tree08(200, 400, a).svg();
}

SS_CASE("tree01_args", "v1") {
  TreeArgs a;
  a.hei = 73.5;
  a.wid = 2.4;
  a.col = "rgba(100,100,100,0.412)";
  return tree01(50, 200, a).svg();
}

SS_CASE("tree01_nocol", "v2") {
  TreeArgs a;
  a.col = "black";
  return tree01(50, 200, a).svg();
}

SS_CASE("tree02_clu", "v3") {
  TreeArgs a;
  a.col = "rgba(100,100,100,0.623)";
  a.clu = 2;
  return tree02(80, 120, a).svg();
}

SS_CASE("tree03_ben", "v4") {
  TreeArgs a;
  a.hei = 64.2;
  a.ben = [](real x) { return pow(x * 0.7, 1.3); };
  a.col = "rgba(100,100,100,0.351)";
  return tree03(80, 220, a).svg();
}

SS_CASE("tree04_b", "v5") {
  TreeArgs a;
  return tree04(123.4, 456.7, a).svg();
}

SS_CASE("tree05_h0", "v60") {
  TreeArgs a;
  a.hei = 137.2;
  return tree05(400, 500, a).svg();
}

SS_CASE("tree05_h1", "v61") {
  TreeArgs a;
  a.hei = 288.9;
  return tree05(400, 500, a).svg();
}

SS_CASE("tree05_h2", "v62") {
  TreeArgs a;
  a.hei = 105;
  return tree05(400, 500, a).svg();
}

SS_CASE("tree06_h0", "v70") {
  TreeArgs a;
  a.hei = 67.3;
  return tree06(400, 500, a).svg();
}

SS_CASE("tree06_h1", "v71") {
  TreeArgs a;
  a.hei = 119.1;
  return tree06(400, 500, a).svg();
}

SS_CASE("tree06_h2", "v72") {
  TreeArgs a;
  a.hei = 85;
  return tree06(400, 500, a).svg();
}

SS_CASE("tree07_h0", "v80") {
  TreeArgs a;
  a.hei = 44.4;
  return tree07(300, 400, a).svg();
}

SS_CASE("tree07_h1", "v81") {
  TreeArgs a;
  a.hei = 78.9;
  return tree07(300, 400, a).svg();
}

SS_CASE("tree08_h0", "v90") {
  TreeArgs a;
  a.hei = 63.7;
  return tree08(300, 400, a).svg();
}

SS_CASE("tree08_h1", "v91") {
  TreeArgs a;
  a.hei = 99.2;
  return tree08(300, 400, a).svg();
}
