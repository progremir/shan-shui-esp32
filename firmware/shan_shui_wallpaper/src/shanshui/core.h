#pragma once

// C++ port of shan-shui-inf (https://github.com/LingDong-/shan-shui-inf,
// Copyright (c) 2018 Lingdong Huang, MIT), pinned commit
// 9f754d2b2e73495db7883d4d4055a7b0903b0454.
//
// The port follows the JavaScript line by line so that, seeded the same way,
// it draws the same polylines. Things that matter for that:
//
// - Math.random() is rnd(). JavaScript evaluates operands left to right; C++
//   does not sequence function arguments or the two sides of `+`. Any
//   expression that calls rnd() (or anything that calls it) more than once
//   is split into separate statements in JavaScript order.
// - Numbers are double, as in JavaScript.
// - Drawing appends Prims to a Canvas in the order the JavaScript
//   concatenated SVG strings, which is also the paint order.
// - SS_SVG builds keep the exact SVG text for the reference test.

#include <math.h>
#include <stdint.h>

#include <functional>
#include <string>
#include <vector>

// The board build paints in float (see `real` below). Build it with
// -fsingle-precision-constant (firmware/build.sh does), or literals like 0.5
// drag the float math back into software double and painting is ~2x slower.
#if defined(ARDUINO) && !defined(SS_SVG)
#define SS_FLOAT
#endif

// Arduino.h defines PI as a macro; the port uses its own constant.
#ifdef PI
#undef PI
#endif

namespace ss {

// The reference test uses double, like JavaScript. The ESP32-S3 has a
// single-precision FPU and does double in software, so the board build sets
// SS_FLOAT; its painting then differs from the JavaScript in rounding only.
#ifdef SS_FLOAT
typedef float real;
#else
typedef double real;
#endif

struct Pt {
  real x = 0, y = 0;
  Pt() {}
  // A template so Pt{double, double} builds when real is float.
  template <class A, class B>
  Pt(A x_, B y_) : x((real)x_), y((real)y_) {}
  real& operator[](int i) { return i ? y : x; }
  real operator[](int i) const { return i ? y : x; }
};
typedef std::vector<Pt> Pts;
typedef std::function<real(real)> Fn;

static const real PI = 3.141592653589793;

#ifdef SS_FLOAT
// In float, sin(PI) is about -9e-8 rather than +1e-16, so the original's
// pow(sin(x * PI), 0.5) turns into NaN and half the shapes are lost. Inside
// namespace ss these hide the global pow/sqrt: a tiny negative base counts
// as zero.
inline real pow(real b, real e) {
  if (b < 0 && b > -1e-4f && e != floorf(e)) b = 0;
  return powf(b, e);
}
inline real sqrt(real v) { return sqrtf(v < 0 && v > -1e-4f ? 0 : v); }
#endif
static const real E = 2.718281828459045;

// ---------------------------------------------------------------- strings
// JavaScript number formatting, so colors and SVG text match byte for byte.
std::string jsnum(real v);              // String(v)
std::string fixed(real v, int digits);  // v.toFixed(digits)

// JavaScript number semantics.
int32_t toInt32(real v);              // v | 0, v >> 0
real jmin(real a, real b);            // Math.min: NaN if either is NaN
real jmax(real a, real b);            // Math.max
inline real jround(real v) { return floor(v + 0.5); }  // Math.round

// ---------------------------------------------------------------- colors
// A CSS color as the generator writes it. `gray` is luma 0..255, `a` 0..1.
struct Color {
  float gray = 0;
  float a = 0;
#ifdef SS_SVG
  std::string text;
#endif
  Color() {}
  Color(const char* css);
  Color(const std::string& css) : Color(css.c_str()) {}
  bool none() const { return a <= 0; }
};

// ---------------------------------------------------------------- canvas
// One SVG polyline, stored compactly: a scene can hold a million points and
// the board has 8 MB. The first point is stored as floats; each later point
// is a step from the previous one in quarter user units, one signed byte per
// axis, or an escape byte (-128) and two int16 for a long step. Most points
// cost 2 bytes.
struct Prim {
  uint32_t first = 0;  // byte offset in Canvas::data; prims are stored in order
  uint16_t n = 0;
  uint8_t wid16 = 0;   // stroke width in 1/16 user units
  uint8_t fill_g = 0, fill_a = 0, stroke_g = 0, stroke_a = 0;  // alpha 0..255
  float x0 = 0, y0 = 0;
  float minx = 0, maxx = 0;  // x extent including stroke width
  float wid() const { return wid16 * (1.0f / 16); }
};

struct Canvas {
  std::vector<Prim> prims;
  std::vector<int8_t> data;
#ifdef SS_SVG
  std::vector<std::string> svg_text;  // exact JS text per prim
#endif
  void add(Canvas&& other);
  bool empty() const { return prims.empty(); }
  void bounds(float* minx, float* maxx) const;
  void shrink();
  // Forget polylines that end left of x; returns how many were dropped.
  size_t drop_left(float x);
  size_t bytes() const { return prims.capacity() * sizeof(Prim) + data.capacity(); }
  size_t points() const;
  // Absolute user-unit points of prim i, x0 y0 x1 y1 ...
  void decode(size_t i, std::vector<float>& xy) const;
#ifdef SS_SVG
  std::string svg() const;
#endif
};

// ---------------------------------------------------------------- PRNG
void seed(const std::string& s);  // Math.seed(s) with a string seed
real rnd();                       // Math.random()

// ---------------------------------------------------------------- noise
real noise(real x, real y = 0, real z = 0);  // Noise.noise
void reset_noise();                          // perlin = null

// ---------------------------------------------------------------- PolyTools
Pt midPt(const Pts& plist);
Pt midPt(const Pt& a, const Pt& b);
struct TriArgs {
  real area = 100;
  bool convex = false;
  bool optimize = true;
};
std::vector<Pts> triangulate(Pts plist, const TriArgs& args = TriArgs());

// ---------------------------------------------------------------- Util
real distance(const Pt& a, const Pt& b);
real mapval(real value, real istart, real istop, real ostart, real ostop);
void loopNoise(std::vector<real>& nslist);
template <class T>
T randChoice(const std::vector<T>& arr) {
  return arr[(size_t)floor(arr.size() * rnd())];
}
real normRand(real m, real M);
real wtrand(const Fn& func);
real randGaussian();
Pts bezmh(Pts P, real w = 1);

struct PolyArgs {
  real xof = 0;
  real yof = 0;
  std::string fil = "rgba(0,0,0,0)";
  std::string str = "";  // empty: same as fil
  real wid = 0;
};
Canvas poly(const Pts& plist, const PolyArgs& args = PolyArgs());
extern size_t g_prims, g_nan_prims;  // polylines made, and dropped for NaN

struct StrokeArgs {
  real xof = 0;
  real yof = 0;
  real wid = 2;
  std::string col = "rgba(200,200,200,0.9)";
  real noi = 0.5;
  real out = 1;
  Fn fun;  // default sin(x*PI)
};
Canvas stroke(const Pts& ptlist, const StrokeArgs& args = StrokeArgs());

struct BlobArgs {
  real len = 20;
  real wid = 5;
  real ang = 0;
  std::string col = "rgba(200,200,200,0.9)";
  real noi = 0.5;
  Fn fun;  // default lobed sine
};
Canvas blob(real x, real y, const BlobArgs& args = BlobArgs());
Pts blobPts(real x, real y, const BlobArgs& args = BlobArgs());  // ret: 1

Pts div(const Pts& plist, real reso);

struct TextureArgs {
  real xof = 0;
  real yof = 0;
  real tex = 400;
  real wid = 1.5;
  real len = 0.2;
  real sha = 0;
  Fn noi;                                   // default 30/x
  std::function<std::string(real)> col;     // default random gray
  std::function<real()> dis;                // default two-band
};
Canvas texture(const std::vector<Pts>& ptlist, const TextureArgs& args = TextureArgs());
std::vector<Pts> textureList(const std::vector<Pts>& ptlist,
                             const TextureArgs& args = TextureArgs());  // ret: 1

// ---------------------------------------------------------------- water
struct WaterArgs {
  real hei = 2;
  real len = 800;
  real clu = 10;
};
Canvas water(real xoff, real yoff, real seed, const WaterArgs& args = WaterArgs());

}  // namespace ss
