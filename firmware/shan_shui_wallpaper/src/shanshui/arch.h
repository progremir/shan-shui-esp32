#pragma once
// Arch.* and Man.* from shan-shui-inf. See core.h for porting rules.
#include <optional>

#include "core.h"

namespace ss {

// JavaScript `args`: an unset field means "undefined". Field names match.
struct ArchArgs {
  std::optional<real> hei, wid, rot, per, sto, sty, len, sca;
  std::optional<bool> rai, fli;
};

Canvas arch01(real xoff, real yoff, real seed, const ArchArgs& args = ArchArgs());
Canvas arch02(real xoff, real yoff, real seed, const ArchArgs& args = ArchArgs());
Canvas arch03(real xoff, real yoff, real seed, const ArchArgs& args = ArchArgs());
Canvas arch04(real xoff, real yoff, real seed, const ArchArgs& args = ArchArgs());
Canvas boat01(real xoff, real yoff, real seed, const ArchArgs& args = ArchArgs());
Canvas transmissionTower01(real xoff, real yoff, real seed, const ArchArgs& args = ArchArgs());

// Man.* (only Arch uses these; exposed for the reference test).
typedef std::function<Canvas(const Pt& p0, const Pt& p1, bool fli)> ManItem;
Canvas hat01(const Pt& p0, const Pt& p1, bool fli = false);
Canvas hat02(const Pt& p0, const Pt& p1, bool fli = false);
Canvas stick01(const Pt& p0, const Pt& p1, bool fli = false);
struct ManArgs {
  real sca = 0.5;
  ManItem hat;  // empty: hat01
  ManItem ite;  // empty: nothing
  bool fli = true;
  std::optional<std::vector<real>> ang;  // unset: random pose
  std::vector<real> len = {0, 30, 20, 30, 30, 30, 30, 30, 30};
};
Canvas man(real xoff, real yoff, const ManArgs& args = ManArgs());

}  // namespace ss
