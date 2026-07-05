#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define DYNAMIC_OPS_CONCAT_IMPL(x, y) x##y
#define DYNAMIC_OPS_CONCAT(x, y) DYNAMIC_OPS_CONCAT_IMPL(x, y)

#define DYNAMIC_OPS_REGISTER_INT_BINARY(name, fn)              \
  static ::dynamic_ops::IntBinaryRegistrar DYNAMIC_OPS_CONCAT( \
      dynamic_ops_int_binary_registrar_, __COUNTER__)((name), (fn))

#define DYNAMIC_OPS_REGISTER_FLOAT_BINARY(name, fn)              \
  static ::dynamic_ops::FloatBinaryRegistrar DYNAMIC_OPS_CONCAT( \
      dynamic_ops_float_binary_registrar_, __COUNTER__)((name), (fn))

namespace dynamic_ops {

using IntBinaryFn = int (*)(int, int);
using FloatBinaryFn = float (*)(float, float);

enum class OpKind {
  kIntBinary,
  kFloatBinary,
};

struct OpEntry {
  std::string name;
  OpKind kind;
  void* fn = nullptr;
};

class Registry {
 public:
  static Registry& Instance();

  void RegisterIntBinary(const std::string& name, IntBinaryFn fn);
  void RegisterFloatBinary(const std::string& name, FloatBinaryFn fn);

  const OpEntry* Find(const std::string& name) const;
  std::vector<OpEntry> List() const;

 private:
  Registry() = default;

  std::vector<OpEntry> entries_;
};

class IntBinaryRegistrar {
 public:
  IntBinaryRegistrar(const std::string& name, IntBinaryFn fn);
};

class FloatBinaryRegistrar {
 public:
  FloatBinaryRegistrar(const std::string& name, FloatBinaryFn fn);
};

}  // namespace dynamic_ops

extern "C" {

enum DynamicScalarKind {
  DYNAMIC_OPS_INT = 1,
  DYNAMIC_OPS_FLOAT = 2,
};

struct DynamicValue {
  int kind;
  union {
    int int_value;
    float float_value;
  };
};

int load_plugin(const char* path, char* error, std::size_t error_size);
int call_op(const char* name, const DynamicValue* inputs,
            std::size_t input_count, DynamicValue* output, char* error,
            std::size_t error_size);
int list_ops(char* output, std::size_t output_size);
}
