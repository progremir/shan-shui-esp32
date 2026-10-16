# Shan shui wallpaper

An endless Chinese landscape painting that slowly scrolls across a small paper-like screen. The board paints it by itself, forever, with no computer or internet connection. Every time you turn it on, you get a new landscape.

The paintings come from [shan-shui-inf](https://github.com/LingDong-/shan-shui-inf) by Lingdong Huang, a generator that draws mountains, trees, houses, boats, and water in the style of Chinese ink paintings. This project runs it on an ESP32 microcontroller with a reflective black-and-white display.

![Four moments of the scroll](docs/preview/pan.png)

*The screen is black and white, so gray tones are made of dots. These are frames at 2× size.*

## What you need

- A [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.com/esp32-s3-rlcd-4.2.htm). It is a single board with the screen, buttons, and battery charging built in. No soldering or extra parts.
- A USB-C cable.
- A computer to install the software once. The steps below are for macOS and Linux; on Windows, use the ready-made image (see *Alternative* below).

## Install

**1. Install arduino-cli and the ESP32 support.** On macOS with [Homebrew](https://brew.sh):

```sh
brew install arduino-cli
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

On other systems, see the [arduino-cli install guide](https://arduino.github.io/arduino-cli/latest/installation/), then run the last two commands.

**2. Download this project and plug in the board.**

```sh
git clone https://github.com/progremir/shan-shui-esp32.git
cd shan-shui-esp32
```

Find the board's port. On macOS run `ls /dev/cu.usb*`; it looks like `/dev/cu.usbmodem1101`. On Linux it is usually `/dev/ttyACM0`.

**3. Build and install.** Replace the port with yours:

```sh
firmware/build.sh /dev/cu.usbmodem1101
```

This replaces whatever was on the board before. The first build takes a few minutes.

**If something goes wrong**

- *The upload can't connect:* hold the BOOT button (right), start the command again, and let go once it says "Writing".
- *"bad CPU type in executable" on an Apple Silicon Mac:* one of the Arduino tools needs Rosetta. Run `softwareupdate --install-rosetta`.

**Alternative: without building.** `firmware/flash/` has a ready-made image. With [esptool](https://docs.espressif.com/projects/esptool/) installed (`pip install esptool`), and your port (on Windows, a `COM` port from Device Manager):

```sh
cd firmware/flash
esptool.py --chip esp32s3 --port /dev/cu.usbmodem1101 write_flash @flash_args
```

## Using it

Turn it on with the middle (power) button. You'll see a thin double frame for about 15 seconds while it paints the first view, then the landscape starts moving.

- **Left button:** scroll faster.
- **Right button:** scroll slower.
- **Middle button:** short press turns it on, long press turns it off.

It runs from USB power or a battery. The screen has no backlight: like paper, it needs room light to be seen, and that is also why it uses very little power.

## How it works

The original shan-shui-inf is a web page that draws in a browser. This project rewrites it in C++ so it can run on the board. The board's two processor cores share the work: one paints the landscape a little ahead of what you see, and the other scrolls the view across it.

The display only shows black or white, so the painting's soft grays are turned into fine patterns of dots (dithering).

To make sure the rewrite draws the same thing as the original, there is a test that runs both side by side and compares their output (needs Node.js and a C++ compiler):

```sh
make -C firmware/test/shanshui check
```

## What's in this repository

| Folder | Contents |
| --- | --- |
| `firmware/shan_shui_wallpaper/` | The program that runs on the board |
| `firmware/shan_shui_wallpaper/src/shanshui/` | The C++ version of shan-shui-inf |
| `firmware/flash/` | Ready-made image for flashing without building |
| `firmware/test/` | Tests that compare the C++ version with the original |
| `vendor/shan-shui-inf/` | The original shan-shui-inf web page, unchanged |
| `host/` | An older version where a computer drew the painting and sent it to the board over USB. Not needed. |

## Credits and license

The landscape generator is [shan-shui-inf](https://github.com/LingDong-/shan-shui-inf), Copyright (c) 2018 Lingdong Huang, MIT license. The C++ version in `firmware/shan_shui_wallpaper/src/shanshui/` is a port of it and keeps that license; see `vendor/shan-shui-inf/LICENSE`.

Everything else is MIT licensed; see `LICENSE` and `NOTICE`.
