#include "world.h"

#include <algorithm>

#include "arch.h"
#include "mount.h"

namespace ss {

bool World::init(int tile_w, int height, int sub) {
  if (!raster_.init(tile_w, height, sub)) return false;
  gray_.resize((size_t)tile_w * height);
  bits_.resize((size_t)tile_w * height);
  sky_.resize(tile_w);
  carry_.assign(height, 0.0f);
  return true;
}

void World::start(const std::string& s) {
  seed_ = s;
  seed(s);
  chunks_.clear();
  planmtx_.clear();
  mem_xmin_ = 0;
  mem_xmax_ = 0;
  drawn_x1_ = -INFINITY;
  late_ = 0;
  std::fill(carry_.begin(), carry_.end(), 0.0f);
  salt_ = cloud_salt(s.c_str());
}

double World::planmtx_get(int64_t k) const {
  auto it = planmtx_.find(k);
  return it == planmtx_.end() ? NAN : it->second;
}

std::vector<Plan> World::mountplanner(real xmin, real xmax) {
  std::vector<Plan> reg;
  const real samp = 0.03;
  auto ns = [&](real x, real y) -> real { return std::max<real>(noise(x * samp) - 0.55, 0) * 2; };
  auto yr = [&](real x) { return noise(x * 0.01, PI); };
  auto locmax = [&](real x, real y, real r) {
    real z0 = ns(x, y);
    if (z0 <= 0.3) return false;
    for (real i = x - r; i < x + r; i++)
      for (real j = y - r; j < y + r; j++)
        if (ns(i, j) > z0) return false;
    return true;
  };
  auto chadd = [&](const Plan& r, real mind) {
    for (auto& k : reg)
      if (fabs(k.x - r.x) < mind) return false;
    reg.push_back(r);
    return true;
  };

  const real xstep = 5;
  const real mwid = 200;
  for (real i = xmin; i < xmax; i += xstep) {
    int64_t i1 = (int64_t)floor(i / xstep);
    double v = planmtx_get(i1);
    planmtx_[i1] = (isnan(v) || v == 0) ? 0 : v;  // planmtx[i1] || 0
  }

  real j = 0;  // `var j` outlives its loop; distmount reads it
  for (real i = xmin; i < xmax; i += xstep) {
    for (j = 0; j < yr(i) * 480; j += 30) {
      if (locmax(i, j, 2)) {
        real xof = i + 2 * (rnd() - 0.5) * 500;
        real yof = j + 300;
        Plan r{"mount", xof, yof, ns(i, j)};
        if (chadd(r, 10)) {
          for (real k = floor((xof - mwid) / xstep); k < (xof + mwid) / xstep; k++) {
            double v = planmtx_get((int64_t)k);
            planmtx_[(int64_t)k] = v + 1;  // undefined + 1 is NaN
          }
        }
      }
    }
    if (fmod(fabs(i), 1000) < std::max<real>(1, xstep - 1)) {
      real y = 280 - rnd() * 50;
      chadd(Plan{"distmount", i, y, ns(i, j)}, 10);
    }
  }
  for (real i = xmin; i < xmax; i += xstep) {
    if (planmtx_get((int64_t)floor(i / xstep)) == 0) {
      if (rnd() < 0.01) {
        for (j = 0; j < 4 * rnd(); j++) {
          real x = i + 2 * (rnd() - 0.5) * 700;
          chadd(Plan{"flatmount", x, (real)(700 - j * 50), ns(i, j)}, 10);
        }
      }
    }
  }
  for (real i = xmin; i < xmax; i += xstep) {
    if (rnd() < 0.2) {
      real y = 300 + rnd() * 390;
      chadd(Plan{"boat", i, y, 0}, 400);
    }
  }
  return reg;
}

void World::add(Chunk&& nch) {
  nch.canv.bounds(&nch.minx, &nch.maxx);
  if (nch.minx < drawn_x1_ && drawn_x1_ - nch.minx > late_) late_ = drawn_x1_ - nch.minx;
  nch.canv.shrink();
  if (chunks_.empty()) {
    chunks_.push_back(std::move(nch));
    return;
  }
  if (nch.y <= chunks_[0].y) {
    chunks_.insert(chunks_.begin(), std::move(nch));
    return;
  }
  if (nch.y >= chunks_.back().y) {
    chunks_.push_back(std::move(nch));
    return;
  }
  for (size_t j = 0; j + 1 < chunks_.size(); j++) {
    if (chunks_[j].y <= nch.y && nch.y <= chunks_[j + 1].y) {
      chunks_.insert(chunks_.begin() + j + 1, std::move(nch));
      return;
    }
  }
}

void World::chunkloader(real xmin, real xmax) {
  while (xmax > mem_xmax_ - cwid_ || xmin < mem_xmin_ + cwid_) {
    std::vector<Plan> plan;
    if (xmax > mem_xmax_ - cwid_) {
      plan = mountplanner(mem_xmax_, mem_xmax_ + cwid_);
      mem_xmax_ = mem_xmax_ + cwid_;
    } else {
      plan = mountplanner(mem_xmin_ - cwid_, mem_xmin_);
      mem_xmin_ = mem_xmin_ - cwid_;
    }
    for (size_t i = 0; i < plan.size(); i++) {
      const Plan& p = plan[i];
      if (p.tag == "mount") {
        real s = i * 2 * rnd();
        add(Chunk{p.tag, p.x, p.y, mountain(p.x, p.y, s), 0, 0});
        add(Chunk{p.tag, p.x, p.y - 10000, water(p.x, p.y, i * 2), 0, 0});
      } else if (p.tag == "flatmount") {
        real s = 2 * rnd() * PI;
        FlatMountArgs a;
        a.wid = 600 + rnd() * 400;
        a.hei = 100;
        a.cho = 0.5 + rnd() * 0.2;
        add(Chunk{p.tag, p.x, p.y, flatMount(p.x, p.y, s, a), 0, 0});
      } else if (p.tag == "distmount") {
        real s = rnd() * 100;
        DistMountArgs a;
        a.hei = 150;
        a.len = randChoice(std::vector<real>{500, 1000, 1500});
        add(Chunk{p.tag, p.x, p.y, distMount(p.x, p.y, s, a), 0, 0});
      } else if (p.tag == "boat") {
        real s = rnd();
        ArchArgs a;
        a.sca = p.y / 800;
        a.fli = randChoice(std::vector<bool>{true, false});
        add(Chunk{p.tag, p.x, p.y, boat01(p.x, p.y, s, a), 0, 0});
      }
    }
  }
}

size_t World::bytes() const {
  size_t n = 0;
  for (auto& c : chunks_) n += c.canv.bytes() + sizeof(Chunk);
  return n;
}

size_t World::point_count() const {
  size_t n = 0;
  for (auto& c : chunks_)
    n += c.canv.points();
  return n;
}

void World::render(int64_t device_x0, uint8_t* packed) {
  int w = raster_.width(), h = raster_.height();
  raster_.begin(device_x0);
  real ux0 = raster_.user_x0(), ux1 = raster_.user_x1();
  uint32_t t0 = clock_ ? clock_() : 0;
  chunkloader(ux0, ux1 + ahead_);
  uint32_t t1 = clock_ ? clock_() : 0;
  for (auto& c : chunks_) {
    if (c.maxx < ux0 || c.minx > ux1) continue;
    for (size_t i = 0; i < c.canv.prims.size(); i++) raster_.draw(c.canv, i);
  }
  raster_.end(gray_.data());
  drawn_x1_ = ux1;
  uint32_t t2 = clock_ ? clock_() : 0;
  // Polylines left of this tile can never be drawn again. Compacting a chunk
  // costs a copy, so do it once a chunk's left edge is well behind.
  chunks_.erase(std::remove_if(chunks_.begin(), chunks_.end(),
                               [&](const Chunk& c) { return c.maxx < ux0; }),
                chunks_.end());
  for (auto& c : chunks_) {
    if (c.minx < ux0 - 200) {
      c.canv.drop_left(ux0);
      c.canv.bounds(&c.minx, &c.maxx);
    }
  }
  for (auto it = planmtx_.begin(); it != planmtx_.end() && it->first * 5.0 < ux0 - 4000;)
    it = planmtx_.erase(it);

  skyline(gray_.data(), w, h, 200, sky_.data());
  dither_tile(gray_.data(), w, h, carry_.data(), bits_.data());
  clouds(bits_.data(), w, h, device_x0, salt_, sky_.data());
  pack_columns(bits_.data(), w, h, packed);
  uint32_t t3 = clock_ ? clock_() : 0;
  t_gen += t1 - t0;
  t_raster += t2 - t1;
  t_post += t3 - t2;
}

}  // namespace ss
