#!/bin/sh
# Build (and with a port, flash) the standalone wallpaper with arduino-cli.
#   firmware/build.sh                       compile only
#   firmware/build.sh /dev/cu.usbmodem2101  compile and upload
set -e
cd "$(dirname "$0")"
FQBN=esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PSRAM=opi,CPUFreq=240,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB
# The painter runs in float; keep literals float too (see src/shanshui/core.h).
FLAGS="compiler.cpp.extra_flags=-fsingle-precision-constant"
arduino-cli compile --fqbn "$FQBN" --build-property "$FLAGS" --output-dir build shan_shui_wallpaper
if [ -n "$1" ]; then
  arduino-cli upload -p "$1" --fqbn "$FQBN" --input-dir build shan_shui_wallpaper
fi
