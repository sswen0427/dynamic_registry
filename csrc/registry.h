#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define DYNAMIC_OPS_CONCAT_IMPL(x, y) x##y
#define DYNAMIC_OPS_CONCAT(x, y) DYNAMIC_OPS_CONCAT_IMPL(x, y)

#define DYNAMIC_OPS_LIBRARY(name, m)                                    \
  static void DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_,             \
                                 __LINE__)(::dynamic_ops::Library & m); \
  static ::dynamic_ops::LibraryRegistrar DYNAMIC_OPS_CONCAT(            \
      dynamic_ops_library_registrar_, __LINE__)(                        \
      #name, DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_, __LINE__));  \
  static void DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_,             \
                                 __LINE__)(::dynamic_ops::Library & m)

namespace dynamic_ops {

using IntBinaryFn = int (*)(int, int);
using FloatBinaryFn = float (*)(float, float);

enum class OpKind {
  kUndefined,
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

  void Declare(const std::string& schema);
  void RegisterImpl(const std::string& name, IntBinaryFn fn);
  void RegisterImpl(const std::string& name, FloatBinaryFn fn);

  const OpEntry* Find(const std::string& name) const;
  std::vector<OpEntry> List() const;

 private:
  Registry() = default;

  std::vector<OpEntry> entries_;
};

class Library {
 public:
  explicit Library(const std::string& name);

  void def(const std::string& schema);
  void impl(const std::string& name, IntBinaryFn fn);
  void impl(const std::string& name, FloatBinaryFn fn);

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
