#include "cases.h"
#include "shanshui/mount.h"

using namespace ss;

SS_CASE("rock1", "r1") { return rock(100, 200, 5).svg(); }

SS_CASE("rock2", "r2") {
  RockArgs a;
  a.wid = 30;
  a.hei = 25;
  a.sha = 2;
  return rock(50, 60, 3.3, a).svg();
}

SS_CASE("rock3", "r3") {
  RockArgs a;
  a.wid = 60;
  a.hei = 50;
  a.sha = 5;
  a.tex = 25;
  return rock(-20, 400, 77.7, a).svg();
}

SS_CASE("distDefault", "d1") { return distMount(0, 300, 12).svg(); }

// chunkloader: Mount.distMount(x, y, Math.random() * 100, {hei: 150, len: randChoice([500, 1000, 1500])})
static std::string distChunk(real x, real y) {
  real s = rnd() * 100;
  DistMountArgs a;
  a.hei = 150;
  a.len = randChoice<real>({500, 1000, 1500});
  return distMount(x, y, s, a).svg();
}
SS_CASE("distChunk1", "d2") { return distChunk(1200, 180); }
SS_CASE("distChunk2", "d3") { return distChunk(-300, 220); }
SS_CASE("distChunk3", "d4") { return distChunk(700, 250); }

SS_CASE("mountPts", "m0") {
  std::vector<Pts> r = mountainPts(100, 200, 1);
  Pts p = r[0];
  for (auto& v : r[9]) p.push_back(v);
  return pts_json(p, 6);
}

// chunkloader: Mount.mountain(x, y, i * 2 * Math.random())
static std::string mountChunk(real x, real y, real i) {
  real s = i * 2 * rnd();
  return mountain(x, y, s).svg();
}
SS_CASE("mountChunk1", "m1") { return mountChunk(500, 300, 1); }
SS_CASE("mountChunk2", "m2") { return mountChunk(-200, 450, 3); }
SS_CASE("mountChunk3", "wallpaper") { return mountChunk(900, 380, 0); }
SS_CASE("mountChunk4", "m4") { return mountChunk(1500, 500, 5); }

SS_CASE("mountNoVeg", "m5") {
  MountArgs a;
  a.veg = false;
  a.hei = 250;
  a.wid = 500;
  a.tex = 120;
  return mountain(300, 260, 4.5, a).svg();
}

// chunkloader: Mount.flatMount(x, y, 2 * Math.random() * Math.PI,
//   {wid: 600 + Math.random() * 400, hei: 100, cho: 0.5 + Math.random() * 0.2})
static std::string flatChunk(real x, real y) {
  real s = 2 * rnd() * PI;
  FlatMountArgs a;
  a.wid = 600 + rnd() * 400;
  a.hei = 100;
  a.cho = 0.5 + rnd() * 0.2;
  return flatMount(x, y, s, a).svg();
}
SS_CASE("flatChunk1", "f1") { return flatChunk(800, 350); }
SS_CASE("flatChunk2", "f2") { return flatChunk(-400, 420); }
SS_CASE("flatChunk3", "f3") { return flatChunk(100, 300); }
SS_CASE("flatDefault", "f4") { return flatMount(200, 380, 3).svg(); }

static std::string flatDecCase() {
  Bound b;
  b.xmin = -250;
  b.xmax = 220;
  b.ymin = -40;
  b.ymax = 12;
  return flatDec(100, 300, b).svg();
}
SS_CASE("flatDec1", "fd1") { return flatDecCase(); }
SS_CASE("flatDec2", "fd2") { return flatDecCase(); }
SS_CASE("flatDec3", "fd3") { return flatDecCase(); }
SS_CASE("flatDec4", "fd4") { return flatDecCase(); }
SS_CASE("flatDec5", "fd5") { return flatDecCase(); }
SS_CASE("flatDec6", "fd6") { return flatDecCase(); }
SS_CASE("flatDec7", "fd7") { return flatDecCase(); }
SS_CASE("flatDec8", "fd8") { return flatDecCase(); }
