#include "strip.h"

#include "st7305.h"

#include <stdlib.h>
#include <string.h>

#if !defined(HOST_TEST)
#include <Arduino.h>
#endif

bool strip_init(Strip* strip, uint16_t capacity, uint16_t stride) {
  memset(strip, 0, sizeof(*strip));
  strip->capacity = capacity;
  strip->stride = stride;
#if defined(HOST_TEST)
  strip->data = (uint8_t*)malloc((size_t)capacity * stride);
#else
  strip->data = (uint8_t*)ps_malloc((size_t)capacity * stride);
#endif
  return strip->data != nullptr;
}

void strip_reset(Strip* strip) {
  strip->count = 0;
  strip->head = 0;
  strip->start_x = 0;
}

bool strip_covers(const Strip* strip, uint32_t x, uint16_t n) {
  if (!strip->count || x < strip->start_x) return false;
  uint32_t end = strip->start_x + strip->count;
  return x + n <= end;
}

uint16_t strip_free(const Strip* strip) {
  return (uint16_t)(strip->capacity - strip->count);
}

bool strip_append(Strip* strip, const uint8_t* packed, uint16_t columns) {
  if ((uint32_t)strip->count + columns > strip->capacity) return false;
  for (uint16_t i = 0; i < columns; i++) {
    uint16_t slot = (uint16_t)((strip->head + strip->count) % strip->capacity);
    memcpy(strip->data + (size_t)slot * strip->stride, packed + (size_t)i * strip->stride,
           strip->stride);
    strip->count++;
  }
  return true;
}

void strip_drop_before(Strip* strip, uint32_t x) {
  while (strip->count && strip->start_x < x) {
    strip->head = (uint16_t)((strip->head + 1) % strip->capacity);
    strip->count--;
    strip->start_x++;
  }
}

static bool column_white(const uint8_t* col, int y) {
  return ((col[y >> 3] >> (7 - (y & 7))) & 1) != 0;
}

static const uint8_t* column_at(const Strip* strip, uint32_t abs_x) {
  uint32_t rel = abs_x - strip->start_x;
  uint16_t slot = (uint16_t)((strip->head + rel) % strip->capacity);
  return strip->data + (size_t)slot * strip->stride;
}

// One ST7305 byte is 2 columns by 4 rows. Writing those bytes directly keeps
// a 24 px/s pan from spending the whole core inside per-pixel calls, which
// was starving USB until a frame got cut off and the scroll never resumed.
void strip_blit(const Strip* strip, uint32_t view_x, uint8_t* fb) {
  st7305_fb_clear(fb);
  const int blocks = PANEL_H / 4;
  for (int byte_x = 0; byte_x < PANEL_W / 2; byte_x++) {
    const uint8_t* even = column_at(strip, view_x + (uint32_t)byte_x * 2);
    const uint8_t* odd = column_at(strip, view_x + (uint32_t)byte_x * 2 + 1);
    uint8_t* out = fb + byte_x * blocks;
    for (int block = 0; block < blocks; block++) {
      uint8_t bits = 0xff;
      for (int r = 0; r < 4; r++) {
        int y = PANEL_H - 1 - (block * 4 + r);
        if (!column_white(even, y)) bits = (uint8_t)(bits & ~(1u << (7 - 2 * r)));
        if (!column_white(odd, y)) bits = (uint8_t)(bits & ~(1u << (6 - 2 * r)));
      }
      out[block] = bits;
    }
  }
}
