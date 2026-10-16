#include "arch.h"

#include <algorithm>

// JavaScript never fuses a*b+c; clang on arm64 does by default
// (-ffp-contract=on), which changes e.g. boat01's loop count.
#ifdef __clang__
#pragma STDC FP_CONTRACT OFF
#endif

namespace ss {

namespace {

const char* const kGray3 = "rgba(100,100,100,0.3)";
const char* const kGray4 = "rgba(100,100,100,0.4)";
const char* const kGray5 = "rgba(100,100,100,0.5)";

Pts offset(const Pts& src, real xoff, real yoff) {
  Pts out = src;
  for (auto& v : out) {
    v.x = v.x + xoff;
    v.y = v.y + yoff;
  }
  return out;
}

Pts reversed(Pts p) {
  std::reverse(p.begin(), p.end());
  return p;
}

Pts concat(Pts a, const Pts& b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}

Pts sliceLast(const Pts& p) {  // p.slice(0, -1)
  return p.empty() ? Pts() : Pts(p.begin(), p.end() - 1);
}

Pts flip(Pts ptlist, real axis = 0) {
  for (auto& v : ptlist) v.x = axis - (v.x - axis);
  return ptlist;
}

Fn one() {
  return [](real) { return (real)1; };
}

// ================================================================ hut

Canvas hut(real xoff, real yoff, real hei, real wid, real tex = 300) {
  const int reso[2] = {10, 10};
  std::vector<Pts> ptlist;
  for (int i = 0; i < reso[0]; i++) {
    ptlist.push_back(Pts());
    real heir = hei + hei * 0.2 * rnd();
    for (int j = 0; j < reso[1]; j++) {
      real nx = wid * ((real)i / (reso[0] - 1) - 0.5) * pow((real)j / (reso[1] - 1), 0.7);
      real ny = heir * ((real)j / (reso[1] - 1));
      ptlist.back().push_back(Pt(nx, ny));
    }
  }
  Canvas canv;
  PolyArgs pa;
  pa.xof = xoff;
  pa.yof = yoff;
  pa.fil = "white";
  pa.str = "none";
  canv.add(poly(concat(sliceLast(ptlist[0]), reversed(sliceLast(ptlist.back()))), pa));
  pa.fil = "none";
  pa.str = kGray3;
  pa.wid = 2;
  canv.add(poly(ptlist[0], pa));
  canv.add(poly(ptlist.back(), pa));

  TextureArgs ta;
  ta.xof = xoff;
  ta.yof = yoff;
  ta.tex = tex;
  ta.wid = 1;
  ta.len = 0.25;
  ta.col = [](real) { return "rgba(120,120,120," + fixed(0.3 + rnd() * 0.3, 3) + ")"; };
  ta.dis = []() { return wtrand([](real a) { return a * a; }); };
  ta.noi = [](real) { return (real)5; };
  canv.add(texture(ptlist, ta));
  return canv;
}

// ================================================================ box

struct DecoArgs {
  Pt pul = Pt(0, 0), pur = Pt(0, 100), pdl = Pt(100, 0), pdr = Pt(100, 100);
  int hsp[2] = {1, 3};
  int vsp[2] = {1, 2};
};

std::vector<Pts> deco(real style, const DecoArgs& a) {
  std::vector<Pts> plist;
  Pts dl = div({a.pul, a.pdl}, a.vsp[1]);
  Pts dr = div({a.pur, a.pdr}, a.vsp[1]);
  Pts du = div({a.pul, a.pur}, a.hsp[1]);
  Pts dd = div({a.pdl, a.pdr}, a.hsp[1]);
  int h0 = a.hsp[0], v0 = a.vsp[0];
  if (style == 1) {
    Pt mlu = du[h0];
    Pt mru = du[du.size() - 1 - h0];
    Pt mld = dd[h0];
    Pt mrd = dd[du.size() - 1 - h0];
    for (int i = v0; i < (int)dl.size() - v0; i += v0) {
      Pt mml = div({mlu, mld}, a.vsp[1])[i];
      Pt mmr = div({mru, mrd}, a.vsp[1])[i];
      Pt ml = dl[i];
      Pt mr = dr[i];
      plist.push_back(div({mml, ml}, 5));
      plist.push_back(div({mmr, mr}, 5));
    }
    plist.push_back(div({mlu, mld}, 5));
    plist.push_back(div({mru, mrd}, 5));
  } else if (style == 2) {
    for (int i = h0; i < (int)du.size() - h0; i += h0) {
      plist.push_back(div({du[i], dd[i]}, 5));
    }
  } else if (style == 3) {
    Pt mlu = du[h0];
    Pt mru = du[du.size() - 1 - h0];
    Pt mld = dd[h0];
    Pt mrd = dd[du.size() - 1 - h0];
    for (int i = v0; i < (int)dl.size() - v0; i += v0) {
      Pt mml = div({mlu, mld}, a.vsp[1])[i];
      Pt mmr = div({mru, mrd}, a.vsp[1])[i];
      Pt mmu = div({mlu, mru}, a.vsp[1])[i];
      Pt mmd = div({mld, mrd}, a.vsp[1])[i];
      plist.push_back(div({mml, mmr}, 5));
      plist.push_back(div({mmu, mmd}, 5));
    }
    plist.push_back(div({mlu, mld}, 5));
    plist.push_back(div({mru, mrd}, 5));
  }
  return plist;
}

struct BoxArgs {
  real hei = 20, wid = 120, rot = 0.7, per = 4;
  bool tra = true, bot = true;
  real wei = 3;
  std::function<std::vector<Pts>(const DecoArgs&)> dec;  // empty: none
};

Canvas box(real xoff, real yoff, const BoxArgs& args) {
  real hei = args.hei, wid = args.wid, rot = args.rot, per = args.per;
  real mid = -wid * 0.5 + wid * rot;
  real bmid = -wid * 0.5 + wid * (1 - rot);
  std::vector<Pts> ptlist;
  ptlist.push_back(div({{-wid * 0.5, -hei}, {-wid * 0.5, 0}}, 5));
  ptlist.push_back(div({{wid * 0.5, -hei}, {wid * 0.5, 0}}, 5));
  if (args.bot) {
    ptlist.push_back(div({{-wid * 0.5, 0}, {mid, per}}, 5));
    ptlist.push_back(div({{wid * 0.5, 0}, {mid, per}}, 5));
  }
  ptlist.push_back(div({{mid, -hei}, {mid, per}}, 5));
  if (args.tra) {
    if (args.bot) {
      ptlist.push_back(div({{-wid * 0.5, 0}, {bmid, -per}}, 5));
      ptlist.push_back(div({{wid * 0.5, 0}, {bmid, -per}}, 5));
    }
    ptlist.push_back(div({{bmid, -hei}, {bmid, -per}}, 5));
  }

  real surf = (rot < 0.5) * 2 - 1;
  if (args.dec) {
    DecoArgs d;
    d.pul = Pt(surf * wid * 0.5, -hei);
    d.pur = Pt(mid, -hei + per);
    d.pdl = Pt(surf * wid * 0.5, 0);
    d.pdr = Pt(mid, per);
    for (auto& p : args.dec(d)) ptlist.push_back(p);
  }

  Pts polist = {{-wid * 0.5, -hei}, {wid * 0.5, -hei}, {wid * 0.5, 0}, {mid, per}, {-wid * 0.5, 0}};

  Canvas canv;
  if (!args.tra) {
    PolyArgs pa;
    pa.xof = xoff;
    pa.yof = yoff;
    pa.str = "none";
    pa.fil = "white";
    canv.add(poly(polist, pa));
  }
  for (auto& pl : ptlist) {
    StrokeArgs sa;
    sa.col = kGray4;
    sa.noi = 1;
    sa.wid = args.wei;
    sa.fun = one();
    canv.add(stroke(offset(pl, xoff, yoff), sa));
  }
  return canv;
}

// ================================================================ rail

struct RailArgs {
  real hei = 20, wid = 180, rot = 0.7, per = 4, seg = 4, wei = 1;
  bool tra = true, fro = true;
};

Canvas rail(real xoff, real yoff, real seed, const RailArgs& args) {
  real hei = args.hei, wid = args.wid, rot = args.rot, per = args.per, seg = args.seg;
  real mid = -wid * 0.5 + wid * rot;
  real bmid = -wid * 0.5 + wid * (1 - rot);
  std::vector<Pts> ptlist;
  if (args.fro) {
    ptlist.push_back(div({{-wid * 0.5, 0}, {mid, per}}, seg));
    ptlist.push_back(div({{mid, per}, {wid * 0.5, 0}}, seg));
  }
  if (args.tra) {
    ptlist.push_back(div({{-wid * 0.5, 0}, {bmid, -per}}, seg));
    ptlist.push_back(div({{bmid, -per}, {wid * 0.5, 0}}, seg));
  }
  if (args.fro) {
    ptlist.push_back(div({{-wid * 0.5, -hei}, {mid, -hei + per}}, seg));
    ptlist.push_back(div({{mid, -hei + per}, {wid * 0.5, -hei}}, seg));
  }
  if (args.tra) {
    ptlist.push_back(div({{-wid * 0.5, -hei}, {bmid, -hei - per}}, seg));
    ptlist.push_back(div({{bmid, -hei - per}, {wid * 0.5, -hei}}, seg));
  }
  size_t n = ptlist.size();
  if (args.tra) {
    size_t open = (size_t)floor(rnd() * n);
    // (open + n) % n == open: the same list loses two points.
    ptlist[open] = sliceLast(ptlist[open]);
    ptlist[(open + n) % n] = sliceLast(ptlist[(open + n) % n]);
  }
  Canvas canv;
  for (size_t i = 0; i < n / 2; i++) {
    for (size_t j = 0; j < ptlist[i].size(); j++) {
      Pts& other = ptlist[(n / 2 + i) % n];
      ptlist[i][j].y += (noise(i, j * 0.5, seed) - 0.5) * hei;
      other[j % other.size()].y += (noise(i + 0.5, j * 0.5, seed) - 0.5) * hei;
      Pts ln = div({ptlist[i][j], other[j % other.size()]}, 2);
      ln[0].x += (rnd() - 0.5) * hei * 0.5;
      PolyArgs pa;
      pa.xof = xoff;
      pa.yof = yoff;
      pa.fil = "none";
      pa.str = kGray5;
      pa.wid = 2;
      canv.add(poly(ln, pa));
    }
  }
  for (auto& pl : ptlist) {
    StrokeArgs sa;
    sa.col = kGray5;
    sa.noi = 0.5;
    sa.wid = args.wei;
    sa.fun = one();
    canv.add(stroke(offset(pl, xoff, yoff), sa));
  }
  return canv;
}

// ================================================================ roofs

struct RoofArgs {
  real hei = 20, wid = 120, rot = 0.7, per = 4, cor = 5, wei = 3;
  bool pla = false;  // pla[0] == 1
};

Canvas roof(real xoff, real yoff, const RoofArgs& args) {
  real hei = args.hei, wid = args.wid, rot = args.rot, per = args.per, cor = args.cor;
  auto opf = [&](Pts p) { return rot < 0.5 ? flip(p) : p; };
  real rrot = rot < 0.5 ? 1 - rot : rot;
  real mid = -wid * 0.5 + wid * rrot;
  real quat = (mid + wid * 0.5) * 0.5 - mid;

  std::vector<Pts> ptlist;
  ptlist.push_back(div(opf({{-wid * 0.5 + quat, -hei - per / 2},
                            {-wid * 0.5 + quat * 0.5, -hei / 2 - per / 4},
                            {-wid * 0.5 - cor, 0}}),
                       5));
  ptlist.push_back(div(opf({{mid + quat, -hei},
                            {(mid + quat + wid * 0.5) / 2, -hei / 2},
                            {wid * 0.5 + cor, 0}}),
                       5));
  ptlist.push_back(div(opf({{mid + quat, -hei},
                            {mid + quat / 2, -hei / 2 + per / 2},
                            {mid + cor, per}}),
                       5));
  ptlist.push_back(div(opf({{-wid * 0.5 - cor, 0}, {mid + cor, per}}), 5));
  ptlist.push_back(div(opf({{wid * 0.5 + cor, 0}, {mid + cor, per}}), 5));
  ptlist.push_back(div(opf({{-wid * 0.5 + quat, -hei - per / 2}, {mid + quat, -hei}}), 5));

  Canvas canv;
  Pts polist = opf({{-wid * 0.5, 0},
                    {-wid * 0.5 + quat, -hei - per / 2},
                    {mid + quat, -hei},
                    {wid * 0.5, 0},
                    {mid, per}});
  PolyArgs pa;
  pa.xof = xoff;
  pa.yof = yoff;
  pa.str = "none";
  pa.fil = "white";
  canv.add(poly(polist, pa));

  for (auto& pl : ptlist) {
    StrokeArgs sa;
    sa.col = kGray4;
    sa.noi = 1;
    sa.wid = args.wei;
    sa.fun = one();
    canv.add(stroke(offset(pl, xoff, yoff), sa));
  }
  // pla[0] == 1 adds a <text> sign ("Pizza Hut"); Canvas holds polylines
  // only, so it is left out. It uses no randomness.
  return canv;
}

struct PagroofArgs {
  real hei = 20, wid = 120, rot = 0.7, per = 4, cor = 10, sid = 4, wei = 3;
};

Canvas pagroof(real xoff, real yoff, const PagroofArgs& args) {
  real hei = args.hei, wid = args.wid, per = args.per, cor = args.cor, sid = args.sid;
  std::vector<Pts> ptlist;
  Pts polist = {{0, -hei}};
  Canvas canv;
  for (int i = 0; i < sid; i++) {
    real fx = wid * ((i * 1.0) / (sid - 1) - 0.5);
    real fy = per * (1 - fabs((i * 1.0) / (sid - 1) - 0.5) * 2);
    real fxx = (wid + cor) * ((i * 1.0) / (sid - 1) - 0.5);
    if (i > 0) ptlist.push_back(Pts{ptlist.back()[2], Pt(fxx, fy)});
    ptlist.push_back(Pts{{0, -hei}, {fx * 0.5, (-hei + fy) * 0.5}, {fxx, fy}});
    polist.push_back(Pt(fxx, fy));
  }
  PolyArgs pa;
  pa.xof = xoff;
  pa.yof = yoff;
  pa.str = "none";
  pa.fil = "white";
  canv.add(poly(polist, pa));
  for (auto& pl : ptlist) {
    StrokeArgs sa;
    sa.col = kGray4;
    sa.noi = 1;
    sa.wid = args.wei;
    sa.fun = one();
    canv.add(stroke(offset(div(pl, 5), xoff, yoff), sa));
  }
  return canv;
}

// ================================================================ Man helpers

void expand(const Pts& ptlist, const Fn& wfun, Pts* out0, Pts* out1) {
  Pts vtxlist0, vtxlist1;
  real n0 = rnd() * 10;
  (void)n0;
  size_t n = ptlist.size();
  for (size_t i = 1; i + 1 < n; i++) {
    real w = wfun((real)i / n);
    real a1 = atan2(ptlist[i].y - ptlist[i - 1].y, ptlist[i].x - ptlist[i - 1].x);
    real a2 = atan2(ptlist[i].y - ptlist[i + 1].y, ptlist[i].x - ptlist[i + 1].x);
    real a = (a1 + a2) / 2;
    if (a < a2) a += PI;
    vtxlist0.push_back(Pt(ptlist[i].x + w * cos(a), ptlist[i].y + w * sin(a)));
    vtxlist1.push_back(Pt(ptlist[i].x - w * cos(a), ptlist[i].y - w * sin(a)));
  }
  size_t l = n - 1;
  real a0 = atan2(ptlist[1].y - ptlist[0].y, ptlist[1].x - ptlist[0].x) - PI / 2;
  real a1 = atan2(ptlist[l].y - ptlist[l - 1].y, ptlist[l].x - ptlist[l - 1].x) - PI / 2;
  real w0 = wfun(0);
  real w1 = wfun(1);
  vtxlist0.insert(vtxlist0.begin(), Pt(ptlist[0].x + w0 * cos(a0), ptlist[0].y + w0 * sin(a0)));
  vtxlist1.insert(vtxlist1.begin(), Pt(ptlist[0].x - w0 * cos(a0), ptlist[0].y - w0 * sin(a0)));
  vtxlist0.push_back(Pt(ptlist[l].x + w1 * cos(a1), ptlist[l].y + w1 * sin(a1)));
  vtxlist1.push_back(Pt(ptlist[l].x - w1 * cos(a1), ptlist[l].y - w1 * sin(a1)));
  *out0 = std::move(vtxlist0);
  *out1 = std::move(vtxlist1);
}

Pts flipper(const Pts& plist) {
  Pts out;
  for (auto& v : plist) out.push_back(Pt(-v.x, v.y));
  return out;
}

Pts tranpoly(const Pt& p0, const Pt& p1, const Pts& ptlist) {
  Pts plist = flipper(ptlist);
  real ang = atan2(p1.y - p0.y, p1.x - p0.x) - PI / 2;
  real scl = distance(p0, p1);
  Pts qlist;
  for (auto& v : plist) {
    real d = distance(v, Pt(0, 0));
    real a = atan2(v.y, v.x);
    qlist.push_back(Pt(p0.x + d * scl * cos(ang + a), p0.y + d * scl * sin(ang + a)));
  }
  return qlist;
}

Pts maybeFlip(bool fli, const Pts& p) { return fli ? flipper(p) : p; }

}  // namespace

// ================================================================ Man

Canvas hat01(const Pt& p0, const Pt& p1, bool fli) {
  Canvas canv;
  real seed = rnd();
  PolyArgs pa;
  pa.fil = "rgba(100,100,100,0.8)";
  canv.add(poly(tranpoly(p0, p1,
                         maybeFlip(fli, {{-0.3, 0.5},
                                         {0.3, 0.8},
                                         {0.2, 1},
                                         {0, 1.1},
                                         {-0.3, 1.15},
                                         {-0.55, 1},
                                         {-0.65, 0.5}})),
                pa));
  Pts qlist1;
  for (int i = 0; i < 10; i++) {
    qlist1.push_back(Pt(-0.3 - noise(i * 0.2, seed) * i * 0.1, 0.5 - i * 0.3));
  }
  PolyArgs pb;
  pb.str = "rgba(100,100,100,0.8)";
  pb.wid = 1;
  canv.add(poly(tranpoly(p0, p1, maybeFlip(fli, qlist1)), pb));
  return canv;
}

Canvas hat02(const Pt& p0, const Pt& p1, bool fli) {
  Canvas canv;
  real seed = rnd();
  (void)seed;
  PolyArgs pa;
  pa.fil = "rgba(100,100,100,0.8)";
  canv.add(poly(tranpoly(p0, p1,
                         maybeFlip(fli, {{-0.3, 0.5},
                                         {-1.1, 0.5},
                                         {-1.2, 0.6},
                                         {-1.1, 0.7},
                                         {-0.3, 0.8},
                                         {0.3, 0.8},
                                         {1.0, 0.7},
                                         {1.3, 0.6},
                                         {1.2, 0.5},
                                         {0.3, 0.5}})),
                pa));
  return canv;
}

Canvas stick01(const Pt& p0, const Pt& p1, bool fli) {
  Canvas canv;
  real seed = rnd();
  Pts qlist1;
  int l = 12;
  for (int i = 0; i < l; i++) {
    qlist1.push_back(Pt(-noise(i * 0.1, seed) * 0.1 * sin(((real)i / l) * PI) * 5, 0 + i * 0.3));
  }
  PolyArgs pa;
  pa.str = kGray5;
  pa.wid = 1;
  canv.add(poly(tranpoly(p0, p1, maybeFlip(fli, qlist1)), pa));
  return canv;
}

//      2
//    1/
// 7/  | \_ 6
// 8| 0 \ 5
//      /3
//     4
Canvas man(real xoff, real yoff, const ManArgs& args) {
  real sca = args.sca;
  bool fli = args.fli;
  std::vector<real> ang;
  if (args.ang) {
    ang = *args.ang;
  } else {
    ang.push_back(0);
    ang.push_back(-PI / 2);
    ang.push_back(normRand(0, 0));
    ang.push_back((PI / 4) * rnd());
    ang.push_back(((PI * 3) / 4) * rnd());
    ang.push_back((PI * 3) / 4);
    ang.push_back(-PI / 4);
    ang.push_back((-PI * 3) / 4 - (PI / 4) * rnd());
    ang.push_back(-PI / 4);
  }
  std::vector<real> len = args.len;
  for (auto& v : len) v = v * sca;
  Canvas canv;

  // gpar() over the skeleton tree: the path from the root to each joint.
  static const std::vector<int> par[9] = {{0},       {0, 1},    {0, 1, 2},
                                          {0, 3},    {0, 3, 4}, {0, 1, 5},
                                          {0, 1, 5, 6}, {0, 1, 7}, {0, 1, 7, 8}};
  auto grot = [&](int ind) {
    real rot = 0;
    for (int p : par[ind]) rot += ang[p];
    return rot;
  };
  auto gpos = [&](int ind) {
    Pt pos(0, 0);
    for (int p : par[ind]) {
      real a = grot(p);
      pos.x += len[p] * cos(a);
      pos.y += len[p] * sin(a);
    }
    return pos;
  };
  Pts pts;
  for (size_t i = 0; i < ang.size() && i < 9; i++) pts.push_back(gpos((int)i));
  yoff -= pts[4].y;

  auto toGlobal = [&](const Pt& v) { return Pt((fli ? -1 : 1) * v.x + xoff, v.y + yoff); };
  auto mapG = [&](const Pts& p) {
    Pts out;
    for (auto& v : p) out.push_back(toGlobal(v));
    return out;
  };

  auto cloth = [&](const Pts& plist, const Fn& fun) {
    Canvas c;
    Pts tlist = bezmh(plist, 2);
    Pts tlist1, tlist2;
    expand(tlist, fun, &tlist1, &tlist2);
    std::reverse(tlist2.begin(), tlist2.end());  // reverse() mutates tlist2
    PolyArgs pa;
    pa.fil = "white";
    c.add(poly(mapG(concat(tlist1, tlist2)), pa));
    StrokeArgs sa;
    sa.wid = 1;
    sa.col = kGray5;
    c.add(stroke(mapG(tlist1), sa));
    sa.col = "rgba(100,100,100,0.6)";
    c.add(stroke(mapG(tlist2), sa));
    return c;
  };

  Fn fsleeve = [sca](real x) {
    return sca * 8 * (sin(0.5 * x * PI) * pow(sin(x * PI), 0.1) + (1 - x) * 0.4);
  };
  Fn fbody = [sca](real x) {
    return sca * 11 * (sin(0.5 * x * PI) * pow(sin(x * PI), 0.1) + (1 - x) * 0.5);
  };
  Fn fhead = [sca](real x) { return sca * 7 * pow(0.25 - pow(x - 0.5, 2), 0.3); };

  if (args.ite) canv.add(args.ite(toGlobal(pts[8]), toGlobal(pts[6]), fli));

  canv.add(cloth({pts[1], pts[7], pts[8]}, fsleeve));
  canv.add(cloth({pts[1], pts[0], pts[3], pts[4]}, fbody));
  canv.add(cloth({pts[1], pts[5], pts[6]}, fsleeve));
  canv.add(cloth({pts[1], pts[2]}, fhead));

  Pts hlist = bezmh({pts[1], pts[2]}, 2);
  Pts hlist1, hlist2;
  expand(hlist, fhead, &hlist1, &hlist2);
  hlist1.erase(hlist1.begin(), hlist1.begin() + (size_t)floor(hlist1.size() * 0.1));
  hlist2.erase(hlist2.begin(), hlist2.begin() + (size_t)floor(hlist2.size() * 0.95));
  PolyArgs pa;
  pa.fil = "rgba(100,100,100,0.6)";
  canv.add(poly(mapG(concat(hlist1, reversed(hlist2))), pa));

  if (args.hat) canv.add(args.hat(toGlobal(pts[1]), toGlobal(pts[2]), fli));
  else canv.add(hat01(toGlobal(pts[1]), toGlobal(pts[2]), fli));
  return canv;
}

// ================================================================ Arch

Canvas arch01(real xoff, real yoff, real seed, const ArchArgs& args) {
  real hei = args.hei.value_or(70);
  real wid = args.wid.value_or(180);
  real per = args.per.value_or(5);

  real p = 0.4 + rnd() * 0.2;
  real h0 = hei * p;
  real h1 = hei * (1 - p);

  Canvas canv;
  canv.add(hut(xoff, yoff - hei, h0, wid));
  BoxArgs ba;
  ba.hei = h1;
  ba.wid = (wid * 2) / 3;
  ba.per = per;
  ba.bot = false;
  canv.add(box(xoff, yoff, ba));

  RailArgs ra;
  ra.tra = true;
  ra.fro = false;
  ra.hei = 10;
  ra.wid = wid;
  ra.per = per * 2;
  ra.seg = toInt32(3 + rnd() * 3);
  canv.add(rail(xoff, yoff, seed, ra));

  int mcnt = randChoice(std::vector<int>{0, 1, 1, 2});
  if (mcnt == 1) {
    real x = xoff + normRand(-wid / 3, wid / 3);
    ManArgs ma;
    ma.fli = randChoice(std::vector<bool>{true, false});
    ma.sca = 0.42;
    canv.add(man(x, yoff, ma));
  } else if (mcnt == 2) {
    ManArgs ma;
    ma.sca = 0.42;
    ma.fli = false;
    canv.add(man(xoff + normRand(-wid / 4, -wid / 5), yoff, ma));
    ma.fli = true;
    canv.add(man(xoff + normRand(wid / 5, wid / 4), yoff, ma));
  }
  RailArgs rb;
  rb.tra = false;
  rb.fro = true;
  rb.hei = 10;
  rb.wid = wid;
  rb.per = per * 2;
  rb.seg = toInt32(3 + rnd() * 3);
  canv.add(rail(xoff, yoff, seed, rb));
  return canv;
}

Canvas arch02(real xoff, real yoff, real seed, const ArchArgs& args) {
  real hei = args.hei.value_or(10);
  real wid = args.wid.value_or(50);
  real rot = args.rot.value_or(0.3);
  real per = args.per.value_or(5);
  real sto = args.sto.value_or(3);
  real sty = args.sty.value_or(1);
  bool rai = args.rai.value_or(false);
  Canvas canv;

  real hoff = 0;
  for (int i = 0; i < sto; i++) {
    BoxArgs ba;
    ba.tra = false;
    ba.hei = hei;
    ba.wid = wid * pow(0.85, i);
    ba.rot = rot;
    ba.wei = 1.5;
    ba.per = per;
    ba.dec = [sty](const DecoArgs& a) {
      // hsp/vsp come from [[], [1,5], [1,5], [1,4]][sty]; any other sty draws
      // nothing since deco() only knows styles 1-3.
      DecoArgs d = a;
      int s = (int)sty;
      if (s == 1 || s == 2) {
        d.hsp[0] = 1, d.hsp[1] = 5;
        d.vsp[0] = 1, d.vsp[1] = 2;
      } else if (s == 3) {
        d.hsp[0] = 1, d.hsp[1] = 4;
        d.vsp[0] = 1, d.vsp[1] = 3;
      }
      return deco(sty, d);
    };
    canv.add(box(xoff, yoff - hoff, ba));
    if (rai) {
      RailArgs ra;
      ra.wid = wid * pow(0.85, i) * 1.1;
      ra.hei = hei / 2;
      ra.per = per;
      ra.rot = rot;
      ra.wei = 0.5;
      ra.tra = false;
      canv.add(rail(xoff, yoff - hoff, i * 0.2, ra));
    }
    bool pla = false;
    if (sto == 1 && rnd() < 1.0 / 3) pla = true;
    RoofArgs fa;
    fa.hei = hei;
    fa.wid = wid * pow(0.9, i);
    fa.rot = rot;
    fa.wei = 1.5;
    fa.per = per;
    fa.pla = pla;
    canv.add(roof(xoff, yoff - hoff - hei, fa));
    hoff += hei * 1.5;
  }
  return canv;
}

Canvas arch03(real xoff, real yoff, real seed, const ArchArgs& args) {
  real hei = args.hei.value_or(10);
  real wid = args.wid.value_or(50);
  real rot = args.rot.value_or(0.7);
  real per = args.per.value_or(5);
  real sto = args.sto.value_or(7);
  Canvas canv;

  real hoff = 0;
  for (int i = 0; i < sto; i++) {
    BoxArgs ba;
    ba.tra = false;
    ba.hei = hei;
    ba.wid = wid * pow(0.85, i);
    ba.rot = rot;
    ba.wei = 1.5;
    ba.per = per / 2;
    ba.dec = [](const DecoArgs& a) {
      DecoArgs d = a;
      d.hsp[0] = 1, d.hsp[1] = 4;
      d.vsp[0] = 1, d.vsp[1] = 2;
      return deco(1, d);
    };
    canv.add(box(xoff, yoff - hoff, ba));
    RailArgs ra;
    ra.seg = 5;
    ra.wid = wid * pow(0.85, i) * 1.1;
    ra.hei = hei / 2;
    ra.per = per / 2;
    ra.rot = rot;
    ra.wei = 0.5;
    ra.tra = false;
    canv.add(rail(xoff, yoff - hoff, i * 0.2, ra));
    PagroofArgs pa;
    pa.hei = hei * 1.5;
    pa.wid = wid * pow(0.9, i);
    pa.rot = rot;
    pa.wei = 1.5;
    pa.per = per;
    canv.add(pagroof(xoff, yoff - hoff - hei, pa));
    hoff += hei * 1.5;
  }
  return canv;
}

Canvas arch04(real xoff, real yoff, real seed, const ArchArgs& args) {
  real hei = args.hei.value_or(15);
  real wid = args.wid.value_or(30);
  real rot = args.rot.value_or(0.7);
  real per = args.per.value_or(5);
  real sto = args.sto.value_or(2);
  Canvas canv;

  real hoff = 0;
  for (int i = 0; i < sto; i++) {
    BoxArgs ba;
    ba.tra = true;
    ba.hei = hei;
    ba.wid = wid * pow(0.85, i);
    ba.rot = rot;
    ba.wei = 1.5;
    ba.per = per / 2;
    ba.dec = [](const DecoArgs&) { return std::vector<Pts>(); };
    canv.add(box(xoff, yoff - hoff, ba));
    RailArgs ra;
    ra.seg = 3;
    ra.wid = wid * pow(0.85, i) * 1.2;
    ra.hei = hei / 3;
    ra.per = per / 2;
    ra.rot = rot;
    ra.wei = 0.5;
    ra.tra = true;
    canv.add(rail(xoff, yoff - hoff, i * 0.2, ra));
    PagroofArgs pa;
    pa.hei = hei * 1;
    pa.wid = wid * pow(0.9, i);
    pa.rot = rot;
    pa.wei = 1.5;
    pa.per = per;
    canv.add(pagroof(xoff, yoff - hoff - hei, pa));
    hoff += hei * 1.2;
  }
  return canv;
}

Canvas boat01(real xoff, real yoff, real seed, const ArchArgs& args) {
  real len = args.len.value_or(120);
  real sca = args.sca.value_or(1);
  bool fli = args.fli.value_or(false);
  Canvas canv;

  real dir = fli ? -1 : 1;
  ManArgs ma;
  ma.ite = stick01;
  ma.hat = hat02;
  ma.sca = 0.5 * sca;
  ma.fli = !fli;
  ma.len = {0, 30, 20, 30, 10, 30, 30, 30, 30};
  canv.add(man(xoff + 20 * sca * dir, yoff, ma));

  Pts plist1, plist2;
  auto fun1 = [sca](real x) { return pow(sin(x * PI), 0.5) * 7 * sca; };
  auto fun2 = [sca](real x) { return pow(sin(x * PI), 0.5) * 10 * sca; };
  for (real i = 0; i < len * sca; i += 5 * sca) {
    plist1.push_back(Pt(i * dir, fun1(i / len)));
    plist2.push_back(Pt(i * dir, fun2(i / len)));
  }
  Pts plist = concat(plist1, reversed(plist2));
  PolyArgs pa;
  pa.xof = xoff;
  pa.yof = yoff;
  pa.fil = "white";
  canv.add(poly(plist, pa));
  Pts g;
  for (auto& v : plist) g.push_back(Pt(xoff + v.x, yoff + v.y));
  StrokeArgs sa;
  sa.wid = 1;
  sa.fun = [](real x) { return sin(x * PI * 2); };
  sa.col = kGray4;
  canv.add(stroke(g, sa));
  return canv;
}

Canvas transmissionTower01(real xoff, real yoff, real seed, const ArchArgs& args) {
  real hei = args.hei.value_or(100);
  real wid = args.wid.value_or(20);
  Canvas canv;

  auto quickstroke = [&](const Pts& pl) {
    StrokeArgs sa;
    sa.wid = 1;
    sa.fun = [](real) { return (real)0.5; };
    sa.col = kGray4;
    return stroke(offset(div(pl, 5), xoff, yoff), sa);
  };

  Pt p00(-wid * 0.05, -hei), p01(wid * 0.05, -hei);
  Pt p10(-wid * 0.1, -hei * 0.9), p11(wid * 0.1, -hei * 0.9);
  Pt p20(-wid * 0.2, -hei * 0.5), p21(wid * 0.2, -hei * 0.5);
  Pt p30(-wid * 0.5, 0), p31(wid * 0.5, 0);

  const real bch[3][2] = {{0.7, -0.85}, {1, -0.675}, {0.7, -0.5}};
  for (int i = 0; i < 3; i++) {
    canv.add(quickstroke({{-bch[i][0] * wid, bch[i][1] * hei}, {bch[i][0] * wid, bch[i][1] * hei}}));
    canv.add(quickstroke({{-bch[i][0] * wid, bch[i][1] * hei}, {0, (bch[i][1] - 0.05) * hei}}));
    canv.add(quickstroke({{bch[i][0] * wid, bch[i][1] * hei}, {0, (bch[i][1] - 0.05) * hei}}));
    canv.add(quickstroke({{-bch[i][0] * wid, bch[i][1] * hei}, {-bch[i][0] * wid, (bch[i][1] + 0.1) * hei}}));
    canv.add(quickstroke({{bch[i][0] * wid, bch[i][1] * hei}, {bch[i][0] * wid, (bch[i][1] + 0.1) * hei}}));
  }

  Pts l10 = div({p00, p10, p20, p30}, 5);
  Pts l11 = div({p01, p11, p21, p31}, 5);
  for (size_t i = 0; i + 1 < l10.size(); i++) {
    canv.add(quickstroke({l10[i], l11[i + 1]}));
    canv.add(quickstroke({l11[i], l10[i + 1]}));
  }

  canv.add(quickstroke({p00, p01}));
  canv.add(quickstroke({p10, p11}));
  canv.add(quickstroke({p20, p21}));
  canv.add(quickstroke({p00, p10, p20, p30}));
  canv.add(quickstroke({p01, p11, p21, p31}));
  return canv;
}

}  // namespace ss
