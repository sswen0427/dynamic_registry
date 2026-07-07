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

std::mutex& RegistryMutex();
std::string ParseOpName(const std::string& schema);
std::string QualifyName(const std::string& library, const std::string& name);
std::string QualifySchema(const std::string& library,
                          const std::string& schema);

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

std::mutex& RegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

void CopyMessage(const std::string& message, char* output,
                 std::size_t output_size) {
  if (output == nullptr || output_size == 0) {
    return;
  }
  const std::size_t bytes = std::min(output_size - 1, message.size());
  std::memcpy(output, message.data(), bytes);
  output[bytes] = '\0';
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

const char* KindName(OpKind kind) {
  if (kind == OpKind::kBoxed) {
    return "boxed";
  }
  return "undefined";
}

int CheckBinaryStack(const std::string& name, const Stack& stack,
                     int expected_kind, char* error, std::size_t error_size) {
  if (stack.size() != 2) {
    CopyMessage(std::string("op expects 2 inputs: ") + name, error, error_size);
    return 0;
  }
  if (stack[0].kind != expected_kind || stack[1].kind != expected_kind) {
    CopyMessage(std::string("op input type mismatch: ") + name, error,
                error_size);
    return 0;
  }
  return 1;
}

int RunBoxedKernel(const std::string& name, const OpEntry& entry,
                   const DynamicValue* inputs, std::size_t input_count,
                   DynamicValue* output, char* error, std::size_t error_size) {
  Stack stack(inputs, inputs + input_count);

  if (entry.schema.find("(int ") != std::string::npos) {
    if (!CheckBinaryStack(name, stack, DYNAMIC_OPS_INT, error, error_size)) {
      return 0;
    }
  } else if (entry.schema.find("(float ") != std::string::npos) {
    if (!CheckBinaryStack(name, stack, DYNAMIC_OPS_FLOAT, error, error_size)) {
      return 0;
    }
  }

  entry.kernel(&stack);

  if (stack.size() != 1) {
    CopyMessage(std::string("op expects 1 output: ") + name, error, error_size);
    return 0;
  }
  *output = stack[0];
  return 1;
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

int load_plugin(const char* path, char* error, std::size_t error_size) {
  if (path == nullptr) {
    dynamic_ops::CopyMessage("plugin path is null", error, error_size);
    return 0;
  }

  void* handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
  if (handle == nullptr) {
    dynamic_ops::CopyMessage(dlerror(), error, error_size);
    return 0;
  }
  return 1;
}

int call_op(const char* name, const dynamic_ops::DynamicValue* inputs,
            std::size_t input_count, dynamic_ops::DynamicValue* output,
            char* error, std::size_t error_size) {
  if (name == nullptr || inputs == nullptr || output == nullptr) {
    dynamic_ops::CopyMessage("invalid op call argument", error, error_size);
    return 0;
  }

  const OpEntry* entry = Registry::Instance().Find(name);
  if (entry == nullptr) {
    dynamic_ops::CopyMessage(std::string("op not found: ") + name, error,
                             error_size);
    return 0;
  }

  if (!entry->kernel) {
    dynamic_ops::CopyMessage(std::string("op has no implementation: ") + name,
                             error, error_size);
    return 0;
  }

  return RunBoxedKernel(name, *entry, inputs, input_count, output, error,
                        error_size);
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
  dynamic_ops::CopyMessage(stream.str(), output, output_size);
  return static_cast<int>(entries.size());
}
}

}  // namespace dynamic_ops
