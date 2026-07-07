#include "registry.h"

namespace {

int AddInt(int left, int right) { return left + right; }

float AddFloat(float left, float right) { return left + right; }

DYNAMIC_OPS_LIBRARY(custom_ops, module) {
  module.def("add_int(int left, int right) -> int");
  module.def("add_float(float left, float right) -> float");

  module.impl("add_int", AddInt);
  module.impl("add_float", AddFloat);
}

}  // namespace
