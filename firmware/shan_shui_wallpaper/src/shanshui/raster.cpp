#include "raster.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>

#if defined(ARDUINO)
#include <esp32-hal-psram.h>
#include <esp_heap_caps.h>
static void* big_alloc(size_t n) {
  void* p = ps_malloc(n);
  return p ? p : malloc(n);
}
// The sample buffer is rewritten for every polyline; internal RAM is several
// times faster than PSRAM for that.
static void* fast_alloc(size_t n) {
  void* p = heap_caps_malloc(n, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  return p ? p : big_alloc(n);
}
#else
static void* big_alloc(size_t n) { return malloc(n); }
static void* fast_alloc(size_t n) { return malloc(n); }
#endif

namespace ss {

// ================================================================ raster

bool Raster::init(int w, int h, int sub) {
  w_ = w;
  h_ = h;
  sub_ = sub;
  sw_ = w * sub;
  sh_ = h * sub;
  px_per_user_ = (float)h / kSceneUserH;
  buf_ = (uint8_t*)fast_alloc((size_t)sw_ * sh_);
  return buf_ != nullptr;
}

void Raster::begin(int64_t device_x0) {
  ux0_ = (float)((double)device_x0 / px_per_user_);
  memset(buf_, 255, (size_t)sw_ * sh_);
}

void Raster::add_poly(const float* xy, int n, bool closed) {
  float k = px_per_user_ * sub_;
  int m = closed ? n : n - 1;
  for (int i = 0; i < m; i++) {
    int j = (i + 1) % n;
    Edge e;
    e.x0 = (xy[2 * i] - ux0_) * k;
    e.y0 = xy[2 * i + 1] * k;
    e.x1 = (xy[2 * j] - ux0_) * k;
    e.y1 = xy[2 * j + 1] * k;
    if (e.y0 == e.y1 || isnan(e.x0 + e.y0 + e.x1 + e.y1)) continue;
    e.dir = e.y1 > e.y0 ? 1 : -1;
    edges_.push_back(e);
  }
}

void Raster::add_quad(float ax, float ay, float bx, float by, float cx, float cy, float dx,
                      float dy) {
  // Every piece of a stroke gets the same winding so nonzero fill unions
  // them; overlapping joins are painted once, as in SVG.
  float area = (bx - ax) * (by + ay) + (cx - bx) * (cy + by) + (dx - cx) * (dy + cy) +
               (ax - dx) * (ay + dy);
  float q[8] = {ax, ay, bx, by, cx, cy, dx, dy};
  if (area < 0) {
    float r[8] = {dx, dy, cx, cy, bx, by, ax, ay};
    memcpy(q, r, sizeof q);
  }
  add_poly(q, 4, true);
}

void Raster::path(int g, int alpha) {
  if (edges_.empty() || alpha <= 0) return;
  int a8 = alpha == 255 ? 256 : alpha;

  float ylo = INFINITY, yhi = -INFINITY;
  for (auto& e : edges_) {
    float t = e.y0 < e.y1 ? e.y0 : e.y1;
    float b = e.y0 < e.y1 ? e.y1 : e.y0;
    if (t < ylo) ylo = t;
    if (b > yhi) yhi = b;
    if (e.y0 > e.y1) {
      std::swap(e.x0, e.x1);
      std::swap(e.y0, e.y1);
    }
  }
  int r0 = (int)ceilf(ylo - 0.5f);
  int r1 = (int)floorf(yhi - 0.5f);
  if (r0 < 0) r0 = 0;
  if (r1 > sh_ - 1) r1 = sh_ - 1;
  if (r0 > r1) return;
  std::sort(edges_.begin(), edges_.end(), [](const Edge& a, const Edge& b) { return a.y0 < b.y0; });

  size_t next = 0;
  std::vector<int>& active = active_;
  active.clear();
  for (int r = r0; r <= r1; r++) {
    float sy = r + 0.5f;
    while (next < edges_.size() && edges_[next].y0 <= sy) active.push_back((int)next++);
    xs_.clear();
    ds_.clear();
    size_t keep = 0;
    for (size_t k = 0; k < active.size(); k++) {
      const Edge& e = edges_[active[k]];
      if (e.y1 <= sy) continue;
      active[keep++] = active[k];
      float x = e.x0 + (sy - e.y0) * (e.x1 - e.x0) / (e.y1 - e.y0);
      // insertion sort: crossings per row are few
      size_t pos = xs_.size();
      xs_.push_back(x);
      ds_.push_back(e.dir);
      while (pos > 0 && xs_[pos - 1] > x) {
        xs_[pos] = xs_[pos - 1];
        ds_[pos] = ds_[pos - 1];
        pos--;
      }
      xs_[pos] = x;
      ds_[pos] = e.dir;
    }
    active.resize(keep);
    uint8_t* row = buf_ + (size_t)r * sw_;
    int wind = 0;
    for (size_t k = 0; k + 1 < xs_.size(); k++) {
      wind += ds_[k];
      if (!wind) continue;
      int c0 = (int)ceilf(xs_[k] - 0.5f);
      int c1 = (int)ceilf(xs_[k + 1] - 0.5f);
      if (c0 < 0) c0 = 0;
      if (c1 > sw_) c1 = sw_;
      for (int c = c0; c < c1; c++) {
        int v = row[c];
        row[c] = (uint8_t)(v + (((g - v) * a8) >> 8));
      }
    }
  }
}

void Raster::draw(const Canvas& cv, size_t i) {
  const Prim& p = cv.prims[i];
  if (p.maxx < ux0_ || p.minx > user_x1()) return;
  int n = (int)p.n;
  if (n < 2) return;
  bool fill = p.fill_a > 0 && n >= 3;
  bool stroke = p.stroke_a > 0 && p.wid16 > 0;
  if (!fill && !stroke) return;
  cv.decode(i, xy_);
  const float* xy = xy_.data();
  if (fill) {
    edges_.clear();
    add_poly(xy, n, true);
    path(p.fill_g, p.fill_a);
  }
  if (stroke && p.wid() * px_per_user_ * sub_ <= 1.5f) {
    // Nearly every outline here is under one sample wide: plot it as a line
    // of single samples whose strength stands in for its width. Building and
    // scanning quads for it cost several times more than everything else.
    float cover = p.wid() * px_per_user_ * sub_;
    hairline(xy, n, p.stroke_g, (int)(p.stroke_a * (cover < 1 ? cover : 1) + 0.5f));
  } else if (stroke) {
    edges_.clear();
    float hw = p.wid() * 0.5f;
    for (int i = 0; i + 1 < n; i++) {
      float ax = xy[2 * i], ay = xy[2 * i + 1];
      float bx = xy[2 * i + 2], by = xy[2 * i + 3];
      float dx = bx - ax, dy = by - ay;
      float len = sqrtf(dx * dx + dy * dy);
      if (len <= 0 || isnan(len)) continue;
      float nx = -dy / len * hw, ny = dx / len * hw;
      add_quad(ax + nx, ay + ny, bx + nx, by + ny, bx - nx, by - ny, ax - nx, ay - ny);
      if (i > 0) add_quad(ax - hw, ay, ax, ay - hw, ax + hw, ay, ax, ay + hw);
    }
    path(p.stroke_g, p.stroke_a);
  }
}

void Raster::hairline(const float* xy, int n, int g, int alpha) {
  if (alpha <= 0) return;
  int a8 = alpha == 255 ? 256 : alpha;
  float k = px_per_user_ * sub_;
  for (int i = 0; i + 1 < n; i++) {
    float ax = (xy[2 * i] - ux0_) * k, ay = xy[2 * i + 1] * k;
    float bx = (xy[2 * i + 2] - ux0_) * k, by = xy[2 * i + 3] * k;
    if ((ax < 0 && bx < 0) || (ax >= sw_ && bx >= sw_)) continue;
    float dx = bx - ax, dy = by - ay;
    float adx = fabsf(dx), ady = fabsf(dy);
    int steps = (int)ceilf(adx > ady ? adx : ady);
    if (steps <= 0 || steps > 100000) continue;
    float sx = dx / steps, sy = dy / steps;
    // Skip each segment's first sample after the first; the previous
    // segment ended there.
    for (int t = i ? 1 : 0; t <= steps; t++) {
      int x = (int)floorf(ax + sx * t);
      int y = (int)floorf(ay + sy * t);
      if (x < 0 || x >= sw_ || y < 0 || y >= sh_) continue;
      uint8_t* px = buf_ + (size_t)y * sw_ + x;
      int v = *px;
      *px = (uint8_t)(v + (((g - v) * a8) >> 8));
    }
  }
}

void Raster::end(uint8_t* gray) const {
  int area = sub_ * sub_;
  for (int y = 0; y < h_; y++) {
    for (int x = 0; x < w_; x++) {
      int sum = 0;
      for (int dy = 0; dy < sub_; dy++) {
        const uint8_t* row = buf_ + (size_t)(y * sub_ + dy) * sw_ + x * sub_;
        for (int dx = 0; dx < sub_; dx++) sum += row[dx];
      }
      gray[y * w_ + x] = (uint8_t)((sum + area / 2) / area);
    }
  }
}

// ================================================================ dither

void dither_tile(const uint8_t* gray, int w, int h, float* carry, uint8_t* bits,
                 int paper_cutoff, int threshold) {
  static float* px = nullptr;
  static size_t px_cap = 0;
  size_t need = (size_t)w * h;
  if (px_cap < need) {
    free(px);
    px = (float*)big_alloc(need * sizeof(float));
    px_cap = need;
  }
  for (size_t i = 0; i < need; i++) px[i] = gray[i] >= paper_cutoff ? 255.0f : gray[i];
  for (int y = 0; y < h; y++) px[y * w] += carry[y];
  std::vector<float> out(h, 0.0f);
  for (int y = 0; y < h; y++) {
    int row = y * w;
    for (int x = 0; x < w; x++) {
      int i = row + x;
      float old = px[i];
      float on = old >= threshold ? 255.0f : 0.0f;
      bits[i] = on == 255.0f ? 1 : 0;
      float err = old - on;
      if (x + 1 < w) px[i + 1] += (err * 7) / 16;
      else out[y] += (err * 7) / 16;
      if (y + 1 < h) {
        if (x > 0) px[i + w - 1] += (err * 3) / 16;
        px[i + w] += (err * 5) / 16;
        if (x + 1 < w) px[i + w + 1] += err / 16;
        else out[y + 1] += err / 16;
      }
    }
  }
  for (int y = 0; y < h; y++) carry[y] = out[y];
}

void skyline(const uint8_t* gray, int w, int h, int max_luma, int16_t* out) {
  for (int x = 0; x < w; x++) {
    int y = 0;
    for (; y < h; y++)
      if (gray[y * w + x] < max_luma) break;
    out[x] = (int16_t)y;
  }
}

// ================================================================ clouds

static const int CELL = 110;
static const uint8_t BAYER8[64] = {
    0,  32, 8,  40, 2,  34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44, 4,  36, 14, 46,
    6,  38, 60, 28, 52, 20, 62, 30, 54, 22, 3,  35, 11, 43, 1,  33, 9,  41, 51, 19, 59, 27,
    49, 17, 57, 25, 15, 47, 7,  39, 13, 45, 5,  37, 63, 31, 55, 23, 61, 29, 53, 21,
};

static uint32_t hash32(uint32_t n) {
  n = (n ^ (n >> 16)) * 0x7feb352du;
  n = (n ^ (n >> 15)) * 0x846ca68bu;
  return n ^ (n >> 16);
}

static float unit(uint32_t n) { return (float)(hash32(n) >> 8) * (1.0f / 16777216.0f); }

uint32_t cloud_salt(const char* seed) {
  uint32_t h = 2166136261u;
  for (const char* p = seed; *p; p++) {
    h ^= (uint8_t)*p;
    h *= 16777619u;
  }
  return h;
}

struct Cloud {
  float cx, cy, rx, ry, ox, oy, peak;
};

static bool cloud_for_cell(int64_t cell, uint32_t salt, Cloud* c) {
  uint32_t h = hash32((uint32_t)toInt32((double)cell * 2654435761.0) ^ salt);
  if (unit(h) > 0.8) return false;
  c->rx = 26 + unit(h ^ 0x11) * 34;
  c->ry = 8 + unit(h ^ 0x22) * 12;
  c->cx = cell * CELL + 12 + unit(h ^ 0x33) * (CELL - 24);
  c->cy = 10 + unit(h ^ 0x44) * 48;
  c->ox = (unit(h ^ 0x55) - 0.5) * c->rx * 0.7;
  c->oy = (unit(h ^ 0x66) - 0.5) * c->ry * 0.4;
  c->peak = 0.34 + unit(h ^ 0x77) * 0.22;
  return true;
}

static float lobe(float wx, float y, float cx, float cy, float rx, float ry) {
  float dx = (wx - cx) / rx;
  float dy = (y - cy) / ry;
  float d2 = dx * dx + dy * dy;
  if (d2 >= 1) return 0;
  float t = 1 - sqrtf(d2);
  return t * t;
}

void clouds(uint8_t* bits, int w, int h, int64_t origin_x, uint32_t salt, const int16_t* sky) {
  int64_t cell0 = (int64_t)floor((double)(origin_x - CELL) / CELL);
  int64_t cell1 = (int64_t)floor((double)(origin_x + w + CELL) / CELL);
  std::vector<Cloud> list;
  for (int64_t cell = cell0; cell <= cell1; cell++) {
    Cloud c;
    if (cloud_for_cell(cell, salt, &c)) list.push_back(c);
  }
  if (list.empty()) return;
  for (int x = 0; x < w; x++) {
    int top = sky[x];
    if (top < 4) continue;
    int64_t wx = origin_x + x;
    for (int y = 0; y < top; y++) {
      float density = 0;
      for (auto& c : list) {
        float a = lobe((float)wx, y, c.cx, c.cy, c.rx, c.ry);
        float b = lobe((float)wx, y, c.cx + c.ox, c.cy + c.oy, c.rx * 0.72f, c.ry * 0.8f);
        float d = c.peak * (a > b ? a : b);
        if (d > density) density = d;
      }
      if (density < 0.05) continue;
      float threshold = (BAYER8[((y & 7) << 3) + (int)(wx & 7)] + 0.5f) / 64;
      if (density > threshold) bits[y * w + x] = 0;
    }
  }
}

void pack_columns(const uint8_t* bits, int w, int h, uint8_t* out) {
  int stride = (h + 7) / 8;
  memset(out, 0, (size_t)w * stride);
  for (int x = 0; x < w; x++)
    for (int y = 0; y < h; y++)
      if (bits[y * w + x]) out[x * stride + (y >> 3)] |= (uint8_t)(1 << (7 - (y & 7)));
}

}  // namespace ss
