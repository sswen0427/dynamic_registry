#pragma once

#include <cstddef>
#include <string>

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

class Library {
 public:
  explicit Library(const std::string& name);

  void def(const std::string& schema);
  void impl(const std::string& name, int (*fn)(int, int));
  void impl(const std::string& name, float (*fn)(float, float));

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

int load_plugin(const char* path, char* error, std::size_t error_size);
int call_op(const char* name, const dynamic_ops::DynamicValue* inputs,
            std::size_t input_count, dynamic_ops::DynamicValue* output,
            char* error, std::size_t error_size);
int list_ops(char* output, std::size_t output_size);
}
