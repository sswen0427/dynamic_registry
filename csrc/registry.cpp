#include "registry.h"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace dynamic_ops {
namespace {

using Stack = detail::Stack;
using BoxedKernel = detail::BoxedKernel;

enum class OpKind {
  kUndefined,
  kBoxed,
};

std::mutex& RegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

std::string ParseOpName(const std::string& schema) {
  const std::size_t args_start = schema.find('(');
  if (args_start == std::string::npos || args_start == 0) {
    throw std::invalid_argument("invalid op schema: " + schema);
  }
  return schema.substr(0, args_start);
}

std::string QualifyName(const std::string& library, const std::string& name) {
  return library + "::" + name;
}

std::string QualifySchema(const std::string& library,
                          const std::string& schema) {
  const std::size_t args_start = schema.find('(');
  if (args_start == std::string::npos || args_start == 0) {
    throw std::invalid_argument("invalid op schema: " + schema);
  }
  return QualifyName(library, schema.substr(0, args_start)) +
         schema.substr(args_start);
}

struct OpEntry {
  std::string name;
  std::string schema;
  OpKind kind;
  BoxedKernel kernel;
};

class Registry {
 public:
  static Registry& Instance() {
    static Registry registry;
    return registry;
  }

  void Declare(const std::string& schema) {
    std::lock_guard<std::mutex> lock(RegistryMutex());
    entries_.push_back(OpEntry{ParseOpName(schema), schema, OpKind::kUndefined,
                               BoxedKernel{}});
  }

  void RegisterImpl(const std::string& name, BoxedKernel kernel) {
    std::lock_guard<std::mutex> lock(RegistryMutex());
    for (OpEntry& entry : entries_) {
      if (entry.name == name) {
        entry.kind = OpKind::kBoxed;
        entry.kernel = std::move(kernel);
        return;
      }
    }
    entries_.push_back(OpEntry{name, "", OpKind::kBoxed, std::move(kernel)});
  }

  const OpEntry* Find(const std::string& name) const {
    for (const OpEntry& entry : entries_) {
      if (entry.name == name) {
        return &entry;
      }
    }
    return nullptr;
  }

  std::vector<OpEntry> List() const {
    std::lock_guard<std::mutex> lock(RegistryMutex());
    return entries_;
  }

 private:
  Registry() = default;

  std::vector<OpEntry> entries_;
};

const char* KindName(OpKind kind) {
  if (kind == OpKind::kBoxed) {
    return "boxed";
  }
  return "undefined";
}

void RunBoxedKernel(const OpEntry& entry, const DynamicValue* inputs,
                    std::size_t input_count, DynamicValue* output) {
  Stack stack(inputs, inputs + input_count);
  entry.kernel(&stack);
  *output = stack[0];
}

}  // namespace

Library::Library(const std::string& name) : name_(name) {}

void Library::def(const std::string& schema) {
  Registry::Instance().Declare(QualifySchema(name_, schema));
}

void Library::RegisterBoxedImpl(const std::string& name, BoxedKernel kernel) {
  Registry::Instance().RegisterImpl(QualifyName(name_, name),
                                    std::move(kernel));
}

LibraryRegistrar::LibraryRegistrar(const std::string& name, InitFn init) {
  Library library(name);
  init(library);
}

extern "C" {

void load_plugin(const char* path) {
  dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}

int call_op(const char* name, const dynamic_ops::DynamicValue* inputs,
            std::size_t input_count, dynamic_ops::DynamicValue* output) {
  const OpEntry* entry = Registry::Instance().Find(name);
  RunBoxedKernel(*entry, inputs, input_count, output);
  return 1;
}

int list_ops(char* output, std::size_t output_size) {
  std::ostringstream stream;
  const std::vector<OpEntry> entries = Registry::Instance().List();
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i > 0) {
      stream << "\n";
    }
    stream << entries[i].name << "\t" << dynamic_ops::KindName(entries[i].kind)
           << "\t" << entries[i].schema;
  }
  const std::string text = stream.str();
  const std::size_t bytes = std::min(output_size - 1, text.size());
  std::memcpy(output, text.data(), bytes);
  output[bytes] = '\0';
  return static_cast<int>(entries.size());
}
}

}  // namespace dynamic_ops
