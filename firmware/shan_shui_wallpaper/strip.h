#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

struct Strip {
  uint8_t* data;
  uint16_t capacity;
  uint16_t stride;
  uint16_t count;
  uint16_t head;
  uint32_t start_x;
};

bool strip_init(Strip* strip, uint16_t capacity, uint16_t stride);
void strip_reset(Strip* strip);
bool strip_covers(const Strip* strip, uint32_t x, uint16_t n);
uint16_t strip_free(const Strip* strip);
bool strip_append(Strip* strip, const uint8_t* packed, uint16_t columns);
void strip_drop_before(Strip* strip, uint32_t x);
void strip_blit(const Strip* strip, uint32_t view_x, uint8_t* fb);
