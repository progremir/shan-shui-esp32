# Shan shui on an ESP32-S3 reflective LCD

A [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.com/esp32-s3-rlcd-4.2.htm) paints an endless [shan-shui-inf](https://github.com/LingDong-/shan-shui-inf) landscape by itself and pans across it. No computer is needed after flashing: plug it into any USB power or run it from the battery.

The firmware carries a C++ port of shan-shui-inf at commit `9f754d2b2e73495db7883d4d4055a7b0903b0454`. One core plans and paints the mountains, trees, buildings, boats, and water into a strip of columns in PSRAM; the other pans the view across it. The painting is dithered to the panel's 1-bit ink and paper, with a few clouds in the open sky.

## Preview

Columns 0, 400, 800, and 1200 of the `wallpaper` seed as 1-bit panel frames, drawn at 2×. They are not photographs of the LCD. The board paints a new seed at every power-on, so its landscapes differ.

![Four moments of the scroll](docs/preview/pan.png)

[0](docs/preview/frame-0000.png) · [400](docs/preview/frame-0400.png) · [800](docs/preview/frame-0800.png) · [1200](docs/preview/frame-1200.png)

## Hardware

ESP32-S3-WROOM-1-N16R8: dual-core 240 MHz, 16 MB flash, 8 MB octal PSRAM. USB-C is power, flashing, and the serial link.

The display is the onboard 4.2 inch reflective monochrome LCD (ST7305, SPI, no backlight). The firmware uses the vendor landscape window, 400×300, at 1 bit per pixel. Init command `0x3A = 0x11` selects that depth.

| Signal | GPIO |
| --- | --- |
| SCK | 11 |
| MOSI | 12 |
| CS | 40 |
| DC | 5 |
| RST | 41 |
| KEY | 18, active low |
| BOOT | 0, active low |

SPI is mode 0 at 10 MHz. The register sequence and landscape pixel layout follow the vendor example for this board. Audio, the microphones, the humidity sensor, the RTC, the battery charger, the microSD slot, and the GPIO header are unused.

The middle button is power. A short press turns the board on, and a long press turns it off. Firmware cannot read that button. Hold BOOT while plugging in USB if a flash does not catch the port.

**Flashing replaces the firmware currently on the board.**

## Requirements

- arduino-cli (or Arduino IDE 2) with the Espressif esp32 boards package 3.x.
- For the reference test only: Node.js 20 or newer and a C++20 compiler.

## Flash

Install [arduino-cli](https://arduino.github.io/arduino-cli/) and the Espressif esp32 boards package 3.x, then:

```sh
firmware/build.sh /dev/cu.usbmodem1101
```

On macOS the port is a `cu.usbmodem` device (`ls /dev/cu.usb*`). On Linux it is `/dev/ttyACM*` or `/dev/ttyUSB*`. Without a port the script only compiles. If the upload cannot sync, hold BOOT, start the upload, and release BOOT when writing starts.

`build.sh` passes `-fsingle-precision-constant`. The painter works in float, and without that flag its decimal constants pull the math back into software double, which paints about half as fast. The Arduino IDE also works (ESP32S3 Dev Module, USB CDC On Boot: Enabled, USB Mode: Hardware CDC and JTAG, Flash Size: 16MB, Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS), PSRAM: OPI PSRAM), just slower.

`firmware/flash/` holds a prebuilt image for `esptool.py write_flash @flash_args` from that directory.

## Run

Power it. The panel shows a thin double border for about 15 seconds while the first screen is painted, then the landscape starts to pan. Every power-on paints a new landscape from a random seed. After 400,000 columns (about 5 hours at the fastest pan, a day at the default) it starts a new one, because the float coordinates get coarse that far out.

KEY, on the left, steps the pan faster: 2, 4, 6, 8, 12, 18, then 24 pixels per second. BOOT, on the right, steps it slower. The painter keeps 480 columns ready ahead of the view and paints about 50 columns a second, so no speed outruns it.

USB serial (115200) prints the seed, painting speed, and free memory.

## Development

`firmware/test/shanshui/` checks the port against the original. `ref.mjs` runs the unmodified `vendor/shan-shui-inf/index.html` in Node, and the same cases run through the C++ port built in double precision; the SVG text must match byte for byte.

```sh
make -C firmware/test/shanshui check
```

`render.cpp` in the same directory paints any number of columns to an image on the desktop, with timing and memory figures.

`host/` is the earlier setup, in which a Mac rendered the painting in Chrome and streamed it over USB. It needs the streaming firmware from commit `918215f` and does not talk to the current firmware.

## Config

Panel numbers are in `firmware/shan_shui_wallpaper/config.h`. Tile size, look-ahead, and the painting length are at the top of `firmware/shan_shui_wallpaper/shan_shui_wallpaper.ino`.

## License

shan-shui-inf is Copyright (c) 2018 Lingdong Huang, MIT. The notice is `vendor/shan-shui-inf/LICENSE`. `vendor/shan-shui-inf/index.html` is the unmodified file from the pinned commit, and `firmware/shan_shui_wallpaper/src/shanshui/` is a C++ port of it. Keep that copyright and permission text with either if you redistribute them.

The host and firmware are separately MIT. See `LICENSE` and `NOTICE`.

The ST7305 register values are the vendor init for this board. This repository does not copy their example tree.
