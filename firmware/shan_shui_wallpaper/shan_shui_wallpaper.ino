// Standalone wallpaper for the Waveshare ESP32-S3-RLCD-4.2.
// Flashing this replaces whatever is already on the board.
// USB-C is the serial link. Do not Serial.print: the bytes are the protocol.
// This sketch was not run on hardware in the environment that produced it.

#include <Arduino.h>

#include "config.h"
#include "protocol.h"
#include "st7305.h"
#include "strip.h"

static SsParser parser;
static uint8_t tx[SS_MAX_PAYLOAD + 16];
static Strip strip;
static uint8_t* fb = nullptr;
static bool psram_ok = false;
static bool host_ok = false;
static bool have_frame = false;
static uint16_t speed = SS_SCROLL_DEFAULT;
static uint16_t seq = 1;
static uint32_t view_x = 0;
static uint32_t next_hello = 0;
static uint32_t next_scroll = 0;
static uint32_t next_need = 0;
static bool need_wait = false;
static uint32_t need_x = 0;
static const uint16_t kSpeeds[] = {2, 4, 6, 8, 12, 18, 24};
static const int kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);
static int speed_i = 2;

struct SsButton {
  int pin;
  int stable;
  int read;
  uint32_t change;
};

static SsButton key_btn = {PIN_KEY, 1, 1, 0};
static SsButton boot_btn = {PIN_BOOT, 1, 1, 0};

static void match_speed(uint16_t sp);

// The USB CDC TX FIFO is 64 bytes. Serial.write busy-spins and can return a
// short count, which puts a torn frame on the wire. Send a whole frame or
// nothing, and let the caller try again.
static bool send_bytes(const uint8_t* data, size_t n) {
  if (!n || n > 60) return false;
  if (Serial.availableForWrite() < (int)n) return false;
  return Serial.write(data, n) == n;
}

static bool send_log(const char* text) {
  size_t n = strlen(text);
  if (n > 48) n = 48;
  size_t framed = ss_frame_encode(tx, sizeof tx, SS_TYPE_LOG, 0, seq, (const uint8_t*)text, (uint16_t)n);
  if (!framed || !send_bytes(tx, framed)) return false;
  seq++;
  return true;
}

static bool send_hello() {
  size_t n = ss_encode_hello(tx, sizeof tx, seq);
  if (!n || !send_bytes(tx, n)) return false;
  seq++;
  return true;
}

static bool send_ack(uint32_t start, uint16_t columns) {
  uint8_t payload[8];
  ss_write_u32(payload, start);
  ss_write_u16(payload + 4, columns);
  ss_write_u16(payload + 6, strip.count);
  size_t n = ss_frame_encode(tx, sizeof tx, SS_TYPE_ACK, 0, seq, payload, 8);
  if (!n || !send_bytes(tx, n)) return false;
  seq++;
  return true;
}

static bool send_need(uint32_t start, uint16_t columns) {
  uint8_t payload[8];
  ss_write_u32(payload, start);
  ss_write_u16(payload + 4, columns);
  ss_write_u16(payload + 6, strip_free(&strip));
  size_t n = ss_frame_encode(tx, sizeof tx, SS_TYPE_NEED, 0, seq, payload, 8);
  if (!n || !send_bytes(tx, n)) return false;
  seq++;
  return true;
}

static uint32_t scroll_interval() {
  uint16_t sp = speed < 1 ? 1 : speed;
  return 1000UL / sp;
}

static void show_border() {
  if (!fb) return;
  st7305_fb_clear(fb);
  st7305_fb_rect(fb, 2, 2, PANEL_W - 3, PANEL_H - 3, false);
  st7305_fb_rect(fb, 6, 6, PANEL_W - 7, PANEL_H - 7, false);
  st7305_flush(fb);
}

static void show_view() {
  if (!fb) return;
  strip_blit(&strip, view_x, fb);
  st7305_flush(fb);
}

static void handle_columns(const SsFrame& frame) {
  if (frame.len < 10) return;
  uint32_t start = ss_read_u32(frame.payload);
  uint16_t columns = ss_read_u16(frame.payload + 4);
  uint16_t height = ss_read_u16(frame.payload + 6);
  uint16_t stride = ss_read_u16(frame.payload + 8);
  if (height != PANEL_H || stride != SS_COL_STRIDE) {
    send_log("column header does not match this panel");
    need_wait = false;
    return;
  }
  if ((uint32_t)columns * stride + 10 != frame.len || columns == 0) {
    send_log("column payload length mismatch");
    need_wait = false;
    return;
  }
  uint32_t expected = strip.start_x + strip.count;
  if (start + columns <= expected) {
    send_ack(start, columns);
    need_wait = false;
    return;
  }
  if (start > expected) {
    send_log("column gap");
    need_wait = false;
    return;
  }
  uint32_t skip = expected - start;
  uint16_t take = (uint16_t)(columns - skip);
  const uint8_t* pixels = frame.payload + 10 + skip * stride;
  if (!strip_append(&strip, pixels, take)) {
    send_log("strip full");
    need_wait = false;
    return;
  }
  send_ack(expected, take);
  need_wait = false;
}

static void handle_frame(const SsFrame& frame) {
  if (frame.type == SS_TYPE_HELLO_ACK && frame.len >= 2) {
    uint16_t sp = ss_read_u16(frame.payload);
    if (sp >= 1 && sp <= SS_SCROLL_MAX) match_speed(sp);
    host_ok = true;
    send_log("host connected");
    return;
  }
  if (frame.type == SS_TYPE_COLUMNS) handle_columns(frame);
}

static void drain_serial() {
  uint8_t tmp[256];
  while (Serial.available() > 0) {
    int avail = Serial.available();
    if (avail > (int)sizeof tmp) avail = sizeof tmp;
    int n = Serial.read(tmp, (size_t)avail);
    if (n <= 0) break;
    size_t off = 0;
    while (off < (size_t)n) {
      if (parser.ready) {
        SsFrame frame;
        ss_parser_take(&parser, &frame);
        handle_frame(frame);
        continue;
      }
      size_t consumed = ss_parser_push(&parser, tmp + off, (size_t)n - off);
      if (consumed == 0) break;
      off += consumed;
    }
    if (parser.ready) {
      SsFrame frame;
      ss_parser_take(&parser, &frame);
      handle_frame(frame);
    }
  }
}

static void log_speed() {
  char msg[16];
  snprintf(msg, sizeof msg, "speed %u", speed);
  send_log(msg);
}

static void apply_speed_index(int index) {
  if (index < 0) index = 0;
  if (index >= kSpeedCount) index = kSpeedCount - 1;
  if (index == speed_i && speed == kSpeeds[index]) return;
  speed_i = index;
  speed = kSpeeds[index];
  next_scroll = millis();
  log_speed();
}

static void match_speed(uint16_t sp) {
  if (sp < 1) sp = 1;
  if (sp > SS_SCROLL_MAX) sp = SS_SCROLL_MAX;
  speed = sp;
  int best = 0;
  uint16_t best_d = 0xffff;
  for (int i = 0; i < kSpeedCount; i++) {
    uint16_t d = kSpeeds[i] > sp ? kSpeeds[i] - sp : sp - kSpeeds[i];
    if (d < best_d) {
      best_d = d;
      best = i;
    }
  }
  speed_i = best;
}

static void poll_button(SsButton* button, int dir) {
  int reading = digitalRead(button->pin);
  uint32_t now = millis();
  if (reading != button->read) {
    button->read = reading;
    button->change = now;
    return;
  }
  if ((uint32_t)(now - button->change) > 25 && reading != button->stable) {
    button->stable = reading;
    if (button->stable == 0) apply_speed_index(speed_i + dir);
  }
}

static void maybe_need(uint32_t now) {
  if (!psram_ok) return;
  uint32_t filled = strip.start_x + strip.count;
  uint32_t right = view_x + PANEL_W;
  uint32_t ahead = filled > right ? filled - right : 0;
  if (ahead >= SS_LOW_WATER) return;
  uint16_t free_cols = strip_free(&strip);
  if (free_cols < 8) return;
  uint16_t ask = free_cols < SS_CHUNK_COLUMNS ? free_cols : SS_CHUNK_COLUMNS;
  if (need_wait && need_x == filled && (int32_t)(now - next_need) < 0) return;
  if (!send_need(filled, ask)) return;
  need_wait = true;
  need_x = filled;
  next_need = now + 1500;
}

static void maybe_scroll(uint32_t now) {
  if (!psram_ok) return;
  if (!have_frame) {
    if (strip_covers(&strip, 0, PANEL_W)) {
      view_x = 0;
      show_view();
      have_frame = true;
      next_scroll = now + scroll_interval();
    }
    return;
  }
  if ((int32_t)(now - next_scroll) < 0) return;
  next_scroll += scroll_interval();
  if ((int32_t)(now - next_scroll) > 1000) next_scroll = now + scroll_interval();
  if (!strip_covers(&strip, view_x + 1, PANEL_W)) return;
  view_x++;
  strip_drop_before(&strip, view_x);
  show_view();
}

void setup() {
  Serial.setRxBufferSize(8192);
  Serial.setTxBufferSize(16384);
  Serial.begin(115200);
  Serial.setTimeout(0);
  Serial.setTxTimeoutMs(0);
  ss_parser_init(&parser);
  pinMode(PIN_KEY, INPUT_PULLUP);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  st7305_begin();
  fb = (uint8_t*)ps_malloc(SS_FB_BYTES);
  psram_ok = fb && strip_init(&strip, SS_STRIP_COLUMNS, SS_COL_STRIDE);
  if (!psram_ok && !fb) fb = (uint8_t*)malloc(SS_FB_BYTES);
  show_border();
  if (!psramFound() || !psram_ok) send_log("PSRAM missing; enable OPI PSRAM");
  else send_log("ST7305 ready");
}

void loop() {
  drain_serial();
  poll_button(&key_btn, 1);
  poll_button(&boot_btn, -1);
  uint32_t now = millis();
  if (!host_ok) {
    if ((int32_t)(now - next_hello) >= 0) {
      next_hello = send_hello() ? now + 1000 : now + 50;
    }
  } else {
    maybe_need(now);
    maybe_scroll(now);
  }
  delay(1);
}
