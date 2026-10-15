#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

struct SsFrame {
  uint8_t type;
  uint8_t flags;
  uint16_t seq;
  uint16_t len;
  uint8_t payload[SS_MAX_PAYLOAD];
};

struct SsParser {
  int state;
  uint16_t got;
  uint16_t need;
  uint8_t crc_lo;
  uint8_t buf[6 + SS_MAX_PAYLOAD];
  bool ready;
  SsFrame frame;
};

void ss_parser_init(SsParser* parser);
// Consumes bytes until a frame is ready or the buffer ends.
// Returns 0 when a frame is already waiting and must be taken first.
size_t ss_parser_push(SsParser* parser, const uint8_t* data, size_t n);
bool ss_parser_take(SsParser* parser, SsFrame* out);

uint16_t ss_crc16(const uint8_t* data, size_t len);
size_t ss_frame_encode(uint8_t* dst, size_t cap, uint8_t type, uint8_t flags,
                       uint16_t seq, const uint8_t* payload, uint16_t len);
size_t ss_encode_hello(uint8_t* dst, size_t cap, uint16_t seq);

void ss_write_u16(uint8_t* p, uint16_t v);
void ss_write_u32(uint8_t* p, uint32_t v);
uint16_t ss_read_u16(const uint8_t* p);
uint32_t ss_read_u32(const uint8_t* p);
