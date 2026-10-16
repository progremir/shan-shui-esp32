#include "cases.h"

using namespace ss;

SS_CASE("rnd", "wallpaper") {
  double a = rnd(), b = rnd(), c = rnd();
  return fixed_list({a, b, c}, 15);
}

SS_CASE("noise", "wallpaper") {
  double a = noise(0.3), b = noise(12.7, 3.1), c = noise(1000.25, 2, 0.5), d = noise(-5.5);
  return fixed_list({a, b, c, d}, 15);
}

SS_CASE("poly", "a") {
  PolyArgs a;
  a.xof = 1;
  a.fil = "rgba(100,100,100,0.3)";
  a.wid = 1.5;
  return poly({{0, 0}, {10.25, 5.75}, {-0.25, -3.05}}, a).svg();
}

SS_CASE("stroke", "b") {
  StrokeArgs a;
  a.wid = 3;
  a.col = "rgba(100,100,100,0.5)";
  return stroke({{0, 0}, {10, 5}, {20, 0}, {30, 10}, {40, 3}}, a).svg();
}

SS_CASE("blob", "c") {
  BlobArgs a;
  a.len = 30;
  a.wid = 10;
  a.ang = 0.3;
  return blob(50, 60, a).svg();
}

SS_CASE("div", "c") { return pts_json(div({{0, 0}, {10, 5}, {20, -4}}, 3), 6); }

SS_CASE("bezmh", "c") { return pts_json(bezmh({{0, 0}, {10, 5}, {20, -4}, {30, 8}}, 1.5), 6); }

SS_CASE("texture", "d") {
  std::vector<Pts> l = {{{0, 0}, {10, 1}, {20, 0}, {30, 2}, {40, 0}, {50, 1}},
                        {{0, 10}, {10, 11}, {20, 10}, {30, 12}, {40, 10}, {50, 11}},
                        {{0, 20}, {10, 21}, {20, 20}, {30, 22}, {40, 20}, {50, 21}}};
  TextureArgs a;
  a.tex = 30;
  a.sha = 1;
  return texture(l, a).svg();
}

SS_CASE("water", "e") { return water(100, 400, 3).svg(); }

SS_CASE("gauss", "f") {
  double a = randGaussian();
  double b = normRand(3, 9);
  double c = wtrand([](real x) { return x * x; });
  return fixed_list({a, b, c}, 15);
}

SS_CASE("triangulate", "g") {
  TriArgs a;
  a.area = 200;
  a.convex = true;
  a.optimize = false;
  return tris_json(triangulate({{0, 0}, {40, 5}, {60, 40}, {30, 25}, {5, 50}}, a), 4);
}

SS_CASE("triangulate2", "g") {
  TriArgs a;
  a.area = 100;
  return tris_json(triangulate({{0, 0}, {40, 5}, {60, 40}, {30, 25}, {5, 50}}, a), 4);
}
