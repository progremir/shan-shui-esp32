// Desktop preview of the on-device painter.
//   render <seed> <columns> <out.pgm> [tile_w] [sub]
// Writes the 1-bit panel image (white = paper) and prints timing and memory.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <string>
#include <vector>

#include "shanshui/world.h"

int main(int argc, char** argv) {
  std::string seed = argc > 1 ? argv[1] : "wallpaper";
  int cols = argc > 2 ? atoi(argv[2]) : 1600;
  const char* out = argc > 3 ? argv[3] : "/tmp/ss_render.pgm";
  int tile = argc > 4 ? atoi(argv[4]) : 40;
  int sub = argc > 5 ? atoi(argv[5]) : 3;
  const int H = 300, stride = (H + 7) / 8;
  ss::World w;
  w.init(tile, H, sub);
  w.start(seed);
  if (getenv("AHEAD")) w.set_ahead(atof(getenv("AHEAD")));
  std::vector<uint8_t> img((size_t)cols * H, 255), packed((size_t)tile * stride);
  size_t max_pts = 0, max_bytes = 0;
  clock_t t0 = clock();
  for (int x = 0; x < cols; x += tile) {
    w.render(x, packed.data());
    if (w.point_count() > max_pts) max_pts = w.point_count();
    if (w.bytes() > max_bytes) max_bytes = w.bytes();
    for (int c = 0; c < tile && x + c < cols; c++)
      for (int y = 0; y < H; y++)
        img[(size_t)y * cols + x + c] = (packed[c * stride + (y >> 3)] >> (7 - (y & 7))) & 1 ? 255 : 0;
  }
  double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
  printf("late by %.0f user units; %zu of %zu polylines dropped for NaN; ", w.late(), ss::g_nan_prims, ss::g_prims);
  printf("%d columns in %.2f s (%.1f cols/s), peak %zu points, %.2f MB, %zu chunks now\n", cols, secs,
         cols / secs, max_pts, max_bytes / 1048576.0, w.chunks().size());
  FILE* f = fopen(out, "wb");
  fprintf(f, "P5 %d %d 255\n", cols, H);
  fwrite(img.data(), 1, img.size(), f);
  fclose(f);
}
