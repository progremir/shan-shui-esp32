#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

void st7305_fb_clear(uint8_t* fb);
void st7305_fb_set_pixel(uint8_t* fb, int x, int y, bool white);
void st7305_fb_rect(uint8_t* fb, int x0, int y0, int x1, int y1, bool white);

#ifndef HOST_TEST
void st7305_begin(void);
void st7305_flush(const uint8_t* fb);
#endif
