#include "protocol.h"

#include <string.h>

void ss_write_u16(uint8_t* p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
}

void ss_write_u32(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

uint16_t ss_read_u16(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t ss_read_u32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

uint16_t ss_crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xffff;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
      else crc = (uint16_t)(crc << 1);
    }
  }
  return crc;
}

size_t ss_frame_encode(uint8_t* dst, size_t cap, uint8_t type, uint8_t flags,
                       uint16_t seq, const uint8_t* payload, uint16_t len) {
  if ((size_t)len + 10 > cap || len > SS_MAX_PAYLOAD) return 0;
  dst[0] = SS_SYNC0;
  dst[1] = SS_SYNC1;
  dst[2] = type;
  dst[3] = flags;
  ss_write_u16(dst + 4, seq);
  ss_write_u16(dst + 6, len);
  if (len && payload) memcpy(dst + 8, payload, len);
  uint16_t crc = ss_crc16(dst + 2, (size_t)len + 6);
  ss_write_u16(dst + 8 + len, crc);
  return (size_t)len + 10;
}

size_t ss_encode_hello(uint8_t* dst, size_t cap, uint16_t seq) {
  uint8_t payload[28];
  memset(payload, 0, sizeof payload);
  ss_write_u16(payload + 0, PANEL_W);
  ss_write_u16(payload + 2, PANEL_H);
  ss_write_u16(payload + 4, PANEL_BPP);
  ss_write_u16(payload + 6, SS_STRIP_COLUMNS);
  ss_write_u16(payload + 8, PANEL_W);
  ss_write_u16(payload + 10, SS_PROTO_VERSION);
  memcpy(payload + 12, "ST7305", 6);
  return ss_frame_encode(dst, cap, SS_TYPE_HELLO, 0, seq, payload, 28);
}

void ss_parser_init(SsParser* parser) {
  memset(parser, 0, sizeof(*parser));
}

static void parser_reset(SsParser* parser) {
  parser->state = 0;
  parser->got = 0;
  parser->need = 0;
}

size_t ss_parser_push(SsParser* parser, const uint8_t* data, size_t n) {
  if (parser->ready) return 0;
  for (size_t i = 0; i < n; i++) {
    uint8_t b = data[i];
    if (parser->state == 0) {
      if (b == SS_SYNC0) parser->state = 1;
      continue;
    }
    if (parser->state == 1) {
      parser->state = (b == SS_SYNC1) ? 2 : (b == SS_SYNC0 ? 1 : 0);
      continue;
    }
    if (parser->state == 2) {
      parser->buf[parser->got++] = b;
      if (parser->got == 6) {
        parser->need = ss_read_u16(parser->buf + 4);
        if (parser->need > SS_MAX_PAYLOAD) {
          parser_reset(parser);
          continue;
        }
        parser->state = parser->need == 0 ? 4 : 3;
      }
      continue;
    }
    if (parser->state == 3) {
      parser->buf[parser->got++] = b;
      if (parser->got == 6 + parser->need) parser->state = 4;
      continue;
    }
    if (parser->state == 4) {
      parser->crc_lo = b;
      parser->state = 5;
      continue;
    }
    uint16_t crc = (uint16_t)parser->crc_lo | ((uint16_t)b << 8);
    if (ss_crc16(parser->buf, 6 + parser->need) == crc) {
      parser->frame.type = parser->buf[0];
      parser->frame.flags = parser->buf[1];
      parser->frame.seq = ss_read_u16(parser->buf + 2);
      parser->frame.len = parser->need;
      if (parser->need) memcpy(parser->frame.payload, parser->buf + 6, parser->need);
      parser->ready = true;
    }
    parser_reset(parser);
    if (parser->ready) return i + 1;
  }
  return n;
}

bool ss_parser_take(SsParser* parser, SsFrame* out) {
  if (!parser->ready) return false;
  *out = parser->frame;
  parser->ready = false;
  return true;
}
