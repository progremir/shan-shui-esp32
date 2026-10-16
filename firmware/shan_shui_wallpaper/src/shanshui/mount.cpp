#include "mount.h"

#include <algorithm>

#include "arch.h"
#include "tree.h"

namespace ss {

namespace {

Pts shifted(const Pts& src, real xof, real yof) {
  Pts out = src;
  for (auto& v : out) {
    v.x += xof;
    v.y += yof;
  }
  return out;
}

std::string grayA(const char* rgb, real a) {
  return std::string("rgba(") + rgb + "," + fixed(a, 3) + ")";
}

// Mount.foot (ret: 0). ptlist rows all have the same length.
Canvas foot(const std::vector<Pts>& ptlist, real xof, real yof) {
  std::vector<Pts> ftlist;
  int span = 10;
  int ni = 0;
  for (int i = 0; i < (int)ptlist.size() - 2; i += 1) {
    if (i == ni) {
      ni = (int)jmin(ni + randChoice<real>({1, 2}), (real)ptlist.size() - 1);

      ftlist.push_back(Pts());
      ftlist.push_back(Pts());
      Pts& f0 = ftlist[ftlist.size() - 2];
      Pts& f1 = ftlist[ftlist.size() - 1];
      const Pts& row = ptlist[i];
      size_t n = row.size();
      for (int j = 0; j < jmin(n / 8.0, 10); j++) {
        f0.push_back(Pt(row[j].x + noise(j * 0.1, i) * 10, row[j].y));
        f1.push_back(Pt(row[n - 1 - j].x - noise(j * 0.1, i) * 10, row[n - 1 - j].y));
      }

      std::reverse(f0.begin(), f0.end());
      std::reverse(f1.begin(), f1.end());
      for (int j = 0; j < span; j++) {
        real p = (real)j / span;
        real x1 = row[0].x * (1 - p) + ptlist[ni][0].x * p;
        real y1 = row[0].y * (1 - p) + ptlist[ni][0].y * p;

        real x2 = row[n - 1].x * (1 - p) + ptlist[ni][n - 1].x * p;
        real y2 = row[n - 1].y * (1 - p) + ptlist[ni][n - 1].y * p;

        real vib = -1.7 * (p - 1) * pow(p, 1.0 / 5);
        y1 += vib * 5 + noise(xof * 0.05, i) * 5;
        y2 += vib * 5 + noise(xof * 0.05, i) * 5;

        f0.push_back(Pt(x1, y1));
        f1.push_back(Pt(x2, y2));
      }
    }
  }
  Canvas canv;
  for (size_t i = 0; i < ftlist.size(); i++) {
    PolyArgs pa;
    pa.xof = xof;
    pa.yof = yof;
    pa.fil = "white";
    pa.str = "none";
    canv.add(poly(ftlist[i], pa));
  }
  for (size_t j = 0; j < ftlist.size(); j++) {
    StrokeArgs sa;
    sa.col = grayA("100,100,100", 0.1 + rnd() * 0.1);
    sa.wid = 1;
    canv.add(stroke(shifted(ftlist[j], xof, yof), sa));
  }
  return canv;
}

Canvas mountainImpl(real xoff, real yoff, real seed, const MountArgs& args,
                    std::vector<Pts>* ret) {
  real hei = args.hei ? *args.hei : 100 + rnd() * 400;
  real wid = args.wid ? *args.wid : 400 + rnd() * 200;
  real tex = args.tex ? *args.tex : 200;
  bool veg = args.veg ? *args.veg : true;

  Canvas canv;

  std::vector<Pts> ptlist;
  real h = hei;
  real w = wid;
  int reso[2] = {10, 50};

  real hoff = 0;
  for (int j = 0; j < reso[0]; j++) {
    hoff += (rnd() * yoff) / 100;
    ptlist.push_back(Pts());
    for (int i = 0; i < reso[1]; i++) {
      real x = ((real)i / reso[1] - 0.5) * PI;
      real y = cos(x);
      y *= noise(x + 10, j * 0.15, seed);
      real p = 1 - (real)j / reso[0];
      ptlist.back().push_back(Pt((x / PI) * w * p, -y * h * p + hoff));
    }
  }

  auto vegetate = [&](const std::function<Canvas(real, real)>& treeFunc,
                      const std::function<bool(int, int)>& growthRule,
                      const std::function<bool(const Pts&, int)>& proofRule) {
    Pts veglist;
    for (int i = 0; i < (int)ptlist.size(); i += 1) {
      for (int j = 0; j < (int)ptlist[i].size(); j += 1) {
        if (growthRule(i, j)) veglist.push_back(ptlist[i][j]);
      }
    }
    for (int i = 0; i < (int)veglist.size(); i++) {
      if (proofRule(veglist, i)) canv.add(treeFunc(veglist[i].x, veglist[i].y));
    }
  };
  auto always = [](const Pts&, int) { return true; };

  // RIM
  vegetate(
      [&](real x, real y) {
        TreeArgs ta;
        ta.col = grayA("100,100,100", noise(0.01 * x, 0.01 * y) * 0.5 * 0.3 + 0.5);
        ta.clu = 2;
        return tree02(x + xoff, y + yoff - 5, ta);
      },
      [&](int i, int j) {
        real ns = noise(j * 0.1, seed);
        return i == 0 && ns * ns * ns < 0.1 && fabs(ptlist[i][j].y) / h > 0.2;
      },
      always);

  // WHITE BG
  {
    Pts bg = ptlist[0];
    bg.push_back(Pt(0, reso[0] * 4));
    PolyArgs pa;
    pa.xof = xoff;
    pa.yof = yoff;
    pa.fil = "white";
    pa.str = "none";
    canv.add(poly(bg, pa));
  }
  // OUTLINE
  {
    StrokeArgs sa;
    sa.col = "rgba(100,100,100,0.3)";
    sa.noi = 1;
    sa.wid = 3;
    canv.add(stroke(shifted(ptlist[0], xoff, yoff), sa));
  }

  canv.add(foot(ptlist, xoff, yoff));
  {
    TextureArgs ta;
    ta.xof = xoff;
    ta.yof = yoff;
    ta.tex = tex;
    ta.sha = randChoice<real>({0, 0, 0, 0, 5});
    ta.col = args.col;
    canv.add(texture(ptlist, ta));
  }

  // TOP
  vegetate(
      [&](real x, real y) {
        TreeArgs ta;
        ta.col = grayA("100,100,100", noise(0.01 * x, 0.01 * y) * 0.5 * 0.3 + 0.5);
        return tree02(x + xoff, y + yoff, ta);
      },
      [&](int i, int j) {
        real ns = noise(i * 0.1, j * 0.1, seed + 2);
        return ns * ns * ns < 0.1 && fabs(ptlist[i][j].y) / h > 0.5;
      },
      always);

  if (veg) {
    // MIDDLE
    vegetate(
        [&](real x, real y) {
          real ht = ((h + y) / h) * 70;
          ht = ht * 0.3 + rnd() * ht * 0.7;
          TreeArgs ta;
          ta.hei = ht;
          ta.wid = rnd() * 3 + 1;
          ta.col = grayA("100,100,100", noise(0.01 * x, 0.01 * y) * 0.5 * 0.3 + 0.3);
          return tree01(x + xoff, y + yoff, ta);
        },
        [&](int i, int j) {
          real ns = noise(i * 0.2, j * 0.05, seed);
          return j % 2 && ns * ns * ns * ns < 0.012 && fabs(ptlist[i][j].y) / h < 0.3;
        },
        [](const Pts& veglist, int i) {
          int counter = 0;
          for (int j = 0; j < (int)veglist.size(); j++) {
            if (i != j && pow(veglist[i].x - veglist[j].x, 2) +
                                  pow(veglist[i].y - veglist[j].y, 2) <
                              30 * 30) {
              counter++;
            }
            if (counter > 2) return true;
          }
          return false;
        });

    // BOTTOM
    vegetate(
        [&](real x, real y) {
          real ht = ((h + y) / h) * 120;
          ht = ht * 0.5 + rnd() * ht * 0.5;
          real bc = rnd() * 0.1;
          real bp = 1;
          TreeArgs ta;
          ta.hei = ht;
          ta.ben = Fn([bc, bp](real x) { return pow(x * bc, bp); });
          ta.col = grayA("100,100,100", noise(0.01 * x, 0.01 * y) * 0.5 * 0.3 + 0.3);
          return tree03(x + xoff, y + yoff, ta);
        },
        [&](int i, int j) {
          real ns = noise(i * 0.2, j * 0.05, seed);
          return (j == 0 || j == (int)ptlist[i].size() - 1) && ns * ns * ns * ns < 0.012;
        },
        always);
  }

  // BOTT ARCH
  vegetate(
      [&](real x, real y) {
        real tt = randChoice<real>({0, 0, 1, 1, 1, 2});
        if (tt == 1) {
          ArchArgs aa;
          aa.wid = normRand(40, 70);
          aa.sto = randChoice<real>({1, 2, 2, 3});
          aa.rot = rnd();
          aa.sty = randChoice<real>({1, 2, 3});
          return arch02(x + xoff, y + yoff, seed, aa);
        } else if (tt == 2) {
          ArchArgs aa;
          aa.sto = randChoice<real>({1, 1, 1, 2, 2});
          return arch04(x + xoff, y + yoff, seed, aa);
        } else {
          return Canvas();
        }
      },
      [&](int i, int j) {
        real ns = noise(i * 0.2, j * 0.05, seed + 10);
        return i != 0 && (j == 1 || j == (int)ptlist[i].size() - 2) && ns * ns * ns * ns < 0.008;
      },
      always);
  // TOP ARCH
  vegetate(
      [&](real x, real y) {
        ArchArgs aa;
        aa.sto = randChoice<real>({5, 7});
        aa.wid = 40 + rnd() * 20;
        return arch03(x + xoff, y + yoff, seed, aa);
      },
      [&](int i, int j) {
        return i == 1 && fabs(j - ptlist[i].size() / 2.0) < 1 && rnd() < 0.02;
      },
      always);

  // TRANSM
  vegetate([&](real x, real y) { return transmissionTower01(x + xoff, y + yoff, seed); },
           [&](int i, int j) {
             real ns = noise(i * 0.2, j * 0.05, seed + 20 * PI);
             return i % 2 == 0 && (j == 1 || j == (int)ptlist[i].size() - 2) &&
                    ns * ns * ns * ns < 0.002;
           },
           always);

  // BOTT ROCK
  vegetate(
      [&](real x, real y) {
        RockArgs ra;
        ra.wid = 20 + rnd() * 20;
        ra.hei = 20 + rnd() * 20;
        ra.sha = 2;
        return rock(x + xoff, y + yoff, seed, ra);
      },
      [&](int i, int j) {
        return (j == 0 || j == (int)ptlist[i].size() - 1) && rnd() < 0.1;
      },
      always);

  if (ret) *ret = ptlist;
  return canv;
}

}  // namespace

Canvas mountain(real xoff, real yoff, real seed, const MountArgs& args) {
  return mountainImpl(xoff, yoff, seed, args, nullptr);
}

std::vector<Pts> mountainPts(real xoff, real yoff, real seed, const MountArgs& args) {
  std::vector<Pts> ptlist;
  mountainImpl(xoff, yoff, seed, args, &ptlist);
  return ptlist;
}

Canvas flatMount(real xoff, real yoff, real seed, const FlatMountArgs& args) {
  real hei = args.hei ? *args.hei : 40 + rnd() * 400;
  real wid = args.wid ? *args.wid : 400 + rnd() * 200;
  real tex = args.tex ? *args.tex : 80;
  real cho = args.cho ? *args.cho : 0.5;

  Canvas canv;
  std::vector<Pts> ptlist;
  int reso[2] = {5, 50};
  real hoff = 0;
  std::vector<Pts> flat;
  for (int j = 0; j < reso[0]; j++) {
    hoff += (rnd() * yoff) / 100;
    ptlist.push_back(Pts());
    flat.push_back(Pts());
    for (int i = 0; i < reso[1]; i++) {
      real x = ((real)i / reso[1] - 0.5) * PI;
      real y = cos(x * 2) + 1;
      y *= noise(x + 10, j * 0.1, seed);
      real p = 1 - ((real)j / reso[0]) * 0.6;
      real nx = (x / PI) * wid * p;
      real ny = -y * hei * p + hoff;
      real h = 100;
      if (ny < -h * cho + hoff) {
        ny = -h * cho + hoff;
        if (flat.back().size() % 2 == 0) flat.back().push_back(Pt(nx, ny));
      } else {
        // Never reached with an empty row: flat rows start even.
        if (flat.back().size() % 2 == 1) flat.back().push_back(ptlist.back().back());
      }

      ptlist.back().push_back(Pt(nx, ny));
    }
  }

  // WHITE BG
  {
    Pts bg = ptlist[0];
    bg.push_back(Pt(0, reso[0] * 4));
    PolyArgs pa;
    pa.xof = xoff;
    pa.yof = yoff;
    pa.fil = "white";
    pa.str = "none";
    canv.add(poly(bg, pa));
  }
  // OUTLINE
  {
    StrokeArgs sa;
    sa.col = "rgba(100,100,100,0.3)";
    sa.noi = 1;
    sa.wid = 3;
    canv.add(stroke(shifted(ptlist[0], xoff, yoff), sa));
  }

  {
    TextureArgs ta;
    ta.xof = xoff;
    ta.yof = yoff;
    ta.tex = tex;
    ta.wid = 2;
    ta.dis = []() {
      if (rnd() > 0.5) {
        return 0.1 + 0.4 * rnd();
      } else {
        return 0.9 - 0.4 * rnd();
      }
    };
    canv.add(texture(ptlist, ta));
  }
  Pts grlist1, grlist2;
  for (size_t i = 0; i < flat.size(); i += 2) {
    if (flat[i].size() >= 2) {
      grlist1.push_back(flat[i][0]);
      grlist2.push_back(flat[i][flat[i].size() - 1]);
    }
  }

  if (grlist1.empty()) return canv;
  real wb[2] = {grlist1[0].x, grlist2[0].x};
  for (int i = 0; i < 3; i++) {
    real p = 0.8 - i * 0.2;
    grlist1.insert(grlist1.begin(), Pt(wb[0] * p, grlist1[0].y - 5));
    grlist2.insert(grlist2.begin(), Pt(wb[1] * p, grlist2[0].y - 5));
  }
  wb[0] = grlist1.back().x;
  wb[1] = grlist2.back().x;
  for (int i = 0; i < 3; i++) {
    real p = 0.6 - i * i * 0.1;
    grlist1.push_back(Pt(wb[0] * p, grlist1.back().y + 1));
    grlist2.push_back(Pt(wb[1] * p, grlist2.back().y + 1));
  }

  real d = 5;
  grlist1 = div(grlist1, d);
  grlist2 = div(grlist2, d);

  // grlist1.reverse().concat(grlist2.concat([grlist1[0]])): div() returns
  // its input's last point by reference, so the first and last entries of
  // grlist are one array, and the loop below scales it twice.
  std::reverse(grlist1.begin(), grlist1.end());
  Pts grlist = grlist1;
  for (auto& v : grlist2) grlist.push_back(v);
  size_t n = grlist.size() + 1;
  for (size_t i = 0; i < n; i++) {
    Pt& g = i == n - 1 ? grlist[0] : grlist[i];
    real v = (1 - fabs(fmod((real)i, d) - d / 2) / (d / 2)) * 0.12;
    g.x *= 1 - v + noise(g.y * 0.5) * v;
  }
  grlist.push_back(grlist[0]);

  {
    PolyArgs pa;
    pa.xof = xoff;
    pa.yof = yoff;
    pa.str = "none";
    pa.fil = "white";
    pa.wid = 2;
    canv.add(poly(grlist, pa));
  }
  {
    StrokeArgs sa;
    sa.wid = 3;
    sa.col = "rgba(100,100,100,0.2)";
    canv.add(stroke(shifted(grlist, xoff, yoff), sa));
  }

  Bound bd;
  for (size_t i = 0; i < grlist.size(); i++) {
    if (i == 0 || grlist[i].x < bd.xmin) bd.xmin = grlist[i].x;
    if (i == 0 || grlist[i].x > bd.xmax) bd.xmax = grlist[i].x;
    if (i == 0 || grlist[i].y < bd.ymin) bd.ymin = grlist[i].y;
    if (i == 0 || grlist[i].y > bd.ymax) bd.ymax = grlist[i].y;
  }

  canv.add(flatDec(xoff, yoff, bd));

  return canv;
}

Canvas flatDec(real xoff, real yoff, const Bound& grbd) {
  Canvas canv;

  real tt = randChoice<real>({0, 0, 1, 2, 3, 4});

  for (int j = 0; j < rnd() * 5; j++) {
    real x = xoff + normRand(grbd.xmin, grbd.xmax);
    real y = yoff + (grbd.ymin + grbd.ymax) / 2 + normRand(-10, 10) + 10;
    real s = rnd() * 100;
    RockArgs ra;
    ra.wid = 10 + rnd() * 20;
    ra.hei = 10 + rnd() * 20;
    ra.sha = 2;
    canv.add(rock(x, y, s, ra));
  }
  for (int j = 0; j < randChoice<real>({0, 0, 1, 2}); j++) {
    real xr = xoff + normRand(grbd.xmin, grbd.xmax);
    real yr = yoff + (grbd.ymin + grbd.ymax) / 2 + normRand(-5, 5) + 20;
    for (int k = 0; k < 2 + rnd() * 3; k++) {
      real x = xr + jmin(jmax(normRand(-30, 30), grbd.xmin), grbd.xmax);
      TreeArgs ta;
      ta.hei = 60 + rnd() * 40;
      canv.add(tree08(x, yr, ta));
    }
  }

  if (tt == 0) {
    for (int j = 0; j < rnd() * 3; j++) {
      real x = xoff + normRand(grbd.xmin, grbd.xmax);
      real y = yoff + (grbd.ymin + grbd.ymax) / 2 + normRand(-5, 5) + 20;
      real s = rnd() * 100;
      RockArgs ra;
      ra.wid = 50 + rnd() * 20;
      ra.hei = 40 + rnd() * 20;
      ra.sha = 5;
      canv.add(rock(x, y, s, ra));
    }
  }
  if (tt == 1) {
    real pmin = rnd() * 0.5;
    real pmax = rnd() * 0.5 + 0.5;
    real xmin = grbd.xmin * (1 - pmin) + grbd.xmax * pmin;
    real xmax = grbd.xmin * (1 - pmax) + grbd.xmax * pmax;
    for (real i = xmin; i < xmax; i += 30) {
      real x = xoff + i + 20 * normRand(-1, 1);
      real y = yoff + (grbd.ymin + grbd.ymax) / 2 + 20;
      TreeArgs ta;
      ta.hei = 100 + rnd() * 200;
      canv.add(tree05(x, y, ta));
    }
    for (int j = 0; j < rnd() * 4; j++) {
      real x = xoff + normRand(grbd.xmin, grbd.xmax);
      real y = yoff + (grbd.ymin + grbd.ymax) / 2 + normRand(-5, 5) + 20;
      real s = rnd() * 100;
      RockArgs ra;
      ra.wid = 50 + rnd() * 20;
      ra.hei = 40 + rnd() * 20;
      ra.sha = 5;
      canv.add(rock(x, y, s, ra));
    }
  } else if (tt == 2) {
    for (int i = 0; i < randChoice<real>({1, 1, 1, 1, 2, 2, 3}); i++) {
      real xr = normRand(grbd.xmin, grbd.xmax);
      real yr = (grbd.ymin + grbd.ymax) / 2;
      canv.add(tree04(xoff + xr, yoff + yr + 20, TreeArgs()));
      for (int j = 0; j < rnd() * 2; j++) {
        real x = xoff + jmax(grbd.xmin, jmin(grbd.xmax, xr + normRand(-50, 50)));
        real y = yoff + yr + normRand(-5, 5) + 20;
        real s = j * i * rnd() * 100;
        RockArgs ra;
        ra.wid = 50 + rnd() * 20;
        ra.hei = 40 + rnd() * 20;
        ra.sha = 5;
        canv.add(rock(x, y, s, ra));
      }
    }
  } else if (tt == 3) {
    for (int i = 0; i < randChoice<real>({1, 1, 1, 1, 2, 2, 3}); i++) {
      real x = xoff + normRand(grbd.xmin, grbd.xmax);
      real y = yoff + (grbd.ymin + grbd.ymax) / 2;
      TreeArgs ta;
      ta.hei = 60 + rnd() * 60;
      canv.add(tree06(x, y, ta));
    }
  } else if (tt == 4) {
    real pmin = rnd() * 0.5;
    real pmax = rnd() * 0.5 + 0.5;
    real xmin = grbd.xmin * (1 - pmin) + grbd.xmax * pmin;
    real xmax = grbd.xmin * (1 - pmax) + grbd.xmax * pmax;
    for (real i = xmin; i < xmax; i += 20) {
      real x = xoff + i + 20 * normRand(-1, 1);
      real y = yoff + (grbd.ymin + grbd.ymax) / 2 + normRand(-1, 1) + 0;
      TreeArgs ta;
      ta.hei = normRand(40, 80);
      canv.add(tree07(x, y, ta));
    }
  }

  for (int i = 0; i < 50 * rnd(); i++) {
    real x = xoff + normRand(grbd.xmin, grbd.xmax);
    real y = yoff + normRand(grbd.ymin, grbd.ymax);
    canv.add(tree02(x, y));
  }

  real ts = randChoice<real>({0, 0, 0, 0, 1});
  if (ts == 1 && tt != 4) {
    real x = xoff + normRand(grbd.xmin, grbd.xmax);
    real y = yoff + (grbd.ymin + grbd.ymax) / 2 + 20;
    real s = rnd();
    ArchArgs aa;
    aa.wid = normRand(160, 200);
    aa.hei = normRand(80, 100);
    aa.per = rnd();
    canv.add(arch01(x, y, s, aa));
  }

  return canv;
}

Canvas distMount(real xoff, real yoff, real seed, const DistMountArgs& args) {
  real hei = args.hei ? *args.hei : 300;
  real len = args.len ? *args.len : 2000;
  real seg = args.seg ? *args.seg : 5;

  Canvas canv;
  real span = 10;

  std::vector<Pts> ptlist;

  for (int i = 0; i < len / span / seg; i++) {
    ptlist.push_back(Pts());
    for (int j = 0; j < seg + 1; j++) {
      real k = i * seg + j;
      ptlist.back().push_back(Pt(
          xoff + k * span,
          yoff - hei * noise(k * 0.05, seed) * pow(sin((PI * k) / (len / span)), 0.5)));
    }
    for (int j = 0; j < seg / 2 + 1; j++) {
      real k = i * seg + j * 2;
      ptlist.back().insert(
          ptlist.back().begin(),
          Pt(xoff + k * span,
             yoff + 24 * noise(k * 0.05, 2, seed) * pow(sin((PI * k) / (len / span)), 1)));
    }
  }
  auto getCol = [&](real x, real y) {
    std::string c = std::to_string(toInt32(noise(x * 0.02, y * 0.02, yoff) * 55 + 200));
    return "rgb(" + c + "," + c + "," + c + ")";
  };
  for (size_t i = 0; i < ptlist.size(); i++) {
    const Pt& last = ptlist[i][ptlist[i].size() - 1];
    PolyArgs pa;
    pa.fil = getCol(last.x, last.y);
    pa.str = "none";
    pa.wid = 1;
    canv.add(poly(ptlist[i], pa));

    TriArgs ta;
    ta.area = 100;
    ta.convex = true;
    ta.optimize = false;
    std::vector<Pts> T = triangulate(ptlist[i], ta);
    for (size_t k = 0; k < T.size(); k++) {
      Pt m = midPt(T[k]);
      std::string co = getCol(m.x, m.y);
      PolyArgs pb;
      pb.fil = co;
      pb.str = co;
      pb.wid = 1;
      canv.add(poly(T[k], pb));
    }
  }
  return canv;
}

Canvas rock(real xoff, real yoff, real seed, const RockArgs& args) {
  real hei = args.hei ? *args.hei : 80;
  real wid = args.wid ? *args.wid : 100;
  real tex = args.tex ? *args.tex : 40;
  real sha = args.sha ? *args.sha : 10;

  Canvas canv;

  int reso[2] = {10, 50};
  std::vector<Pts> ptlist;

  for (int i = 0; i < reso[0]; i++) {
    ptlist.push_back(Pts());

    std::vector<real> nslist;
    for (int j = 0; j < reso[1]; j++) nslist.push_back(noise(i, j * 0.2, seed));
    loopNoise(nslist);

    for (int j = 0; j < reso[1]; j++) {
      real a = ((real)j / reso[1]) * PI * 2 - PI / 2;
      real l = (wid * hei) / sqrt(pow(hei * cos(a), 2) + pow(wid * sin(a), 2));

      l *= 0.7 + 0.3 * nslist[j];

      real p = 1 - (real)i / reso[0];

      real nx = cos(a) * l * p;
      real ny = -sin(a) * l * p;

      if (PI < a || a < 0) ny *= 0.2;

      ny += hei * ((real)i / reso[0]) * 0.2;

      ptlist.back().push_back(Pt(nx, ny));
    }
  }

  // WHITE BG
  {
    Pts bg = ptlist[0];
    bg.push_back(Pt(0, 0));
    PolyArgs pa;
    pa.xof = xoff;
    pa.yof = yoff;
    pa.fil = "white";
    pa.str = "none";
    canv.add(poly(bg, pa));
  }
  // OUTLINE
  {
    StrokeArgs sa;
    sa.col = "rgba(100,100,100,0.3)";
    sa.noi = 1;
    sa.wid = 3;
    canv.add(stroke(shifted(ptlist[0], xoff, yoff), sa));
  }
  {
    TextureArgs ta;
    ta.xof = xoff;
    ta.yof = yoff;
    ta.tex = tex;
    ta.wid = 3;
    ta.sha = sha;
    ta.col = [](real) { return grayA("180,180,180", 0.3 + rnd() * 0.3); };
    ta.dis = []() {
      if (rnd() > 0.5) {
        return 0.15 + 0.15 * rnd();
      } else {
        return 0.85 - 0.15 * rnd();
      }
    };
    canv.add(texture(ptlist, ta));
  }
  return canv;
}

}  // namespace ss
