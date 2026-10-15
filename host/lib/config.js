// Panel geometry matches the Waveshare ESP32-S3-RLCD-4.2 landscape mode.
export const PANEL_W = 400;
export const PANEL_H = 300;

// shan-shui-inf calcViewBox() uses this zoom. The live demo's vertical
// window is MEM.windy / zoom = 800 / 1.142 user units. Mapping that whole
// window onto 300 device rows keeps the same sky-to-foreground composition.
export const SOURCE_ZOOM = 1.142;
export const SCENE_USER_H = 800 / SOURCE_ZOOM;

export const PAPER = [244, 240, 230];
export const INK = [20, 20, 20];

// The panel is black or paper. The painting is rastered at INK_SCALE, then
// Floyd–Steinberg turns every tone darker than PAPER_CUTOFF into a dither,
// so a translucent stroke reads as gray. Luma at or above the cutoff stays
// blank paper.
export const INK_SCALE = 2;
export const PAPER_CUTOFF = 224;
export const DITHER_THRESHOLD = 128;

export const STRIP_RENDER = 160;
export const SCROLL_PX_PER_SEC = 6;
export const DEFAULT_SEED = "wallpaper";

export function columnStride(height = PANEL_H) {
  return Math.ceil(height / 8);
}
