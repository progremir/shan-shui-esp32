#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <charconv>

namespace ss {

// ================================================================ strings

std::string jsnum(real v) {
  if (isnan(v)) return "NaN";
  if (v == 0) return "0";
  if (isinf(v)) return v < 0 ? "-Infinity" : "Infinity";
  std::string sign = v < 0 ? "-" : "";
  if (v < 0) v = -v;
  // Shortest round-trip digits, then ECMAScript Number::toString layout.
  char buf[64];
  auto res = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::scientific);
  *res.ptr = 0;
  char* e = strchr(buf, 'e');
  int exp10 = atoi(e + 1);
  std::string digits;
  for (char* p = buf; p < e; p++)
    if (*p != '.') digits += *p;
  int k = (int)digits.size();
  int n = exp10 + 1;  // decimal point position
  std::string out;
  if (k <= n && n <= 21) {
    out = digits + std::string(n - k, '0');
  } else if (0 < n && n <= 21) {
    out = digits.substr(0, n) + "." + digits.substr(n);
  } else if (-6 < n && n <= 0) {
    out = "0." + std::string(-n, '0') + digits;
  } else {
    std::string ex = (n - 1 >= 0 ? "+" : "-") + std::to_string(abs(n - 1));
    out = digits.substr(0, 1) + (k > 1 ? "." + digits.substr(1) : "") + "e" + ex;
  }
  return sign + out;
}

std::string fixed(real v, int digits) {
  if (isnan(v)) return "NaN";
#ifndef SS_SVG
  // Only the board build gets here; colors need no exact rounding, and
  // printing a double exactly is slow without an FPU for it.
  if (fabs(v) < 1e9 && digits <= 6) {
    static const int64_t p10[] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    bool neg = v < 0;
    uint64_t q = (uint64_t)(fabsf((float)v) * (float)p10[digits] + 0.5f);
    char buf[32];
    char* e = buf + sizeof buf;
    char* p = e;
    for (int d = 0; d < digits; d++) {
      *--p = (char)('0' + q % 10);
      q /= 10;
    }
    if (digits) *--p = '.';
    do {
      *--p = (char)('0' + q % 10);
      q /= 10;
    } while (q);
    if (neg) *--p = '-';
    return std::string(p, e - p);
  }
#endif
  if (fabs(v) >= 1e21) return jsnum(v);
  bool neg = v < 0;
  if (neg) v = -v;
  // Exact decimal expansion, then round half up (JS picks the larger n).
  char buf[400];
  snprintf(buf, sizeof buf, "%.120f", v);
  std::string s(buf);
  size_t dot = s.find('.');
  std::string ip = s.substr(0, dot);
  std::string fp = s.substr(dot + 1);
  std::string keep = ip + fp.substr(0, digits);
  bool up = fp[digits] >= '5';
  if (up) {
    int i = (int)keep.size() - 1;
    while (i >= 0) {
      if (keep[i] == '9') {
        keep[i] = '0';
        i--;
      } else {
        keep[i]++;
        break;
      }
    }
    if (i < 0) keep = "1" + keep;
  }
  std::string out = keep.substr(0, keep.size() - digits);
  if (digits > 0) out += "." + keep.substr(keep.size() - digits);
  return (neg ? "-" : "") + out;
}

real jmin(real a, real b) { return (isnan(a) || isnan(b)) ? NAN : (a < b ? a : b); }
real jmax(real a, real b) { return (isnan(a) || isnan(b)) ? NAN : (a > b ? a : b); }

// ================================================================ colors

Color::Color(const char* css) {
#ifdef SS_SVG
  text = css;
#endif
  float r = 0, g = 0, b = 0, al = 1;
  if (!strncmp(css, "rgba(", 5) || !strncmp(css, "rgb(", 4)) {
    // strtof is far cheaper than sscanf on the board.
    const char* p = css + (css[3] == 'a' ? 5 : 4);
    float* out[4] = {&r, &g, &b, &al};
    int want = css[3] == 'a' ? 4 : 3;
    for (int i = 0; i < want; i++) {
      char* end;
      *out[i] = strtof(p, &end);
      if (end == p) {
        al = 0;
        break;
      }
      p = end + 1;
    }
  } else if (!strcmp(css, "white")) {
    r = g = b = 255;
  } else if (!strcmp(css, "black")) {
    r = g = b = 0;
  } else if (!strcmp(css, "red")) {
    r = 255;
  } else if (!strcmp(css, "green")) {
    g = 128;
  } else {
    al = 0;  // "none" and anything unknown
  }
  // An unparsable color (say, NaN from float rounding) is black in a
  // browser; on the board it is better left out.
  if (isnan(r) || isnan(g) || isnan(b)) al = 0;
  if (!(al > 0)) al = 0;
  if (al > 1) al = 1;
  gray = 0.299f * r + 0.587f * g + 0.114f * b;
  a = al;
}

// ================================================================ canvas

void Canvas::add(Canvas&& other) {
  uint32_t base = (uint32_t)data.size();
  data.insert(data.end(), other.data.begin(), other.data.end());
  for (auto& p : other.prims) {
    prims.push_back(p);
    prims.back().first += base;
  }
#ifdef SS_SVG
  for (auto& t : other.svg_text) svg_text.push_back(std::move(t));
  other.svg_text.clear();
#endif
  other.prims.clear();
  other.data.clear();
}

void Canvas::bounds(float* minx, float* maxx) const {
  float lo = INFINITY, hi = -INFINITY;
  for (auto& p : prims) {
    if (p.minx < lo) lo = p.minx;
    if (p.maxx > hi) hi = p.maxx;
  }
  *minx = lo;
  *maxx = hi;
}

void Canvas::shrink() {
  prims.shrink_to_fit();
  data.shrink_to_fit();
}

size_t Canvas::points() const {
  size_t n = 0;
  for (auto& p : prims) n += p.n;
  return n;
}

size_t Canvas::drop_left(float x) {
  size_t keep = 0, out = 0;
  for (size_t i = 0; i < prims.size(); i++) {
    Prim p = prims[i];
    size_t end = i + 1 < prims.size() ? prims[i + 1].first : data.size();
    if (p.maxx < x) continue;
    size_t len = end - p.first;
    memmove(data.data() + out, data.data() + p.first, len);
    p.first = (uint32_t)out;
    out += len;
#ifdef SS_SVG
    svg_text[keep] = std::move(svg_text[i]);
#endif
    prims[keep++] = p;
  }
  size_t dropped = prims.size() - keep;
  if (dropped) {
    prims.resize(keep);
    data.resize(out);
#ifdef SS_SVG
    svg_text.resize(keep);
#endif
    shrink();
  }
  return dropped;
}

void Canvas::decode(size_t i, std::vector<float>& xy) const {
  const Prim& p = prims[i];
  xy.resize((size_t)p.n * 2);
  if (!p.n) return;
  xy[0] = p.x0;
  xy[1] = p.y0;
  const int8_t* src = data.data() + p.first;
  int32_t qx = 0, qy = 0;
  for (uint32_t j = 1; j < p.n; j++) {
    if (*src == -128) {
      int16_t dx, dy;
      memcpy(&dx, src + 1, 2);
      memcpy(&dy, src + 3, 2);
      qx += dx;
      qy += dy;
      src += 5;
    } else {
      qx += src[0];
      qy += src[1];
      src += 2;
    }
    xy[2 * j] = p.x0 + qx * 0.25f;
    xy[2 * j + 1] = p.y0 + qy * 0.25f;
  }
}

#ifdef SS_SVG
std::string Canvas::svg() const {
  std::string s;
  for (auto& t : svg_text) s += t;
  return s;
}
#endif

// ================================================================ PRNG

static double prng_s = 1234;
static const double prng_p = 999979;
static const double prng_q = 999983;
static const double prng_m = prng_p * prng_q;

static std::string base64(const std::string& in) {
  static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  size_t i = 0;
  for (; i + 2 < in.size(); i += 3) {
    uint32_t v = ((uint8_t)in[i] << 16) | ((uint8_t)in[i + 1] << 8) | (uint8_t)in[i + 2];
    out += tbl[(v >> 18) & 63];
    out += tbl[(v >> 12) & 63];
    out += tbl[(v >> 6) & 63];
    out += tbl[v & 63];
  }
  if (i + 1 == in.size()) {
    uint32_t v = (uint8_t)in[i] << 16;
    out += tbl[(v >> 18) & 63];
    out += tbl[(v >> 12) & 63];
    out += "==";
  } else if (i + 2 == in.size()) {
    uint32_t v = ((uint8_t)in[i] << 16) | ((uint8_t)in[i + 1] << 8);
    out += tbl[(v >> 18) & 63];
    out += tbl[(v >> 12) & 63];
    out += tbl[(v >> 6) & 63];
    out += "=";
  }
  return out;
}

static double prng_hash(const std::string& x) {
  // window.btoa(JSON.stringify(x)) for a plain string.
  std::string json = "\"";
  for (char c : x) {
    if (c == '"' || c == '\\') json += '\\';
    json += c;
  }
  json += "\"";
  std::string y = base64(json);
  double z = 0;
  for (size_t i = 0; i < y.size(); i++) z += (double)(uint8_t)y[i] * pow(128.0, (double)i);
  return z;
}

#ifdef SS_FLOAT
// The board does not reproduce the JavaScript bit for bit anyway (float), and
// the original's double-precision square-mod costs ~6 us without a double
// FPU. xorshift32, seeded from the same string hash, is ~50x cheaper.
static uint32_t xs_state = 2463534242u;
real rnd() {
  uint32_t x = xs_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  xs_state = x;
  return (real)(x >> 8) * (1.0f / 16777216.0f);
}
#else
real rnd() {
  prng_s = fmod(prng_s * prng_s, prng_m);
  return prng_s / prng_m;
}
#endif

void seed(const std::string& s) {
  double y = 0;
  double z = 0;
  double h = prng_hash(s);
  while (fmod(y, prng_p) == 0 || fmod(y, prng_q) == 0 || y == 0 || y == 1) {
    y = fmod(h + z, prng_m);
    z += 1;
  }
  prng_s = y;
#ifdef SS_FLOAT
  xs_state = (uint32_t)fmod(y, 4294967291.0) | 1u;
#endif
  for (int i = 0; i < 10; i++) rnd();
  reset_noise();
}

// ================================================================ noise

static const int PERLIN_YWRAPB = 4;
static const int PERLIN_YWRAP = 1 << PERLIN_YWRAPB;
static const int PERLIN_ZWRAPB = 8;
static const int PERLIN_ZWRAP = 1 << PERLIN_ZWRAPB;
static const int PERLIN_SIZE = 4095;
static const int perlin_octaves = 4;
static const real perlin_amp_falloff = 0.5;
static real* perlin = nullptr;

void reset_noise() {
  free(perlin);
  perlin = nullptr;
}

int32_t toInt32(real v) {
  if (!isfinite(v)) return 0;
  if (v > -2147483648.0 && v < 2147483648.0) return (int32_t)v;
  double m = fmod(trunc(v), 4294967296.0);
  if (m < 0) m += 4294967296.0;
  return (int32_t)(uint32_t)m;
}

#ifdef SS_FLOAT
// noise() calls this 12 times; cosf is ~1 us on the board. i is in [0, 1).
static real scaled_cosine(real i) {
  static float table[257];
  static bool ready = false;
  if (!ready) {
    for (int k = 0; k <= 256; k++) table[k] = 0.5f * (1.0f - cosf(k * (3.14159265f / 256)));
    ready = true;
  }
  float f = i * 256;
  int k = (int)f;
  if (k < 0) k = 0;
  if (k > 255) k = 255;
  float t = f - k;
  return table[k] + (table[k + 1] - table[k]) * t;
}
#else
static real scaled_cosine(real i) { return 0.5 * (1.0 - cos(i * PI)); }
#endif

real noise(real x, real y, real z) {
  if (perlin == nullptr) {
    perlin = (real*)malloc(sizeof(real) * (PERLIN_SIZE + 1));
    for (int i = 0; i < PERLIN_SIZE + 1; i++) perlin[i] = rnd();
  }
  if (x < 0) x = -x;
  if (y < 0) y = -y;
  if (z < 0) z = -z;
  int32_t xi = toInt32(floor(x)), yi = toInt32(floor(y)), zi = toInt32(floor(z));
  real xf = x - floor(x);
  real yf = y - floor(y);
  real zf = z - floor(z);
  real rxf, ryf;
  real r = 0;
  real ampl = 0.5;
  real n1, n2, n3;
  for (int o = 0; o < perlin_octaves; o++) {
    int32_t of = (int32_t)((uint32_t)xi + ((uint32_t)yi << PERLIN_YWRAPB) +
                           ((uint32_t)zi << PERLIN_ZWRAPB));
    rxf = scaled_cosine(xf);
    ryf = scaled_cosine(yf);
    n1 = perlin[of & PERLIN_SIZE];
    n1 += rxf * (perlin[(of + 1) & PERLIN_SIZE] - n1);
    n2 = perlin[(of + PERLIN_YWRAP) & PERLIN_SIZE];
    n2 += rxf * (perlin[(of + PERLIN_YWRAP + 1) & PERLIN_SIZE] - n2);
    n1 += ryf * (n2 - n1);
    of += PERLIN_ZWRAP;
    n2 = perlin[of & PERLIN_SIZE];
    n2 += rxf * (perlin[(of + 1) & PERLIN_SIZE] - n2);
    n3 = perlin[(of + PERLIN_YWRAP) & PERLIN_SIZE];
    n3 += rxf * (perlin[(of + PERLIN_YWRAP + 1) & PERLIN_SIZE] - n3);
    n2 += ryf * (n3 - n2);
    n1 += scaled_cosine(zf) * (n2 - n1);
    r += n1 * ampl;
    ampl *= perlin_amp_falloff;
    xi = (int32_t)((uint32_t)xi << 1);
    xf *= 2;
    yi = (int32_t)((uint32_t)yi << 1);
    yf *= 2;
    zi = (int32_t)((uint32_t)zi << 1);
    zf *= 2;
    if (xf >= 1.0) {
      xi++;
      xf--;
    }
    if (yf >= 1.0) {
      yi++;
      yf--;
    }
    if (zf >= 1.0) {
      zi++;
      zf--;
    }
  }
  return r;
}

// ================================================================ PolyTools

Pt midPt(const Pts& plist) {
  real ax = 0, ay = 0;
  real n = (real)plist.size();
  for (auto& v : plist) {
    ax = v.x / n + ax;
    ay = v.y / n + ay;
  }
  return Pt(ax, ay);
}

Pt midPt(const Pt& a, const Pt& b) { return midPt(Pts{a, b}); }

namespace {

struct Line {
  Pt a, b;
};

void lineExpr(const Pt& pt0, const Pt& pt1, real* m, real* k) {
  real den = pt1.x - pt0.x;
  *m = den == 0 ? INFINITY : (pt1.y - pt0.y) / den;
  *k = pt0.y - *m * pt0.x;
}


bool onSegJs(const Pt& p, const Line& ln) {
  return jmin(ln.a.x, ln.b.x) <= p.x && p.x <= jmax(ln.a.x, ln.b.x) &&
         jmin(ln.a.y, ln.b.y) <= p.y && p.y <= jmax(ln.a.y, ln.b.y);
}

bool intersect(const Line& ln0, const Line& ln1) {
  real m0, k0, m1, k1;
  lineExpr(ln0.a, ln0.b, &m0, &k0);
  lineExpr(ln1.a, ln1.b, &m1, &k1);
  real den = m0 - m1;
  if (den == 0) return false;
  real x = (k1 - k0) / den;
  real y = m0 * x + k0;
  Pt p(x, y);
  return onSegJs(p, ln0) && onSegJs(p, ln1);
}

bool ptInPoly(const Pt& pt, const Pts& plist) {
  int scount = 0;
  for (size_t i = 0; i < plist.size(); i++) {
    const Pt& np = plist[i != plist.size() - 1 ? i + 1 : 0];
    if (intersect(Line{plist[i], np}, Line{pt, Pt(pt.x + 999, pt.y + 999)})) scount++;
  }
  return scount % 2 == 1;
}

bool lnInPoly(const Line& ln, const Pts& plist) {
  real ep = 0.01;
  Line lnc;
  lnc.a.x = ln.a.x * (1 - ep) + ln.b.x * ep;
  lnc.a.y = ln.a.y * (1 - ep) + ln.b.y * ep;
  lnc.b.x = ln.a.x * ep + ln.b.x * (1 - ep);
  lnc.b.y = ln.a.y * ep + ln.b.y * (1 - ep);
  for (size_t i = 0; i < plist.size(); i++) {
    const Pt& pt = plist[i];
    const Pt& np = plist[i != plist.size() - 1 ? i + 1 : 0];
    if (intersect(lnc, Line{pt, np})) return false;
  }
  Pt mid = midPt(Pts{ln.a, ln.b});
  if (!ptInPoly(mid, plist)) return false;
  return true;
}

std::vector<real> sidesOf(const Pts& plist) {
  std::vector<real> slist;
  for (size_t i = 0; i < plist.size(); i++) {
    const Pt& pt = plist[i];
    const Pt& np = plist[i != plist.size() - 1 ? i + 1 : 0];
    slist.push_back(sqrt(pow(np.x - pt.x, 2) + pow(np.y - pt.y, 2)));
  }
  return slist;
}

real areaOf(const Pts& plist) {
  auto slist = sidesOf(plist);
  real a = slist[0], b = slist.size() > 1 ? slist[1] : NAN, c = slist.size() > 2 ? slist[2] : NAN;
  real s = (a + b + c) / 2;
  return sqrt(s * (s - a) * (s - b) * (s - c));
}

real sliverRatio(const Pts& plist) {
  real A = areaOf(plist);
  real P = 0;
  for (real v : sidesOf(plist)) P = P + v;
  return A / P;
}

struct Cut {
  Pts ear, rest;
};

Cut bestEar(const Pts& plist, const TriArgs& args) {
  std::vector<Cut> cuts;
  for (size_t i = 0; i < plist.size(); i++) {
    const Pt& pt = plist[i];
    const Pt& lp = plist[i != 0 ? i - 1 : plist.size() - 1];
    const Pt& np = plist[i != plist.size() - 1 ? i + 1 : 0];
    Pts qlist = plist;
    qlist.erase(qlist.begin() + i);
    if (args.convex || lnInPoly(Line{lp, np}, plist)) {
      Cut c{Pts{lp, pt, np}, qlist};
      if (!args.optimize) return c;
      cuts.push_back(c);
    }
  }
  Cut best{plist, Pts{}};
  real bestRatio = 0;
  for (auto& c : cuts) {
    real r = sliverRatio(c.ear);
    if (r >= bestRatio) {
      best = c;
      bestRatio = r;
    }
  }
  return best;
}

void shatter(const Pts& plist, real a, std::vector<Pts>& out, int depth) {
  if (plist.empty()) return;
  // A degenerate triangle has a NaN area, which never tests smaller than
  // `a`: JavaScript would split it forever (and float rounding makes such
  // slivers common on the board). Treat it as small.
  real area = areaOf(plist);
  if (isnan(area) || area < a || depth > 24) {
    out.push_back(plist);
    return;
  }
  auto slist = sidesOf(plist);
  size_t ind = 0;
  for (size_t i = 0; i < slist.size(); i++)
    if (slist[i] > slist[ind]) ind = i;
  size_t nind = (ind + 1) % plist.size();
  size_t lind = (ind + 2) % plist.size();
  Pt mid = midPt(Pts{plist[ind], plist[nind]});
  shatter(Pts{plist[ind], mid, plist[lind]}, a, out, depth + 1);
  shatter(Pts{plist[lind], plist[nind], mid}, a, out, depth + 1);
}

}  // namespace

std::vector<Pts> triangulate(Pts plist, const TriArgs& args) {
  std::vector<Pts> out;
  while (true) {
    if (plist.size() <= 3) {
      shatter(plist, args.area, out, 0);
      return out;
    }
    Cut cut = bestEar(plist, args);
    shatter(cut.ear, args.area, out, 0);
    plist = cut.rest;
  }
}

// ================================================================ Util

real distance(const Pt& p0, const Pt& p1) {
  return sqrt(pow(p0.x - p1.x, 2) + pow(p0.y - p1.y, 2));
}

real mapval(real value, real istart, real istop, real ostart, real ostop) {
  return ostart + (ostop - ostart) * (((value - istart) * 1.0) / (istop - istart));
}

void loopNoise(std::vector<real>& nslist) {
  real dif = nslist[nslist.size() - 1] - nslist[0];
  real bds[2] = {100, -100};
  real n = (real)nslist.size();
  for (size_t i = 0; i < nslist.size(); i++) {
    nslist[i] += (dif * (n - 1 - i)) / (n - 1);
    if (nslist[i] < bds[0]) bds[0] = nslist[i];
    if (nslist[i] > bds[1]) bds[1] = nslist[i];
  }
  for (size_t i = 0; i < nslist.size(); i++) nslist[i] = mapval(nslist[i], bds[0], bds[1], 0, 1);
}

real normRand(real m, real M) { return mapval(rnd(), 0, 1, m, M); }

real wtrand(const Fn& func) {
  while (true) {
    real x = rnd();
    real y = rnd();
    if (y < func(x)) return x;
  }
}

real randGaussian() {
  return wtrand([](real x) { return pow(E, -24 * pow(x - 0.5, 2)); }) * 2 - 1;
}

Pts bezmh(Pts P, real w) {
  if (P.size() == 2) P = Pts{P[0], midPt(P[0], P[1]), P[1]};
  Pts plist;
  for (int j = 0; j < (int)P.size() - 2; j++) {
    Pt p0, p1, p2;
    if (j == 0) p0 = P[j];
    else p0 = midPt(P[j], P[j + 1]);
    p1 = P[j + 1];
    if (j == (int)P.size() - 3) p2 = P[j + 2];
    else p2 = midPt(P[j + 1], P[j + 2]);
    int pl = 20;
    for (int i = 0; i < pl + (j == (int)P.size() - 3 ? 1 : 0); i += 1) {
      real t = (real)i / pl;
      real u = pow(1 - t, 2) + 2 * t * (1 - t) * w + t * t;
      plist.push_back(Pt((pow(1 - t, 2) * p0.x + 2 * t * (1 - t) * p1.x * w + t * t * p2.x) / u,
                         (pow(1 - t, 2) * p0.y + 2 * t * (1 - t) * p1.y * w + t * t * p2.y) / u));
    }
  }
  return plist;
}

size_t g_nan_prims = 0;
size_t g_prims = 0;

static uint8_t alpha8(float a) { return (uint8_t)(a * 255 + 0.5f); }

Canvas poly(const Pts& plist, const PolyArgs& args) {
  Canvas c;
  g_prims++;
  const std::string& str = args.str.empty() ? args.fil : args.str;
  Color fill(args.fil), strokec(str);
  Prim p;
  p.fill_g = (uint8_t)(fill.gray + 0.5f);
  p.fill_a = alpha8(fill.a);
  p.stroke_g = (uint8_t)(strokec.gray + 0.5f);
  p.stroke_a = args.wid > 0 ? alpha8(strokec.a) : 0;
#ifdef SS_SVG
  std::string t = "<polyline points='";
#endif
  // chunkloader() rewrites NaN to -1000 in the SVG text.
  std::vector<float> xy;
  xy.reserve(plist.size() * 2);
  float lo = INFINITY, hi = -INFINITY;
  for (size_t i = 0; i < plist.size(); i++) {
    real x = plist[i].x + args.xof;
    real y = plist[i].y + args.yof;
#ifdef SS_SVG
    t += " ";
    t += isnan(x) ? "-1000" : fixed(x, 1);
    t += ",";
    t += isnan(y) ? "-1000" : fixed(y, 1);
    if (!isfinite(x)) x = -1000;
    if (!isfinite(y)) y = -1000;
#else
    // Float rounding on the board makes the odd NaN, from a sliver triangle
    // or similar; JavaScript would draw it to -1000 (and color it black).
    // The shape is garbage, so drop it.
    if (!isfinite(x) || !isfinite(y)) {
      g_nan_prims++;
      return c;
    }
#endif
    xy.push_back((float)x);
    xy.push_back((float)y);
    if (x < lo) lo = (float)x;
    if (x > hi) hi = (float)x;
  }
#ifdef SS_SVG
  t += "' style='fill:" + fill.text + ";stroke:" + strokec.text + ";stroke-width:" +
       jsnum(args.wid) + "'/>";
  c.svg_text.push_back(std::move(t));
#endif
  size_t n = xy.size() / 2;
  if (n > 65535) n = 65535;
  p.n = (uint16_t)n;
  if (n) {
    p.x0 = xy[0];
    p.y0 = xy[1];
  }
  float w16 = args.wid * 16 + 0.5f;
  p.wid16 = (uint8_t)(w16 < 0 ? 0 : w16 > 255 ? 255 : w16);
  c.data.reserve(n * 2);
  int32_t px = 0, py = 0;
  for (size_t i = 1; i < n; i++) {
    int32_t qx = (int32_t)lroundf((xy[2 * i] - p.x0) * 4);
    int32_t qy = (int32_t)lroundf((xy[2 * i + 1] - p.y0) * 4);
    int32_t dx = qx - px, dy = qy - py;
    if (dx >= -127 && dx <= 127 && dy >= -127 && dy <= 127) {
      c.data.push_back((int8_t)dx);
      c.data.push_back((int8_t)dy);
    } else {
      dx = std::max<int32_t>(-32767, std::min<int32_t>(32767, dx));
      dy = std::max<int32_t>(-32767, std::min<int32_t>(32767, dy));
      int16_t d16[2] = {(int16_t)dx, (int16_t)dy};
      c.data.push_back(-128);
      const int8_t* b = (const int8_t*)d16;
      c.data.insert(c.data.end(), b, b + 4);
    }
    px += dx;
    py += dy;
  }
  if (!n) lo = hi = 0;
  p.minx = lo - p.wid();
  p.maxx = hi + p.wid();
  c.prims.push_back(p);
  return c;
}

Canvas stroke(const Pts& ptlist, const StrokeArgs& args) {
  if (ptlist.empty()) return Canvas();
  Pts vtxlist0, vtxlist1;
  real n0 = rnd() * 10;
  size_t n = ptlist.size();
  for (size_t i = 1; i + 1 < n; i++) {
    real f = args.fun ? args.fun((real)i / n) : sin(((real)i / n) * PI);
    real w = args.wid * f;
    w = w * (1 - args.noi) + w * args.noi * noise(i * 0.5, n0);
    real a1 = atan2(ptlist[i].y - ptlist[i - 1].y, ptlist[i].x - ptlist[i - 1].x);
    real a2 = atan2(ptlist[i].y - ptlist[i + 1].y, ptlist[i].x - ptlist[i + 1].x);
    real a = (a1 + a2) / 2;
    if (a < a2) a += PI;
    vtxlist0.push_back(Pt(ptlist[i].x + w * cos(a), ptlist[i].y + w * sin(a)));
    vtxlist1.push_back(Pt(ptlist[i].x - w * cos(a), ptlist[i].y - w * sin(a)));
  }
  Pts vtxlist;
  vtxlist.push_back(ptlist[0]);
  for (auto& v : vtxlist0) vtxlist.push_back(v);
  vtxlist.push_back(ptlist[n - 1]);
  for (size_t i = vtxlist1.size(); i-- > 0;) vtxlist.push_back(vtxlist1[i]);
  vtxlist.push_back(ptlist[0]);
  for (auto& v : vtxlist) {
    v.x += args.xof;
    v.y += args.yof;
  }
  PolyArgs pa;
  pa.fil = args.col;
  pa.str = args.col;
  pa.wid = args.out;
  return poly(vtxlist, pa);
}

Pts blobPts(real x, real y, const BlobArgs& args) {
  real reso = 20.0;
  std::vector<Pt> lalist;
  for (int i = 0; i < reso + 1; i++) {
    real p = (i / reso) * 2;
    real xo = args.len / 2 - fabs(p - 1) * args.len;
    real f;
    if (args.fun) f = args.fun(p);
    else f = p <= 1 ? pow(sin(p * PI), 0.5) : -pow(sin((p + 1) * PI), 0.5);
    real yo = (f * args.wid) / 2;
    real a = atan2(yo, xo);
    real l = sqrt(xo * xo + yo * yo);
    lalist.push_back(Pt(l, a));
  }
  std::vector<real> nslist;
  real n0 = rnd() * 10;
  for (int i = 0; i < reso + 1; i++) nslist.push_back(noise(i * 0.05, n0));
  loopNoise(nslist);
  Pts plist;
  for (size_t i = 0; i < lalist.size(); i++) {
    real ns = nslist[i] * args.noi + (1 - args.noi);
    real nx = x + cos(lalist[i].y + args.ang) * lalist[i].x * ns;
    real ny = y + sin(lalist[i].y + args.ang) * lalist[i].x * ns;
    plist.push_back(Pt(nx, ny));
  }
  return plist;
}

Canvas blob(real x, real y, const BlobArgs& args) {
  Pts plist = blobPts(x, y, args);
  PolyArgs pa;
  pa.fil = args.col;
  pa.str = args.col;
  pa.wid = 0;
  return poly(plist, pa);
}

Pts div(const Pts& plist, real reso) {
  real tl = (plist.size() - 1.0) * reso;
  Pts rlist;
  for (real i = 0; i < tl; i += 1) {
    const Pt& lastp = plist[(size_t)floor(i / reso)];
    const Pt& nextp = plist[(size_t)ceil(i / reso)];
    real p = fmod(i, reso) / reso;
    real nx = lastp.x * (1 - p) + nextp.x * p;
    real ny = lastp.y * (1 - p) + nextp.y * p;
    rlist.push_back(Pt(nx, ny));
  }
  if (!plist.empty()) rlist.push_back(plist[plist.size() - 1]);
  return rlist;
}

std::vector<Pts> textureList(const std::vector<Pts>& ptlist, const TextureArgs& args) {
  size_t reso0 = ptlist.size();
  size_t reso1 = ptlist[0].size();
  std::vector<Pts> texlist;
  for (int i = 0; i < args.tex; i++) {
    real d;
    if (args.dis) {
      d = args.dis();
    } else if (rnd() > 0.5) {
      d = (1.0 / 3) * rnd();
    } else {
      d = (1.0 * 2) / 3 + (1.0 / 3) * rnd();
    }
    int mid = toInt32(d * reso1);
    int hlen = (int)floor(rnd() * (reso1 * args.len));
    int start = mid - hlen;
    int end = mid + hlen;
    start = std::min(std::max(start, 0), (int)reso1);
    end = std::min(std::max(end, 0), (int)reso1);
    real layer = (i / args.tex) * (reso0 - 1.0);
    texlist.push_back(Pts());
    for (int j = start; j < end; j++) {
      real p = layer - floor(layer);
      const Pts& lo = ptlist[(size_t)floor(layer)];
      const Pts& hi = ptlist[(size_t)ceil(layer)];
      real x = lo[j].x * p + hi[j].x * (1 - p);
      real y = lo[j].y * p + hi[j].y * (1 - p);
      real nz = args.noi ? args.noi(layer + 1) : 30 / (layer + 1);
      real ns0 = nz * (noise(x, j * 0.5) - 0.5);
      real ns1 = nz * (noise(y, j * 0.5) - 0.5);
      texlist.back().push_back(Pt(x + ns0, y + ns1));
    }
  }
  return texlist;
}

Canvas texture(const std::vector<Pts>& ptlist, const TextureArgs& args) {
  std::vector<Pts> texlist = textureList(ptlist, args);
  Canvas canv;
  int sha = (int)args.sha;
  auto shifted = [&](const Pts& src) {
    Pts out = src;
    for (auto& v : out) {
      v.x += args.xof;
      v.y += args.yof;
    }
    return out;
  };
  if (sha) {
    for (size_t j = 0; j < texlist.size(); j += 1 + (sha != 0)) {
      StrokeArgs sa;
      sa.col = "rgba(100,100,100,0.1)";
      sa.wid = sha;
      canv.add(stroke(shifted(texlist[j]), sa));
    }
  }
  for (size_t j = 0 + sha; j < texlist.size(); j += 1 + sha) {
    StrokeArgs sa;
    if (args.col) sa.col = args.col((real)j / texlist.size());
    else sa.col = "rgba(100,100,100," + fixed(rnd() * 0.3, 3) + ")";
    sa.wid = args.wid;
    canv.add(stroke(shifted(texlist[j]), sa));
  }
  return canv;
}

// ================================================================ water

Canvas water(real xoff, real yoff, real seed_, const WaterArgs& args) {
  (void)seed_;
  Canvas canv;
  std::vector<Pts> ptlist;
  real yk = 0;
  for (int i = 0; i < args.clu; i++) {
    ptlist.push_back(Pts());
    real xk = (rnd() - 0.5) * (args.len / 8);
    yk += rnd() * 5;
    real lk = args.len / 4 + rnd() * (args.len / 4);
    real reso = 5;
    for (real j = -lk; j < lk; j += reso) {
      ptlist.back().push_back(Pt(j + xk, sin(j * 0.2) * args.hei * noise(j * 0.1) - 20 + yk));
    }
  }
  for (size_t j = 1; j < ptlist.size(); j += 1) {
    Pts pts = ptlist[j];
    for (auto& v : pts) {
      v.x += xoff;
      v.y += yoff;
    }
    StrokeArgs sa;
    sa.col = "rgba(100,100,100," + fixed(0.3 + rnd() * 0.3, 3) + ")";
    sa.wid = 1;
    canv.add(stroke(pts, sa));
  }
  return canv;
}

}  // namespace ss
