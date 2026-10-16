#pragma once

// Turns generator polylines into panel pixels: an anti-aliased grayscale
// tile, then the same Floyd-Steinberg dither and sky clouds the Mac host used
// (host/lib/dither.js, host/lib/clouds.js).

#include <stdint.h>

#include <vector>

#include "core.h"

namespace ss {

// shan-shui-inf's viewBox shows MEM.windy / zoom user units of height; the
// host mapped that onto the panel's 300 rows.
static const float kSceneUserH = 800.0f / 1.142f;

class Raster {
 public:
  // Tile `w` x `h` device pixels, rendered at `sub` x `sub` samples each.
  bool init(int w, int h, int sub);
  int width() const { return w_; }
  int height() const { return h_; }

  // Start a tile whose left edge is device column `device_x0`.
  void begin(int64_t device_x0);
  float user_x0() const { return ux0_; }
  float user_x1() const { return ux0_ + w_ / px_per_user_; }
  // Paint polyline i of `c`: fill, then stroke, like the browser.
  void draw(const Canvas& c, size_t i);
  // Average the samples into `gray`, row-major w x h, 255 = paper.
  void end(uint8_t* gray) const;

 private:
  struct Edge {
    float x0, y0, x1, y1;
    int dir;
  };
  void path(int gray, int alpha);  // fills edges_ (nonzero), alpha 0..255
  void add_poly(const float* xy, int n, bool closed);
  void hairline(const float* xy, int n, int gray, int alpha);
  void add_quad(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy);

  int w_ = 0, h_ = 0, sub_ = 1;
  int sw_ = 0, sh_ = 0;
  float px_per_user_ = 1;
  float ux0_ = 0;
  uint8_t* buf_ = nullptr;
  std::vector<Edge> edges_;
  std::vector<float> xs_;
  std::vector<int> ds_;
  std::vector<int> active_;
  std::vector<float> xy_;
};

// host/lib/dither.js ditherStrip(): `carry` (h floats) crosses into the
// next tile. `bits` gets 1 = paper, 0 = ink, row-major.
void dither_tile(const uint8_t* gray, int w, int h, float* carry, uint8_t* bits,
                 int paper_cutoff = 224, int threshold = 128);

// host/lib/dither.js skylineFromGray().
void skyline(const uint8_t* gray, int w, int h, int max_luma, int16_t* out);

// host/lib/clouds.js applyClouds().
uint32_t cloud_salt(const char* seed);
void clouds(uint8_t* bits, int w, int h, int64_t origin_x, uint32_t salt, const int16_t* sky);

// host/lib/dither.js packColumns(): column-major, bit (7 - y%8) of byte y/8.
void pack_columns(const uint8_t* bits, int w, int h, uint8_t* out);

}  // namespace ss
