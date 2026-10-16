#include "tree.h"

#include <stdlib.h>

namespace ss {

namespace {

// ---------------------------------------------------------------- helpers

// parseFloat: leading number, NaN if none.
real parseFloat(const std::string& s) {
  const char* b = s.c_str();
  char* e = nullptr;
  real v = strtod(b, &e);
  return e == b ? NAN : v;
}

// col.replace("rgba(", "").replace(")", "").split(","), or the fallback.
// Missing entries read as JavaScript undefined.
std::vector<std::string> leafColor(const std::string& col, const char* alpha) {
  if (col.find("rgba(") == std::string::npos) return {"100", "100", "100", alpha};
  std::string s = col;
  s.erase(s.find("rgba("), 5);
  size_t r = s.find(")");
  if (r != std::string::npos) s.erase(r, 1);
  std::vector<std::string> out;
  size_t st = 0;
  while (true) {
    size_t c = s.find(",", st);
    if (c == std::string::npos) {
      out.push_back(s.substr(st));
      break;
    }
    out.push_back(s.substr(st, c - st));
    st = c + 1;
  }
  while (out.size() < 4) out.push_back("undefined");
  return out;
}

std::string leafRgba(const std::vector<std::string>& lc, const std::string& a) {
  return "rgba(" + lc[0] + "," + lc[1] + "," + lc[2] + "," + a + ")";
}

real pm1() { return randChoice(std::vector<real>{-1, 1}); }

Pts concatRev(const Pts& a, const Pts& b) {
  Pts r = a;
  for (size_t i = b.size(); i-- > 0;) r.push_back(b[i]);
  return r;
}

Pts offset(const Pts& l, real x, real y) {
  Pts r;
  r.reserve(l.size());
  for (auto& v : l) r.push_back(Pt(v.x + x, v.y + y));
  return r;
}

real leafFun(real x) {
  return x <= 1 ? pow(sin(x * PI) * x, 0.5) : -pow(sin((x - 2) * PI * (x - 2)), 0.5);
}

// ---------------------------------------------------------------- branch
struct BranchArgs {
  real hei = 300;
  real wid = 6;
  real ang = 0;
  real det = 10;
  real ben = PI * 0.2;
};

std::vector<Pts> branch(const BranchArgs& args) {
  real hei = args.hei, wid = args.wid, ben = args.ben;
  real nx = 0;
  real ny = 0;
  Pts tlist = {Pt(nx, ny)};
  real a0 = 0;
  int g = 3;
  for (int i = 0; i < g; i++) {
    real r = (ben / 2 + (rnd() * ben) / 2);
    a0 += r * pm1();
    nx += (cos(a0) * hei) / g;
    ny -= (sin(a0) * hei) / g;
    tlist.push_back(Pt(nx, ny));
  }
  real ta = atan2(tlist[tlist.size() - 1].y, tlist[tlist.size() - 1].x);

  for (size_t i = 0; i < tlist.size(); i++) {
    real a = atan2(tlist[i].y, tlist[i].x);
    real d = sqrt(tlist[i].x * tlist[i].x + tlist[i].y * tlist[i].y);
    tlist[i].x = d * cos(a - ta + args.ang);
    tlist[i].y = d * sin(a - ta + args.ang);
  }

  Pts trlist1, trlist2;
  real span = args.det;
  real tl = (tlist.size() - 1.0) * span;
  real lx = 0;
  real ly = 0;

  for (real i = 0; i < tl; i += 1) {
    const Pt& lastp = tlist[(size_t)floor(i / span)];
    const Pt& nextp = tlist[(size_t)ceil(i / span)];
    real p = fmod(i, span) / span;
    real nx = lastp.x * (1 - p) + nextp.x * p;
    real ny = lastp.y * (1 - p) + nextp.y * p;

    real ang = atan2(ny - ly, nx - lx);  // shadows args.ang (JS var)
    real woff = ((noise(i * 0.3) - 0.5) * wid * hei) / 80;

    real b = 0;
    if (p == 0) b = rnd() * wid;

    real nw = wid * (((tl - i) / tl) * 0.5 + 0.5);
    trlist1.push_back(Pt(nx + cos(ang + PI / 2) * (nw + woff + b),
                         ny + sin(ang + PI / 2) * (nw + woff + b)));
    trlist2.push_back(Pt(nx + cos(ang - PI / 2) * (nw - woff + b),
                         ny + sin(ang - PI / 2) * (nw - woff + b)));
    lx = nx;
    ly = ny;
  }
  return {trlist1, trlist2};
}

// ---------------------------------------------------------------- twig
struct TwigArgs {
  real dir = 1;
  real sca = 1;
  real wid = 1;
  real ang = 0;
  bool lea0 = true;
  real lea1 = 12;
};

Canvas twig(real tx, real ty, int dep, const TwigArgs& args) {
  real dir = args.dir, sca = args.sca, wid = args.wid, ang = args.ang;
  Canvas canv;
  Pts twlist;
  int tl = 10;
  real hs = rnd() * 0.5 + 0.5;
  rnd();  // randChoice([fun2])
  real a0 = ((rnd() * PI) / 6) * dir + ang;
  for (int i = 0; i < tl; i++) {
    // fun2 reads twig's own loop variable i, which equals its argument * tl.
    real tf = -1 / pow((real)i / tl + 1, 5) + 1;
    real mx = dir * tf * 50 * sca * hs;
    real my = -(real)i * 5 * sca;

    real a = atan2(my, mx);
    real d = pow(mx * mx + my * my, 0.5);

    real nx = cos(a + a0) * d;
    real ny = sin(a + a0) * d;

    twlist.push_back(Pt(nx + tx, ny + ty));
    if ((i == tl / 3 || i == (tl * 2) / 3) && dep > 0) {
      TwigArgs ta;
      ta.ang = ang;
      ta.sca = sca * 0.8;
      ta.wid = wid;
      ta.dir = dir * pm1();
      ta.lea0 = args.lea0;
      ta.lea1 = args.lea1;
      canv.add(twig(nx + tx, ny + ty, dep - 1, ta));
    }
    if (i == tl - 1 && args.lea0) {
      for (int j = 0; j < 5; j++) {
        real dj = (j - 2.5) * 5;
        BlobArgs ba;
        ba.wid = (6 + 3 * rnd()) * wid;
        ba.len = (15 + 12 * rnd()) * wid;
        ba.ang = ang / 2 + PI / 2 + PI * 0.2 * (rnd() - 0.5);
        ba.col = "rgba(100,100,100," + fixed(0.5 + dep * 0.2, 3) + ")";
        ba.fun = leafFun;
        canv.add(blob(nx + tx + cos(ang) * dj * wid,
                      ny + ty + (sin(ang) * dj - args.lea1 / (dep + 1)) * wid, ba));
      }
    }
  }
  StrokeArgs sa;
  sa.wid = 1;
  sa.fun = [](real x) { return cos((x * PI) / 2); };
  sa.col = "rgba(100,100,100,0.5)";
  canv.add(stroke(twlist, sa));
  return canv;
}

// ---------------------------------------------------------------- barkify
Canvas bark(real x, real y, real wid, real ang) {
  real len = 10 + 10 * rnd();
  real noi = 0.5;
  real reso = 20.0;
  Canvas canv;

  Pts lalist;
  for (int i = 0; i < reso + 1; i++) {
    real p = (i / reso) * 2;
    real xo = len / 2 - fabs(p - 1) * len;
    real f = p <= 1 ? pow(sin(p * PI), 0.5) : -pow(sin((p + 1) * PI), 0.5);
    real yo = (f * wid) / 2;
    real a = atan2(yo, xo);
    real l = sqrt(xo * xo + yo * yo);
    lalist.push_back(Pt(l, a));
  }
  std::vector<real> nslist;
  real n0 = rnd() * 10;
  for (int i = 0; i < reso + 1; i++) nslist.push_back(noise(i * 0.05, n0));

  loopNoise(nslist);
  Pts brklist;
  for (size_t i = 0; i < lalist.size(); i++) {
    real ns = nslist[i] * noi + (1 - noi);
    real nx = x + cos(lalist[i].y + ang) * lalist[i].x * ns;
    real ny = y + sin(lalist[i].y + ang) * lalist[i].x * ns;
    brklist.push_back(Pt(nx, ny));
  }
  real fr = rnd();
  StrokeArgs sa;
  sa.wid = 0.8;
  sa.noi = 0;
  sa.col = "rgba(100,100,100,0.4)";
  sa.out = 0;
  sa.fun = [fr](real x) { return sin((x + fr) * PI * 3); };
  canv.add(stroke(brklist, sa));
  return canv;
}

// Mutates trlist like the JavaScript: div() returns the last input point
// itself, so the noise added to it lands in the caller's branch outline.
Canvas barkify(real x, real y, std::vector<Pts>& trlist) {
  Canvas canv;
  Pts& t0 = trlist[0];
  Pts& t1 = trlist[1];

  for (int i = 2; i < (int)t0.size() - 1; i++) {
    real a0 = atan2(t0[i].y - t0[i - 1].y, t0[i].x - t0[i - 1].x);
    real a1 = atan2(t1[i].y - t1[i - 1].y, t1[i].x - t1[i - 1].x);
    real p = rnd();
    real nx = t0[i].x * (1 - p) + t1[i].x * p;
    real ny = t0[i].y * (1 - p) + t1[i].y * p;
    if (rnd() < 0.2) {
      BlobArgs ba;
      ba.noi = 1;
      ba.len = 15;
      ba.wid = 6 - fabs(p - 0.5) * 10;
      ba.ang = (a0 + a1) / 2;
      ba.col = "rgba(100,100,100,0.6)";
      canv.add(blob(nx + x, ny + y, ba));
    } else {
      canv.add(bark(nx + x, ny + y, 5 - fabs(p - 0.5) * 10, (a0 + a1) / 2));
    }

    if (rnd() < 0.05) {
      real jl = rnd() * 2 + 2;
      real xya[2][3] = {{t0[i].x, t0[i].y, a0}, {t1[i].x, t1[i].y, a1}};
      const real* c = xya[(size_t)floor(2 * rnd())];
      for (int j = 0; j < jl; j++) {
        BlobArgs ba;
        ba.wid = 4;
        ba.len = 4 + 6 * rnd();
        ba.ang = a0 + PI / 2;
        ba.col = "rgba(100,100,100,0.6)";
        canv.add(blob(c[0] + x + cos(c[2]) * (j - jl / 2) * 4,
                      c[1] + y + sin(c[2]) * (j - jl / 2) * 4, ba));
      }
    }
  }
  // trflist = trlist[0] ++ reverse(trlist[1]), as pointers to the originals.
  std::vector<Pt*> trflist;
  for (auto& v : t0) trflist.push_back(&v);
  for (size_t k = t1.size(); k-- > 0;) trflist.push_back(&t1[k]);
  std::vector<std::vector<Pt*>> rglist(1);
  for (size_t i = 0; i < trflist.size(); i++) {
    if (rnd() < 0.5) rglist.push_back({});
    else rglist.back().push_back(trflist[i]);
  }

  for (size_t i = 0; i < rglist.size(); i++) {
    Pts g;
    for (Pt* q : rglist[i]) g.push_back(*q);
    Pts d = div(g, 4);
    for (size_t j = 0; j < d.size(); j++) {
      real gx = randGaussian();
      d[j].x += (noise(i, j * 0.1, 1) - 0.5) * (15 + 5 * gx);
      real gy = randGaussian();
      d[j].y += (noise(i, j * 0.1, 2) - 0.5) * (15 + 5 * gy);
    }
    if (!rglist[i].empty()) *rglist[i].back() = d.back();
    StrokeArgs sa;
    sa.wid = 1.5;
    sa.col = "rgba(100,100,100,0.7)";
    sa.out = 0;
    canv.add(stroke(offset(d, x, y), sa));
  }
  return canv;
}

// The trunk outline and its shaded edge, shared by tree04/05/06.
void trunk(Canvas& canv, Pts trmlist, real x, real y, const std::string& col, real a0) {
  PolyArgs pa;
  pa.xof = x;
  pa.yof = y;
  pa.fil = "white";
  pa.str = col;
  pa.wid = 0;
  canv.add(poly(trmlist, pa));

  if (!trmlist.empty()) trmlist.erase(trmlist.begin());
  if (!trmlist.empty()) trmlist.pop_back();
  StrokeArgs sa;
  sa.col = "rgba(100,100,100," + fixed(a0 + rnd() * 0.1, 3) + ")";
  sa.wid = 2.5;
  sa.fun = [](real) { return sin(1); };
  sa.noi = 0.9;
  sa.out = 0;
  canv.add(stroke(offset(trmlist, x, y), sa));
}

}  // namespace

// ================================================================ trees

Canvas tree01(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(50);
  real wid = args.wid.value_or(3);
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  int reso = 10;
  std::vector<Pt> nslist;
  for (int i = 0; i < reso; i++) nslist.push_back(Pt(noise(i * 0.5), noise(i * 0.5, 0.5)));

  std::vector<std::string> leafcol = leafColor(col, "0.5");
  Canvas canv;
  Pts line1, line2;
  for (int i = 0; i < reso; i++) {
    real nx = x;
    real ny = y - ((real)i * hei) / reso;
    if (i >= reso / 4.0) {
      for (int j = 0; j < (reso - i) / 5.0; j++) {
        real bx = nx + (rnd() - 0.5) * wid * 1.2 * (reso - i);
        real by = ny + (rnd() - 0.5) * wid;
        BlobArgs ba;
        ba.len = rnd() * 20 * (reso - i) * 0.2 + 10;
        ba.wid = rnd() * 6 + 3;
        ba.ang = ((rnd() - 0.5) * PI) / 6;
        ba.col = leafRgba(leafcol, fixed(rnd() * 0.2 + parseFloat(leafcol[3]), 1));
        canv.add(blob(bx, by, ba));
      }
    }
    line1.push_back(Pt(nx + (nslist[i].x - 0.5) * wid - wid / 2, ny));
    line2.push_back(Pt(nx + (nslist[i].y - 0.5) * wid + wid / 2, ny));
  }
  PolyArgs pa;
  pa.fil = "none";
  pa.str = col;
  pa.wid = 1.5;
  canv.add(poly(line1, pa));
  canv.add(poly(line2, pa));
  return canv;
}

Canvas tree02(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(16);
  real wid = args.wid.value_or(8);
  real clu = args.clu.value_or(5);
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  Canvas canv;
  for (int i = 0; i < clu; i++) {
    real bx = x + randGaussian() * clu * 4;
    real by = y + randGaussian() * clu * 4;
    BlobArgs ba;
    ba.ang = PI / 2;
    ba.fun = leafFun;
    ba.wid = rnd() * wid * 0.75 + wid * 0.5;
    ba.len = rnd() * hei * 0.75 + hei * 0.5;
    ba.col = col;
    canv.add(blob(bx, by, ba));
  }
  return canv;
}

Canvas tree03(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(50);
  real wid = args.wid.value_or(5);
  Fn ben = args.ben ? *args.ben : Fn([](real) { return 0.0; });
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  int reso = 10;
  std::vector<Pt> nslist;
  for (int i = 0; i < reso; i++) nslist.push_back(Pt(noise(i * 0.5), noise(i * 0.5, 0.5)));

  std::vector<std::string> leafcol = leafColor(col, "0.5");
  Canvas canv, blobs;
  Pts line1, line2;
  for (int i = 0; i < reso; i++) {
    real nx = x + ben((real)i / reso) * 100;
    real ny = y - ((real)i * hei) / reso;
    if (i >= reso / 5.0) {
      for (int j = 0; j < (reso - i) * 2; j++) {
        real sx = (real)(reso - i) / reso;
        real shape = log(50 * sx + 1) / 3.95;
        real ox = rnd() * wid * 2 * shape;
        real bx = nx + ox * pm1();
        real by = ny + (rnd() - 0.5) * wid * 2;
        BlobArgs ba;
        ba.len = ox * 2;
        ba.wid = rnd() * 6 + 3;
        ba.ang = ((rnd() - 0.5) * PI) / 6;
        ba.col = leafRgba(leafcol, fixed(rnd() * 0.2 + parseFloat(leafcol[3]), 3));
        blobs.add(blob(bx, by, ba));
      }
    }
    line1.push_back(Pt(nx + (((nslist[i].x - 0.5) * wid - wid / 2) * (reso - i)) / reso, ny));
    line2.push_back(Pt(nx + (((nslist[i].y - 0.5) * wid + wid / 2) * (reso - i)) / reso, ny));
  }
  PolyArgs pa;
  pa.fil = "white";
  pa.str = col;
  pa.wid = 1.5;
  canv.add(poly(concatRev(line1, line2), pa));
  canv.add(std::move(blobs));
  return canv;
}

Canvas tree04(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(300);
  real wid = args.wid.value_or(6);
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  Canvas canv, txcanv, twcanv;

  BranchArgs b0;
  b0.hei = hei;
  b0.wid = wid;
  b0.ang = -PI / 2;
  std::vector<Pts> tr = branch(b0);
  txcanv.add(barkify(x, y, tr));
  Pts trlist = concatRev(tr[0], tr[1]);
  real L = (real)trlist.size();

  Pts trmlist;
  for (size_t i = 0; i < trlist.size(); i++) {
    bool hit = i >= L * 0.3 && i <= L * 0.7 && rnd() < 0.1;
    if (hit || i == L / 2 - 1) {
      real ba = PI * 0.2 - PI * 1.4 * (i > L / 2 ? 1 : 0);
      BranchArgs bb;
      bb.hei = hei * (rnd() + 1) * 0.3;
      bb.wid = wid * 0.5;
      bb.ang = ba;
      std::vector<Pts> brlist = branch(bb);

      brlist[0].erase(brlist[0].begin());
      brlist[1].erase(brlist[1].begin());
      Pt o = trlist[i];
      std::vector<Pts> moved = {offset(brlist[0], o.x, o.y), offset(brlist[1], o.x, o.y)};
      txcanv.add(barkify(x, y, moved));

      for (size_t j = 0; j < brlist[0].size(); j++) {
        if (rnd() < 0.2 || j == brlist[0].size() - 1) {
          TwigArgs ta;
          ta.wid = hei / 300;
          ta.ang = ba > -PI / 2 ? ba : ba + PI;
          ta.sca = (0.5 * hei) / 300;
          ta.dir = ba > -PI / 2 ? 1 : -1;
          twcanv.add(twig(brlist[0][j].x + o.x + x, brlist[0][j].y + o.y + y, 1, ta));
        }
      }
      for (auto& v : offset(concatRev(brlist[0], brlist[1]), o.x, o.y)) trmlist.push_back(v);
    } else {
      trmlist.push_back(trlist[i]);
    }
  }
  trunk(canv, trmlist, x, y, col, 0.4);
  canv.add(std::move(txcanv));
  canv.add(std::move(twcanv));
  return canv;
}

Canvas tree05(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(300);
  real wid = args.wid.value_or(5);
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  Canvas canv, txcanv, twcanv;

  BranchArgs b0;
  b0.hei = hei;
  b0.wid = wid;
  b0.ang = -PI / 2;
  b0.ben = 0;
  std::vector<Pts> tr = branch(b0);
  txcanv.add(barkify(x, y, tr));
  Pts trlist = concatRev(tr[0], tr[1]);
  real L = (real)trlist.size();

  Pts trmlist;
  for (size_t i = 0; i < trlist.size(); i++) {
    real p = fabs(i - L * 0.5) / (L * 0.5);
    bool hit = i >= L * 0.2 && i <= L * 0.8 && i % 3 == 0 && rnd() > p;
    if (hit || i == L / 2 - 1) {
      real bar = rnd() * 0.2;
      real ba = -bar * PI - (1 - bar * 2) * PI * (i > L / 2 ? 1 : 0);
      BranchArgs bb;
      bb.hei = hei * (0.3 * p - rnd() * 0.05);
      bb.wid = wid * 0.5;
      bb.ang = ba;
      bb.ben = 0.5;
      std::vector<Pts> brlist = branch(bb);

      brlist[0].erase(brlist[0].begin());
      brlist[1].erase(brlist[1].begin());
      Pt o = trlist[i];

      for (size_t j = 0; j < brlist[0].size(); j++) {
        if (j % 20 == 0 || j == brlist[0].size() - 1) {
          TwigArgs ta;
          ta.wid = hei / 300;
          ta.ang = ba > -PI / 2 ? ba : ba + PI;
          ta.sca = (0.2 * hei) / 300;
          ta.dir = ba > -PI / 2 ? 1 : -1;
          ta.lea0 = true;
          ta.lea1 = 5;
          twcanv.add(twig(brlist[0][j].x + o.x + x, brlist[0][j].y + o.y + y, 0, ta));
        }
      }
      for (auto& v : offset(concatRev(brlist[0], brlist[1]), o.x, o.y)) trmlist.push_back(v);
    } else {
      trmlist.push_back(trlist[i]);
    }
  }
  trunk(canv, trmlist, x, y, col, 0.4);
  canv.add(std::move(txcanv));
  canv.add(std::move(twcanv));
  return canv;
}

namespace {

struct FracArgs06 {
  real hei = 300;
  real wid = 5;
  real ang = 0;
  real ben = PI * 0.2;
};

Pts fracTree06(Canvas& txcanv, Canvas& twcanv, real xoff, real yoff, int dep,
               const FracArgs06& args) {
  real hei = args.hei, wid = args.wid, ang = args.ang;
  BranchArgs b0;
  b0.hei = hei;
  b0.wid = wid;
  b0.ang = ang;
  b0.ben = args.ben;
  b0.det = hei / 20;
  std::vector<Pts> tr = branch(b0);
  txcanv.add(barkify(xoff, yoff, tr));
  Pts trlist = concatRev(tr[0], tr[1]);
  real L = (real)trlist.size();
  int half = toInt32(L / 2);

  Pts trmlist;
  for (int i = 0; i < (int)trlist.size(); i++) {
    bool hit = rnd() < 0.025 && i >= L * 0.2 && i <= L * 0.8;
    if ((hit || i == half - 1 || i == half + 1) && dep > 0) {
      real bar = 0.02 + rnd() * 0.08;
      real ba = bar * PI - bar * 2 * PI * (i > L / 2 ? 1 : 0);

      FracArgs06 fa;
      fa.hei = hei * (0.7 + rnd() * 0.2);
      fa.wid = wid * 0.6;
      fa.ang = ang + ba;
      fa.ben = 0.55;
      Pt o = trlist[i];
      Pts brlist = fracTree06(txcanv, twcanv, o.x + xoff, o.y + yoff, dep - 1, fa);

      for (size_t j = 0; j < brlist.size(); j++) {
        if (rnd() < 0.03) {
          TwigArgs ta;
          ta.ang = ba * (rnd() * 0.5 + 0.75);
          ta.sca = 0.3;
          ta.dir = ba > 0 ? 1 : -1;
          ta.lea0 = false;
          ta.lea1 = 0;
          twcanv.add(twig(brlist[j].x + o.x + xoff, brlist[j].y + o.y + yoff, 2, ta));
        }
      }
      for (auto& v : brlist) trmlist.push_back(Pt(v.x + o.x, v.y + o.y));
    } else {
      trmlist.push_back(trlist[i]);
    }
  }
  return trmlist;
}

}  // namespace

Canvas tree06(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(100);
  real wid = args.wid.value_or(6);
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  Canvas canv, txcanv, twcanv;
  FracArgs06 fa;
  fa.hei = hei;
  fa.wid = wid;
  fa.ang = -PI / 2;
  fa.ben = 0;
  Pts trmlist = fracTree06(txcanv, twcanv, x, y, 3, fa);
  trunk(canv, trmlist, x, y, col, 0.4);
  canv.add(std::move(txcanv));
  canv.add(std::move(twcanv));
  return canv;
}

Canvas tree07(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(60);
  real wid = args.wid.value_or(4);
  Fn ben = args.ben ? *args.ben : Fn([](real x) { return sqrt(x) * 0.2; });
  std::string col = args.col.value_or("rgba(100,100,100,1)");

  int reso = 10;
  std::vector<Pt> nslist;
  for (int i = 0; i < reso; i++) nslist.push_back(Pt(noise(i * 0.5), noise(i * 0.5, 0.5)));
  std::vector<std::string> leafcol = leafColor(col, "1");
  Canvas canv;
  Pts line1, line2;
  std::vector<Pts> T;
  for (int i = 0; i < reso; i++) {
    real nx = x + ben((real)i / reso) * 100;
    real ny = y - ((real)i * hei) / reso;
    if (i >= reso / 4.0) {
      real bx = nx + (rnd() - 0.5) * wid * 1.2 * (reso - i) * 0.5;
      real by = ny + (rnd() - 0.5) * wid * 0.5;
      BlobArgs ba;
      ba.len = rnd() * 50 + 20;
      ba.wid = rnd() * 12 + 12;
      ba.ang = (-rnd() * PI) / 6;
      ba.col = leafRgba(leafcol, fixed(parseFloat(leafcol[3]), 3));
      ba.fun = [](real x) {
        return x <= 1 ? 2.75 * x * pow(1 - x, 1 / 1.8) : 2.75 * (x - 2) * pow(x - 1, 1 / 1.8);
      };
      Pts bpl = blobPts(bx, by, ba);
      TriArgs t;
      t.area = 50;
      t.convex = true;
      t.optimize = false;
      for (auto& tri : triangulate(bpl, t)) T.push_back(tri);
    }
    line1.push_back(Pt(nx + (nslist[i].x - 0.5) * wid - wid / 2, ny));
    line2.push_back(Pt(nx + (nslist[i].y - 0.5) * wid + wid / 2, ny));
  }

  TriArgs t;
  t.area = 50;
  t.convex = true;
  t.optimize = true;
  std::vector<Pts> T2 = triangulate(concatRev(line1, line2), t);
  for (auto& tri : T) T2.push_back(tri);

  for (auto& tri : T2) {
    Pt m = midPt(tri);
    int32_t c = toInt32(noise(m.x * 0.02, m.y * 0.02) * 200 + 50);
    std::string co = "rgba(" + jsnum(c) + "," + jsnum(c) + "," + jsnum(c) + ",0.8)";
    PolyArgs pa;
    pa.fil = co;
    pa.str = co;
    pa.wid = 0;
    canv.add(poly(tri, pa));
  }
  return canv;
}

namespace {

struct FracArgs08 {
  real ang = -PI / 2;
  real len = 15;
  real ben = 0;
};

Canvas fracTree08(real xoff, real yoff, int dep, const FracArgs08& args) {
  real ang = args.ang, len = args.len, ben = args.ben;
  Pt spt(xoff, yoff);
  Pt ept(xoff + cos(ang) * len, yoff + sin(ang) * len);

  Pts trmlist = {Pt(xoff, yoff), Pt(xoff + len, yoff)};
  real bs = rnd() < 0.5 ? 1 : -1;  // randChoice of +-sin(x*PI)

  trmlist = div(trmlist, 10);

  for (size_t i = 0; i < trmlist.size(); i++)
    trmlist[i].y += bs * sin(((real)i / trmlist.size()) * PI) * 2;
  for (size_t i = 0; i < trmlist.size(); i++) {
    real d = distance(trmlist[i], spt);
    real a = atan2(trmlist[i].y - spt.y, trmlist[i].x - spt.x);
    trmlist[i].x = spt.x + d * cos(a + ang);
    trmlist[i].y = spt.y + d * sin(a + ang);
  }

  Canvas tcanv;
  StrokeArgs sa;
  if (dep == 0) sa.fun = [](real x) { return cos(0.5 * PI * x); };
  else sa.fun = [](real) { return 1.0; };
  sa.wid = 0.8;
  sa.col = "rgba(100,100,100,0.5)";
  tcanv.add(stroke(trmlist, sa));
  if (dep != 0) {
    real nben = ben + pm1() * PI * 0.001 * dep * dep;
    if (rnd() < 0.5) {
      FracArgs08 a1;
      real c0 = normRand(-1, 0.5);
      real c1 = normRand(0.5, 1);
      real c = rnd() < 0.5 ? c0 : c1;
      a1.ang = ang + ben + PI * c * 0.2;
      a1.len = len * normRand(0.8, 0.9);
      a1.ben = nben;
      tcanv.add(fracTree08(ept.x, ept.y, dep - 1, a1));
      FracArgs08 a2;
      c0 = normRand(-1, -0.5);
      c1 = normRand(0.5, 1);
      c = rnd() < 0.5 ? c0 : c1;
      a2.ang = ang + ben + PI * c * 0.2;
      a2.len = len * normRand(0.8, 0.9);
      a2.ben = nben;
      tcanv.add(fracTree08(ept.x, ept.y, dep - 1, a2));
    } else {
      FracArgs08 a1;
      a1.ang = ang + ben;
      a1.len = len * normRand(0.8, 0.9);
      a1.ben = nben;
      tcanv.add(fracTree08(ept.x, ept.y, dep - 1, a1));
    }
  }
  return tcanv;
}

}  // namespace

Canvas tree08(real x, real y, const TreeArgs& args) {
  real hei = args.hei.value_or(80);
  real wid = args.wid.value_or(1);
  std::string col = args.col.value_or("rgba(100,100,100,0.5)");

  Canvas canv, txcanv, twcanv;

  real ang = normRand(-1, 1) * PI * 0.2;

  BranchArgs b0;
  b0.hei = hei;
  b0.wid = wid;
  b0.ang = -PI / 2 + ang;
  b0.ben = PI * 0.2;
  b0.det = hei / 20;
  std::vector<Pts> tr = branch(b0);
  Pts trlist = concatRev(tr[0], tr[1]);

  for (size_t i = 0; i < trlist.size(); i++) {
    if (rnd() < 0.2) {
      int dep = (int)floor(4 * rnd());
      FracArgs08 fa;
      fa.ang = -PI / 2 - ang * rnd();
      twcanv.add(fracTree08(x + trlist[i].x, y + trlist[i].y, dep, fa));
    } else if (i == (size_t)floor(trlist.size() / 2.0)) {
      FracArgs08 fa;
      fa.ang = -PI / 2 + ang;
      twcanv.add(fracTree08(x + trlist[i].x, y + trlist[i].y, 3, fa));
    }
  }

  PolyArgs pa;
  pa.xof = x;
  pa.yof = y;
  pa.fil = "white";
  pa.str = col;
  pa.wid = 0;
  canv.add(poly(trlist, pa));

  StrokeArgs sa;
  sa.col = "rgba(100,100,100," + fixed(0.6 + rnd() * 0.1, 3) + ")";
  sa.wid = 2.5;
  sa.fun = [](real) { return sin(1); };
  sa.noi = 0.9;
  sa.out = 0;
  canv.add(stroke(offset(trlist, x, y), sa));

  canv.add(std::move(txcanv));
  canv.add(std::move(twcanv));
  return canv;
}

}  // namespace ss
