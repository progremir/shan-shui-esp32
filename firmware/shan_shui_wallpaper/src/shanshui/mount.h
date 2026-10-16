#pragma once
// Mount.* from shan-shui-inf. See core.h for porting rules.
#include <optional>

#include "core.h"

namespace ss {

// JavaScript `args`: an unset field means "undefined", so the function applies
// its own default (mountain's and flatMount's hei/wid defaults call
// Math.random). Field names match the JavaScript.
struct MountArgs {
  std::optional<real> hei, wid, tex;
  std::optional<bool> veg;
  std::function<std::string(real)> col;  // texture color; empty: undefined
};

struct FlatMountArgs {
  std::optional<real> hei, wid, tex, cho;
};

struct DistMountArgs {
  std::optional<real> hei, len, seg;
};

struct RockArgs {
  std::optional<real> hei, wid, tex, sha;
};

// The `grbd` bounds flatMount hands to flatDec.
struct Bound {
  real xmin = 0, xmax = 0, ymin = 0, ymax = 0;
};

Canvas mountain(real xoff, real yoff, real seed, const MountArgs& args = MountArgs());
// ret: 1. Draws the same (consuming the same randomness) and returns the
// ridge point lists instead.
std::vector<Pts> mountainPts(real xoff, real yoff, real seed, const MountArgs& args = MountArgs());
Canvas flatMount(real xoff, real yoff, real seed, const FlatMountArgs& args = FlatMountArgs());
Canvas flatDec(real xoff, real yoff, const Bound& grbd);
Canvas distMount(real xoff, real yoff, real seed, const DistMountArgs& args = DistMountArgs());
Canvas rock(real xoff, real yoff, real seed, const RockArgs& args = RockArgs());

}  // namespace ss
