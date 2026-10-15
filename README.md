# Shan shui on an ESP32-S3 reflective LCD

The Mac runs [shan-shui-inf](https://github.com/LingDong-/shan-shui-inf) at commit `9f754d2b2e73495db7883d4d4055a7b0903b0454` and streams the painting to a Waveshare ESP32-S3-RLCD-4.2. The board scrolls that strip in its own memory, so the motion is a slow pan rather than a slide show.

The generator is the upstream `index.html`, unmodified, under `vendor/shan-shui-inf/`. Chrome executes it. The host dithers the whole painting to the panel's 1-bit ink and paper, so a translucent stroke reads as gray, and it also dithers a few clouds into the open sky. It keeps rendering the next section before the one on screen runs out.

No board was attached while this project was written. The display path is the vendor init sequence and pin map; it has not been watched on the panel.

## How it looks

The panel only stores black and paper. The host dithers translucent strokes, and a few clouds in the open sky, into that 1-bit image. These four moments are columns 0, 400, 800, and 1200 of the `wallpaper` seed, drawn at 2× so the dots stay crisp. They are the frames the board is sent, not photographs of the LCD.

![Four moments of the scroll](docs/preview/pan.png)

Same frames, one file each: [0](docs/preview/frame-0000.png), [400](docs/preview/frame-0400.png), [800](docs/preview/frame-0800.png), [1200](docs/preview/frame-1200.png).

## Board

[Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.com/esp32-s3-rlcd-4.2.htm): ESP32-S3-WROOM-1-N16R8 (dual-core 240 MHz, 16 MB flash, 8 MB octal PSRAM), USB-C for power, flashing, and the serial link.

The display is the onboard 4.2 inch reflective monochrome LCD. Controller **ST7305**, SPI, no backlight. Waveshare lists grey scale 2, and the init command `0x3A = 0x11` selects 1 bit per pixel. The firmware uses the vendor landscape window, **400×300**. A set bit is paper and a clear bit is ink, which is the same polarity as their `RLCD_ColorClear(0xFF)` plus the `0x21` invert in that init.

Pins, in `firmware/shan_shui_wallpaper/config.h`:

| Signal | GPIO |
| --- | --- |
| SCK | 11 |
| MOSI | 12 |
| CS | 40 |
| DC | 5 |
| RST | 41 |
| KEY | 18, active low, faster |
| BOOT | 0, active low, slower |

SPI is mode 0 at 10 MHz, from the vendor `display_bsp.cpp` constructor. The register list is `RLCD_Init()` from `02_Example/Arduino/09_LVGL_V9_Test/display_bsp.cpp` in [waveshareteam/ESP32-S3-RLCD-4.2](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2) (the `0x69` / `0x4B` / `0x80` sequence). Landscape pixels follow `RLCD_SetLandscapePixel` in that file.

USB-C is native USB. Hold **BOOT** while powering on (or while plugging in) to force the ROM download mode if a normal upload does not catch the port. The middle button is **PWR**, the power latch: a short press turns the board on, a long press turns it off. Firmware cannot see that button. Turning the board off drops USB, and the picture stays where it was until the board is on and the host is talking to it again.

These stay unused: ES8311 and the speaker socket, the ES7210 dual mics, SHTC3, PCF85063 and the RTC battery, the 18650 holder and charger, the CHG/WRN LEDs, the microSD slot, and the 2×8 GPIO header. KEY speeds the scroll up. BOOT slows it down. Holding BOOT while powering on is still the download strap.

**Flashing this firmware replaces whatever is on the board, including Mochi.** This project does not call Mochi and does not read the SHTC3 or the RTC. After you flash, those devices sit idle until some other program uses them again.

## macOS dependencies

- Node.js 20 or newer (`node` on your PATH). No `npm install`.
- Google Chrome. If it is not the usual application path, set `CHROME` to the binary.
- Arduino IDE 2, or arduino-cli, with the Espressif **esp32** boards package (3.x). That package is what actually compiles the sketch.

## Pick the serial port

Plug in USB-C. The port is a `usbmodem` device. Prefer the `cu` node:

```sh
ls /dev/cu.usb*
```

A typical name is `/dev/cu.usbmodem1101`. `tty.usbmodem*` is the same cable with a different device node; use `cu`. If nothing is listed, hold BOOT, plug the cable in again, and look once more. Release BOOT after the port appears.

## Flash the ESP32

Open `firmware/shan_shui_wallpaper/shan_shui_wallpaper.ino`.

Board menu for an ESP32S3 Dev Module:

- USB CDC On Boot: **Enabled**
- USB Mode: **Hardware CDC and JTAG**
- Flash Size: **16MB**
- Flash Mode: **QIO**
- Partition Scheme: **16M Flash (3MB APP/9.9MB FATFS)**
- PSRAM: **OPI PSRAM**
- CPU Frequency: **240 MHz**
- Upload Speed: 921600 is fine

Select the `cu.usbmodem` port and upload. If esptool cannot sync, hold BOOT, start the upload, and release BOOT when the tool starts writing.

arduino-cli, with the port filled in:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PSRAM=opi,CPUFreq=240,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB firmware/shan_shui_wallpaper
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PSRAM=opi,CPUFreq=240,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB firmware/shan_shui_wallpaper
```

The option names follow the 3.x menu. If a flag is rejected, set the same items in the IDE and leave the fqbn that `arduino-cli board details` prints.

The panel should show a thin double border while it waits for the Mac. That border is only a sign of life. The painting starts after the host answers.

## Run the host

From this directory:

```sh
node host/cli.js preview --open
```

That opens a 400×300 page (drawn at 2×) and scrolls the ink painting at 6 pixels per second. Chrome has to be running the real generator, so the first screen waits while the opening stretch of the scroll is built. Leave that terminal open.

With the board plugged in:

```sh
node host/cli.js stream --port /dev/cu.usbmodem1101
```

Optional flags: `--seed wallpaper` and `--speed 6`. The speed is sent to the board (1 to 24 pixels per second). The firmware also defaults to 6 if you have not connected yet.

The board stores a 1600-column strip in PSRAM. It asks for the next 48 columns when fewer than 480 columns remain ahead of the viewport, then pans one pixel at a time. If the Mac falls behind, the picture holds on the last column instead of jumping.

KEY, the button on the left, steps the pan faster: 2, 4, 6, 8, 12, 18, then 24 pixels per second. BOOT, on the right, steps it slower. The middle button is power, not speed. The host's `--speed` sets the starting step. A fast pan used to stall after a while: the board redrew every pixel by hand and could split a USB frame, after which new columns were rejected and the picture stayed put. Reflash the firmware below to pick that up. The host also keeps painting ahead of the board, so leave `stream` running. A long press on the middle button turns the board off and drops USB, which freezes the picture. Short-press it to turn the board back on. The host stays up and waits for the port. Power-off clears the strip, so the painting starts again from the beginning.

Still frames, same 400×300 ink:

```sh
node host/cli.js frames --out /tmp/shan-shui-frames
```

## Config

All panel numbers live in `firmware/shan_shui_wallpaper/config.h`: resolution, bit depth, pins, SPI clock, strip length, chunk size, low-water mark, and the default scroll speed.

Host-side defaults are in `host/lib/config.js`: the 400×300 window, the shan-shui-inf vertical mapping (`800 / 1.142` user units onto 300 rows), the 2× raster, the luma where blank paper stops being dithered, the dither threshold, clouds in the open sky, and how many columns to render per pass. `SCENE_USER_H`, `INK_SCALE`, `PAPER_CUTOFF`, and `DITHER_THRESHOLD` can be overridden in the environment for one run.

## License

shan-shui-inf is Copyright (c) 2018 Lingdong Huang, MIT. The permission notice is `vendor/shan-shui-inf/LICENSE`, and `vendor/shan-shui-inf/index.html` is the unmodified file from the pinned commit. If you redistribute the generator, or a substantial part of it, keep that copyright line and the MIT permission text with it.

The host and the firmware are separately MIT; see `LICENSE` and `NOTICE`.

The ST7305 register values are the Waveshare init for this board, used so the panel comes up the way their examples do. Their example tree is a different project; this repository does not copy it.
