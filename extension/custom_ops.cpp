#include "registry.h"

namespace {

int AddInt(int left, int right) { return left + right; }

float AddFloat(float left, float right) { return left + right; }

DYNAMIC_OPS_LIBRARY(custom_ops, m) {
  m.def("add_int(int left, int right) -> int", AddInt);
  m.def("add_float(float left, float right) -> float", AddFloat);
}

}  // namespace
