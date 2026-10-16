#pragma once

// shan-shui-inf's mountplanner() and chunkloader(), plus the part the page
// left to the browser: painting the chunks that overlap a window of columns.

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "core.h"
#include "raster.h"

namespace ss {

struct Plan {
  std::string tag;
  real x, y, h;
};

class World {
 public:
  // `tile_w` columns are painted per render() call, `sub` samples per pixel.
  bool init(int tile_w, int height, int sub);
  void start(const std::string& seed);
  // Paint device columns [device_x0, device_x0 + tile_w) and pack them like
  // the host did (column-major, 1 = paper). Tiles must move left to right.
  void render(int64_t device_x0, uint8_t* packed);
  int tile_width() const { return raster_.width(); }

  // For tests: the chunks as chunkloader() would hold them.
  void chunkloader(real xmin, real xmax);
  std::vector<Plan> mountplanner(real xmin, real xmax);
  struct Chunk {
    std::string tag;
    real x, y;
    Canvas canv;
    float minx, maxx;
  };
  const std::vector<Chunk>& chunks() const { return chunks_; }
  size_t point_count() const;
  size_t bytes() const;  // memory held by painted-but-unused chunks
  // Plan and paint chunks this far (user units) right of the tile. The
  // planner puts a mountain up to 500 units left of its slot, so too little
  // look-ahead adds it after some of its columns were already drawn.
  void set_ahead(real units) { ahead_ = units; }
  real late() const { return late_; }  // worst such miss so far, user units
  // Time spent so far, in the units of the clock passed to set_clock().
  uint32_t t_gen = 0, t_raster = 0, t_post = 0;
  void set_clock(uint32_t (*now)()) { clock_ = now; }

 private:
  void add(Chunk&& nch);
  double planmtx_get(int64_t k) const;

  uint32_t (*clock_)() = nullptr;
  std::string seed_;
  real ahead_ = 600;
  real drawn_x1_ = -INFINITY;
  real late_ = 0;
  std::vector<Chunk> chunks_;
  real mem_xmin_ = 0, mem_xmax_ = 0;
  const real cwid_ = 512;
  // MEM.planmtx: missing = undefined, NaN = undefined + 1.
  std::map<int64_t, double> planmtx_;

  Raster raster_;
  std::vector<uint8_t> gray_, bits_;
  std::vector<int16_t> sky_;
  std::vector<float> carry_;
  uint32_t salt_ = 0;
};

}  // namespace ss
