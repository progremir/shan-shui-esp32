#pragma once
// Tree.tree01 ... Tree.tree08 from shan-shui-inf. See core.h for porting rules.
#include <optional>

#include "core.h"

namespace ss {

// JavaScript `args`: an unset field means "undefined", so the tree applies
// its own default. Field names match the JavaScript.
struct TreeArgs {
  std::optional<real> hei, wid, clu, noi, ang, len;
  std::optional<std::string> col;
  std::optional<Fn> ben;  // tree03, tree07: x offset as a function of height
};

Canvas tree01(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree02(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree03(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree04(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree05(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree06(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree07(real x, real y, const TreeArgs& args = TreeArgs());
Canvas tree08(real x, real y, const TreeArgs& args = TreeArgs());

}  // namespace ss
