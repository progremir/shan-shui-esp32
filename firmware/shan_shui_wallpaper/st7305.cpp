#include "st7305.h"

#include <Arduino.h>
#include <SPI.h>

static SPIClass* spi_bus = &SPI;

static void cmd_data(uint8_t cmd, const uint8_t* data, size_t n) {
  digitalWrite(PIN_DC, LOW);
  digitalWrite(PIN_CS, LOW);
  spi_bus->transfer(cmd);
  if (n) {
    digitalWrite(PIN_DC, HIGH);
    spi_bus->transferBytes(const_cast<uint8_t*>(data), nullptr, n);
  }
  digitalWrite(PIN_CS, HIGH);
}

static void cmd(uint8_t c) { cmd_data(c, nullptr, 0); }

static void reset_panel() {
  digitalWrite(PIN_RST, HIGH);
  delay(50);
  digitalWrite(PIN_RST, LOW);
  delay(20);
  digitalWrite(PIN_RST, HIGH);
  delay(50);
}

// Register sequence from Waveshare RLCD_Init() in
// 02_Example/Arduino/09_LVGL_V9_Test/display_bsp.cpp
// (the 0x69 / 0x4B / 0x80 revision used by the current board examples).
static void st7305_init_sequence() {
  const uint8_t d6[] = {0x17, 0x02};
  const uint8_t d1[] = {0x01};
  const uint8_t c0[] = {0x11, 0x04};
  const uint8_t c1[] = {0x69, 0x69, 0x69, 0x69};
  const uint8_t c2[] = {0x19, 0x19, 0x19, 0x19};
  const uint8_t c4[] = {0x4B, 0x4B, 0x4B, 0x4B};
  const uint8_t d8[] = {0x80, 0xE9};
  const uint8_t b2[] = {0x02};
  const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
  const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
  const uint8_t g62[] = {0x32, 0x03, 0x1F};
  const uint8_t b7[] = {0x13};
  const uint8_t b0[] = {0x64};
  const uint8_t c9[] = {0x00};
  const uint8_t m36[] = {0x48};
  const uint8_t m3a[] = {0x11};
  const uint8_t b9[] = {0x20};
  const uint8_t b8[] = {0x29};
  const uint8_t win_a[] = {ST7305_CASET_START, ST7305_CASET_END};
  const uint8_t win_b[] = {ST7305_RASET_START, ST7305_RASET_END};
  const uint8_t m35[] = {0x00};
  const uint8_t d0[] = {0xFF};

  cmd_data(0xD6, d6, sizeof d6);
  cmd_data(0xD1, d1, sizeof d1);
  cmd_data(0xC0, c0, sizeof c0);
  cmd_data(0xC1, c1, sizeof c1);
  cmd_data(0xC2, c2, sizeof c2);
  cmd_data(0xC4, c4, sizeof c4);
  cmd_data(0xC5, c2, sizeof c2);
  cmd_data(0xD8, d8, sizeof d8);
  cmd_data(0xB2, b2, sizeof b2);
  cmd_data(0xB3, b3, sizeof b3);
  cmd_data(0xB4, b4, sizeof b4);
  cmd_data(0x62, g62, sizeof g62);
  cmd_data(0xB7, b7, sizeof b7);
  cmd_data(0xB0, b0, sizeof b0);
  cmd(0x11);
  delay(200);
  cmd_data(0xC9, c9, sizeof c9);
  cmd_data(0x36, m36, sizeof m36);
  cmd_data(0x3A, m3a, sizeof m3a);
  cmd_data(0xB9, b9, sizeof b9);
  cmd_data(0xB8, b8, sizeof b8);
  cmd(0x21);
  cmd_data(0x2A, win_a, sizeof win_a);
  cmd_data(0x2B, win_b, sizeof win_b);
  cmd_data(0x35, m35, sizeof m35);
  cmd_data(0xD0, d0, sizeof d0);
  cmd(0x38);
  cmd(0x29);
}

void st7305_begin() {
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_RST, HIGH);
  spi_bus->begin(PIN_SCK, -1, PIN_MOSI, -1);
  spi_bus->beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  reset_panel();
  st7305_init_sequence();
}

void st7305_flush(const uint8_t* fb) {
  const uint8_t win_a[] = {ST7305_CASET_START, ST7305_CASET_END};
  const uint8_t win_b[] = {ST7305_RASET_START, ST7305_RASET_END};
  cmd_data(0x2A, win_a, sizeof win_a);
  cmd_data(0x2B, win_b, sizeof win_b);
  digitalWrite(PIN_DC, LOW);
  digitalWrite(PIN_CS, LOW);
  spi_bus->transfer(0x2C);
  digitalWrite(PIN_DC, HIGH);
  spi_bus->transferBytes(const_cast<uint8_t*>(fb), nullptr, SS_FB_BYTES);
  digitalWrite(PIN_CS, HIGH);
}
