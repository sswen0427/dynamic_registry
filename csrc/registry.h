#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define DYNAMIC_OPS_CONCAT_IMPL(x, y) x##y
#define DYNAMIC_OPS_CONCAT(x, y) DYNAMIC_OPS_CONCAT_IMPL(x, y)

#define DYNAMIC_OPS_LIBRARY(name, m) \
  DYNAMIC_OPS_LIBRARY_IMPL(name, m, __COUNTER__)

#define DYNAMIC_OPS_LIBRARY_IMPL(name, m, uid)                     \
  static void DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_,        \
                                 uid)(::dynamic_ops::Library & m); \
  static ::dynamic_ops::LibraryRegistrar DYNAMIC_OPS_CONCAT(       \
      dynamic_ops_library_registrar_, uid)(                        \
      #name, DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_, uid));  \
  static void DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_,        \
                                 uid)(::dynamic_ops::Library & m)

namespace dynamic_ops {

using IntBinaryFn = int (*)(int, int);
using FloatBinaryFn = float (*)(float, float);

enum class OpKind {
  kIntBinary,
  kFloatBinary,
};

struct OpEntry {
  std::string name;
  std::string schema;
  OpKind kind;
  void* fn = nullptr;
};

class Registry {
 public:
  static Registry& Instance();

  void RegisterIntBinary(const std::string& name, const std::string& schema,
                         IntBinaryFn fn);
  void RegisterFloatBinary(const std::string& name, const std::string& schema,
                           FloatBinaryFn fn);

  const OpEntry* Find(const std::string& name) const;
  std::vector<OpEntry> List() const;

 private:
  Registry() = default;

  std::vector<OpEntry> entries_;
};

class Library {
 public:
  explicit Library(const std::string& name);

  void def(const std::string& schema, IntBinaryFn fn);
  void def(const std::string& schema, FloatBinaryFn fn);

 private:
  std::string name_;
};

class LibraryRegistrar {
 public:
  using InitFn = void (*)(Library&);

  LibraryRegistrar(const std::string& name, InitFn init);
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
