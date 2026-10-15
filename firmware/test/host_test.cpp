#include "protocol.h"
#include "st7305.h"
#include "strip.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

static int fail(const char* msg) {
  std::fprintf(stderr, "fail: %s\n", msg);
  return 1;
}

static void print_hex(const uint8_t* p, size_t n) {
  for (size_t i = 0; i < n; i++) std::printf("%02x", p[i]);
  std::printf("\n");
}

static int self_test() {
  const char* s = "123456789";
  if (ss_crc16(reinterpret_cast<const uint8_t*>(s), 9) != 0x29b1) return fail("crc");

  uint8_t fb[SS_FB_BYTES];
  st7305_fb_clear(fb);
  st7305_fb_set_pixel(fb, 0, 0, false);
  if (fb[74] != 0xfd) return fail("pixel 0,0");
  st7305_fb_set_pixel(fb, 1, 0, false);
  if (fb[74] != 0xfc) return fail("pixel 1,0");

  Strip strip;
  if (!strip_init(&strip, PANEL_W, SS_COL_STRIDE)) return fail("strip alloc");
  uint8_t col[SS_COL_STRIDE];
  std::memset(col, 0xff, sizeof col);
  col[0] &= (uint8_t)~0x80;
  if (!strip_append(&strip, col, 1)) return fail("append");
  for (int i = 1; i < PANEL_W; i++) {
    uint8_t white[SS_COL_STRIDE];
    std::memset(white, 0xff, sizeof white);
    if (!strip_append(&strip, white, 1)) return fail("append white");
  }
  if (!strip_covers(&strip, 0, PANEL_W)) return fail("covers");
  st7305_fb_clear(fb);
  strip_blit(&strip, 0, fb);
  if (fb[74] != 0xfd) return fail("blit");

  static uint8_t got[SS_FB_BYTES];
  static uint8_t expect[SS_FB_BYTES];
  Strip wrap;
  if (!strip_init(&wrap, PANEL_W, SS_COL_STRIDE)) return fail("wrap alloc");
  uint8_t painted[SS_COL_STRIDE];
  for (uint32_t x = 0; x < PANEL_W + 50; x++) {
    if (x == PANEL_W) strip_drop_before(&wrap, 50);
    std::memset(painted, 0xff, sizeof painted);
    int y = (int)(x % PANEL_H);
    painted[y >> 3] = (uint8_t)(painted[y >> 3] & ~(1u << (7 - (y & 7))));
    if (!strip_append(&wrap, painted, 1)) return fail("wrap append");
  }
  if (wrap.start_x != 50 || wrap.count != PANEL_W) return fail("wrap cursor");
  st7305_fb_clear(expect);
  for (int x = 0; x < PANEL_W; x++) {
    st7305_fb_set_pixel(expect, x, (int)((50 + x) % PANEL_H), false);
  }
  strip_blit(&wrap, 50, got);
  if (std::memcmp(got, expect, SS_FB_BYTES) != 0) return fail("blit match");
  std::printf("ok\n");
  return 0;
}

static int encode_hello() {
  uint8_t dst[64];
  size_t n = ss_encode_hello(dst, sizeof dst, 7);
  if (!n) return fail("encode");
  print_hex(dst, n);
  return 0;
}

static int decode_stdin() {
  std::vector<uint8_t> bytes;
  char buf[256];
  while (true) {
    std::size_t n = std::fread(buf, 1, sizeof buf, stdin);
    if (!n) break;
    bytes.insert(bytes.end(), buf, buf + n);
  }
  SsParser parser;
  ss_parser_init(&parser);
  size_t off = 0;
  SsFrame frame;
  bool got = false;
  while (off < bytes.size()) {
    if (parser.ready) {
      ss_parser_take(&parser, &frame);
      got = true;
      break;
    }
    size_t c = ss_parser_push(&parser, bytes.data() + off, bytes.size() - off);
    if (!c) break;
    off += c;
  }
  if (parser.ready) {
    ss_parser_take(&parser, &frame);
    got = true;
  }
  if (!got) return fail("no frame");
  std::printf("type=%u seq=%u len=%u\n", frame.type, frame.seq, frame.len);
  if (frame.type == SS_TYPE_COLUMNS && frame.len >= 10) {
    std::printf("start=%u columns=%u height=%u stride=%u\n", ss_read_u32(frame.payload),
                ss_read_u16(frame.payload + 4), ss_read_u16(frame.payload + 6),
                ss_read_u16(frame.payload + 8));
  }
  if (frame.type == SS_TYPE_HELLO && frame.len >= 12) {
    std::printf("w=%u h=%u bpp=%u\n", ss_read_u16(frame.payload), ss_read_u16(frame.payload + 2),
                ss_read_u16(frame.payload + 4));
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "encode") == 0) return encode_hello();
  if (argc > 1 && std::strcmp(argv[1], "decode") == 0) return decode_stdin();
  return self_test();
}
