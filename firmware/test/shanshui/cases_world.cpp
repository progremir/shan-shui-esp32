#include "cases.h"
#include "shanshui/world.h"

using namespace ss;

static std::string plan_text(const std::vector<Plan>& reg) {
  std::string s;
  for (auto& r : reg) s += r.tag + "@" + fixed(r.x, 6) + "," + fixed(r.y, 6) + ";";
  return s;
}

// The planner keeps state between calls (MEM.planmtx), so plan several
// consecutive chunks in one case, as chunkloader does.
SS_CASE("planner_wallpaper", "wallpaper") {
  World w;
  std::string s;
  for (int k = 0; k < 8; k++) s += plan_text(w.mountplanner(k * 512, (k + 1) * 512)) + "|";
  return s;
}

SS_CASE("planner_left", "abc") {
  World w;
  std::string s = plan_text(w.mountplanner(0, 512)) + "|";
  s += plan_text(w.mountplanner(-512, 0)) + "|";
  for (int k = 1; k < 6; k++) s += plan_text(w.mountplanner(k * 512, (k + 1) * 512)) + "|";
  return s;
}

static std::string chunks_text(const World& w) {
  std::string s;
  for (auto& c : w.chunks()) s += c.tag + "@" + fixed(c.x, 6) + "," + fixed(c.y, 6) + ":" + c.canv.svg() + "|";
  return s;
}

SS_CASE("chunks_wallpaper", "wallpaper") {
  World w;
  w.chunkloader(0, 1500);
  w.chunkloader(0, 3000);
  return chunks_text(w);
}

SS_CASE("chunks_x7", "x7") {
  World w;
  w.chunkloader(0, 2600);
  return chunks_text(w);
}
