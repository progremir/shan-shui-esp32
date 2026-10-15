#include "st7305.h"

#include <string.h>

// Landscape packing from Waveshare RLCD_SetLandscapePixel / InitLandscapeLUT
// (display_bsp.cpp, width == 400). A set bit is paper, a clear bit is ink,
// matching RLCD_ColorClear(0xFF) before the vendor's 0x21 invert.

void st7305_fb_clear(uint8_t* fb) {
  memset(fb, 0xff, SS_FB_BYTES);
}

void st7305_fb_set_pixel(uint8_t* fb, int x, int y, bool white) {
  if (x < 0 || y < 0 || x >= PANEL_W || y >= PANEL_H) return;
  int inv_y = PANEL_H - 1 - y;
  int byte_x = x / 2;
  int block_y = inv_y / 4;
  int index = byte_x * (PANEL_H / 4) + block_y;
  int bit = 7 - ((inv_y % 4) * 2 + (x % 2));
  uint8_t mask = (uint8_t)(1u << bit);
  if (white) fb[index] |= mask;
  else fb[index] &= (uint8_t)~mask;
}

void st7305_fb_rect(uint8_t* fb, int x0, int y0, int x1, int y1, bool white) {
  for (int x = x0; x <= x1; x++) {
    st7305_fb_set_pixel(fb, x, y0, white);
    st7305_fb_set_pixel(fb, x, y1, white);
  }
  for (int y = y0; y <= y1; y++) {
    st7305_fb_set_pixel(fb, x0, y, white);
    st7305_fb_set_pixel(fb, x1, y, white);
  }
}
