#pragma once

// Waveshare ESP32-S3-RLCD-4.2
// https://www.waveshare.com/esp32-s3-rlcd-4.2.htm
//
// ESP32-S3-WROOM-1-N16R8, 240 MHz, 16 MB flash, 8 MB octal PSRAM.
// Display: 4.2" reflective monochrome LCD, no backlight, ST7305, SPI.
// Landscape window used by the vendor board config: 400 x 300, 1 bit
// (product grey scale 2). Command 0x3A data 0x11 selects that depth.
//
// Pins are the board defaults from Waveshare
// 02_Example/XiaoZhi/.../waveshare-s3-rlcd-4.2/config.h
// and the ESP32-S3-RLCD-4.2 pin table (KEY on GPIO18).
// SPI clock and the init bytes match
// 02_Example/Arduino/09_LVGL_V9_Test/display_bsp.cpp (10 MHz, RLCD_Init).
//
// Change a panel or a pin here. The rest of the firmware reads these macros.

#define PANEL_W 400
#define PANEL_H 300
#define PANEL_BPP 1

#define PIN_SCK 11
#define PIN_MOSI 12
#define PIN_CS 40
#define PIN_DC 5
#define PIN_RST 41
#define PIN_TE 6
#define PIN_KEY 18
#define PIN_BOOT 0

#define SPI_HZ 10000000

#define ST7305_CASET_START 0x12
#define ST7305_CASET_END 0x2A
#define ST7305_RASET_START 0x00
#define ST7305_RASET_END 0xC7

// 4 columns x 2 rows are not the packing. Landscape mode stores 2 columns
// by 4 rows per byte.  (400/2) * (300/4) = 15000, which is also W*H/8.
#define SS_FB_BYTES ((PANEL_W / 2) * (PANEL_H / 4))

#define SS_STRIP_COLUMNS 1600
#define SS_CHUNK_COLUMNS 48
#define SS_LOW_WATER 480
#define SS_SCROLL_DEFAULT 6
#define SS_SCROLL_MAX 24

#define SS_SYNC0 0xA5
#define SS_SYNC1 0x5A
#define SS_TYPE_HELLO 0x01
#define SS_TYPE_HELLO_ACK 0x02
#define SS_TYPE_NEED 0x10
#define SS_TYPE_COLUMNS 0x11
#define SS_TYPE_ACK 0x12
#define SS_TYPE_LOG 0x20
#define SS_MAX_PAYLOAD 3072
#define SS_PROTO_VERSION 1

#define SS_COL_STRIDE ((PANEL_H + 7) / 8)

// GPIO0 is the BOOT button. Holding it while powering on is still the
// download strap. While the wallpaper is running it is an input: press to
// slow the pan. KEY (GPIO18) speeds the pan up.
// Left alone on purpose: GPIO4 battery ADC, GPIO8/9/10/16/45
// I2S for ES8311 + ES7210, GPIO13/14 I2C (ES8311, ES7210, SHTC3, PCF85063),
// GPIO46 speaker amplifier, the microSD slot, CHG/WRN LEDs, and the 2x8
// expansion header. PWR is the board power button (click on, hold off).
