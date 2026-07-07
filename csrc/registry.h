#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#define DYNAMIC_OPS_CONCAT_IMPL(x, y) x##y
#define DYNAMIC_OPS_CONCAT(x, y) DYNAMIC_OPS_CONCAT_IMPL(x, y)

#define DYNAMIC_OPS_LIBRARY(library_name, module)                              \
  static void DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_,                    \
                                 __LINE__)(::dynamic_ops::Library & module);   \
  static ::dynamic_ops::LibraryRegistrar DYNAMIC_OPS_CONCAT(                   \
      dynamic_ops_library_registrar_, __LINE__)(                               \
      #library_name, DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_, __LINE__)); \
  static void DYNAMIC_OPS_CONCAT(dynamic_ops_library_init_,                    \
                                 __LINE__)(::dynamic_ops::Library & module)

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

namespace detail {

using Stack = std::vector<DynamicValue>;
using BoxedKernel = std::function<void(Stack*)>;

template <typename T>
T ReadValue(const DynamicValue& value);

template <>
inline int ReadValue<int>(const DynamicValue& value) {
  return value.int_value;
}

template <>
inline float ReadValue<float>(const DynamicValue& value) {
  return value.float_value;
}

template <typename T>
DynamicValue MakeValue(T value);

template <>
inline DynamicValue MakeValue<int>(int value) {
  DynamicValue output;
  output.kind = DYNAMIC_OPS_INT;
  output.int_value = value;
  return output;
}

template <>
inline DynamicValue MakeValue<float>(float value) {
  DynamicValue output;
  output.kind = DYNAMIC_OPS_FLOAT;
  output.float_value = value;
  return output;
}

template <typename Ret, typename... Args, std::size_t... Indices>
void CallUnboxed(Ret (*fn)(Args...), Stack* stack,
                 std::index_sequence<Indices...>) {
  Ret output = fn(ReadValue<Args>(stack->at(Indices))...);
  stack->clear();
  stack->push_back(MakeValue<Ret>(output));
}

template <typename Ret, typename... Args>
BoxedKernel MakeBoxedKernel(Ret (*fn)(Args...)) {
  return [fn](Stack* stack) {
    CallUnboxed(fn, stack, std::index_sequence_for<Args...>{});
  };
}

}  // namespace detail

class Library {
 public:
  explicit Library(const std::string& name);

  void def(const std::string& schema);

  template <typename Ret, typename... Args>
  void impl(const std::string& name, Ret (*fn)(Args...)) {
    RegisterBoxedImpl(name, detail::MakeBoxedKernel(fn));
  }

 private:
  void RegisterBoxedImpl(const std::string& name, detail::BoxedKernel kernel);

  std::string name_;
};

class LibraryRegistrar {
 public:
  using InitFn = void (*)(Library&);

  LibraryRegistrar(const std::string& name, InitFn init);
};

}  // namespace dynamic_ops

extern "C" {

void load_plugin(const char* path);
int call_op(const char* name, const dynamic_ops::DynamicValue* inputs,
            std::size_t input_count, dynamic_ops::DynamicValue* output);
int list_ops(char* output, std::size_t output_size);
}
