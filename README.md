# Shan shui on an ESP32-S3 reflective LCD

A host computer runs [shan-shui-inf](https://github.com/LingDong-/shan-shui-inf) at commit `9f754d2b2e73495db7883d4d4055a7b0903b0454` and streams the painting to a [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.com/esp32-s3-rlcd-4.2.htm). The board keeps a strip of the image in PSRAM and pans it.

The generator is the upstream `index.html`, unmodified, in `vendor/shan-shui-inf/`. Chrome runs it. The host dithers the painting to the panel's 1-bit ink and paper, and adds a few clouds in the open sky.

## Preview

Columns 0, 400, 800, and 1200 of the `wallpaper` seed. These are the 1-bit frames sent to the panel, drawn at 2×. They are not photographs of the LCD.

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

- Node.js 20 or newer. No packages to install.
- Google Chrome or Chromium. Set `CHROME` if the binary is not on the usual path.
- Arduino IDE 2 or arduino-cli, with the Espressif esp32 boards package 3.x.

## Flash

Open `firmware/shan_shui_wallpaper/shan_shui_wallpaper.ino` and select an ESP32S3 Dev Module:

- USB CDC On Boot: Enabled
- USB Mode: Hardware CDC and JTAG
- Flash Size: 16MB
- Flash Mode: QIO
- Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
- PSRAM: OPI PSRAM
- CPU Frequency: 240 MHz

```sh
FQBN=esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PSRAM=opi,CPUFreq=240,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB
arduino-cli compile --fqbn "$FQBN" firmware/shan_shui_wallpaper
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn "$FQBN" firmware/shan_shui_wallpaper
```

On macOS the port is a `cu.usbmodem` device (`ls /dev/cu.usb*`). On Linux it is `/dev/ttyACM*` or `/dev/ttyUSB*`. If the upload cannot sync, hold BOOT, start the upload, and release BOOT when writing starts.

The panel shows a thin double border until the host connects.

## Run

Preview in a browser. No board is required:

```sh
node host/cli.js preview --open
```

Stream to the board:

```sh
node host/cli.js stream --port /dev/cu.usbmodem1101
```

`--seed wallpaper` and `--speed 6` are optional. Speed is 1 to 24 pixels per second. The firmware defaults to 6.

The board stores 1600 columns and asks for more when fewer than 480 remain ahead of the 400-wide view. If the host falls behind, the picture holds instead of jumping. Leave `stream` running. If the board is powered off, the host waits and continues when the port returns. Power-off clears the on-board strip, so the painting starts again from the beginning.

KEY, on the left, steps the pan faster: 2, 4, 6, 8, 12, 18, then 24 pixels per second. BOOT, on the right, steps it slower.

Write still frames:

```sh
node host/cli.js frames --out /tmp/shan-shui-frames
```

## Config

Panel numbers are in `firmware/shan_shui_wallpaper/config.h`. Host defaults are in `host/lib/config.js`. `SCENE_USER_H`, `INK_SCALE`, `PAPER_CUTOFF`, and `DITHER_THRESHOLD` can be set in the environment for one run.

## License

shan-shui-inf is Copyright (c) 2018 Lingdong Huang, MIT. The notice is `vendor/shan-shui-inf/LICENSE`. `vendor/shan-shui-inf/index.html` is the unmodified file from the pinned commit. Keep that copyright and permission text with the generator if you redistribute it.

The host and firmware are separately MIT. See `LICENSE` and `NOTICE`.

The ST7305 register values are the vendor init for this board. This repository does not copy their example tree.
