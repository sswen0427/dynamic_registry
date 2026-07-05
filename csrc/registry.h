#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define DYNAMIC_OPS_CONCAT_IMPL(x, y) x##y
#define DYNAMIC_OPS_CONCAT(x, y) DYNAMIC_OPS_CONCAT_IMPL(x, y)

#define DYNAMIC_OPS_REGISTER_INT_BINARY(name, fn)                     \
  static ::dynamic_demo::IntBinaryRegistrar DYNAMIC_OPS_CONCAT(       \
      dynamic_ops_int_binary_registrar_, __COUNTER__)((name), (fn))

#define DYNAMIC_OPS_REGISTER_FLOAT_BINARY(name, fn)                   \
  static ::dynamic_demo::FloatBinaryRegistrar DYNAMIC_OPS_CONCAT(     \
      dynamic_ops_float_binary_registrar_, __COUNTER__)((name), (fn))

namespace dynamic_demo {

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

}  // namespace dynamic_demo

extern "C" {

int load_plugin(const char* path, char* error, std::size_t error_size);
int call_int_binary(const char* name, int left, int right, int* output,
                    char* error, std::size_t error_size);
int call_float_binary(const char* name, float left, float right, float* output,
                      char* error, std::size_t error_size);
int list_ops(char* output, std::size_t output_size);

}
