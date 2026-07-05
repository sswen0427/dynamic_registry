#include "registry.h"

namespace {

int AddInt(int left, int right) { return left + right; }

float AddFloat(float left, float right) { return left + right; }

DYNAMIC_OPS_REGISTER_INT_BINARY("add_int", AddInt);
DYNAMIC_OPS_REGISTER_FLOAT_BINARY("add_float", AddFloat);

}  // namespace
