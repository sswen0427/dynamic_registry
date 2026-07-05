#include "registry.h"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace dynamic_ops {
namespace {

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

const char* KindName(OpKind kind) {
  if (kind == OpKind::kBoxed) {
    return "boxed";
  }
  return "undefined";
}

int CheckBinaryInputs(const std::string& name, const DynamicValue* inputs,
                      std::size_t input_count, int expected_kind, char* error,
                      std::size_t error_size) {
  if (input_count != 2) {
    CopyMessage(std::string("op expects 2 inputs: ") + name, error, error_size);
    return 0;
  }
  if (inputs[0].kind != expected_kind || inputs[1].kind != expected_kind) {
    CopyMessage(std::string("op input type mismatch: ") + name, error,
                error_size);
    return 0;
  }
  return 1;
}

}  // namespace

Registry& Registry::Instance() {
  static Registry registry;
  return registry;
}

void Registry::Declare(const std::string& schema) {
  std::lock_guard<std::mutex> lock(RegistryMutex());
  entries_.push_back(
      OpEntry{ParseOpName(schema), schema, OpKind::kUndefined, BoxedKernel{}});
}

void Registry::RegisterImpl(const std::string& name, BoxedKernel kernel) {
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

const OpEntry* Registry::Find(const std::string& name) const {
  for (const OpEntry& entry : entries_) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

std::vector<OpEntry> Registry::List() const {
  std::lock_guard<std::mutex> lock(RegistryMutex());
  return entries_;
}

Library::Library(const std::string& name) : name_(name) {}

void Library::def(const std::string& schema) {
  Registry::Instance().Declare(schema);
}

void Library::impl(const std::string& name, int (*fn)(int, int)) {
  Registry::Instance().RegisterImpl(
      name,
      [name, fn](const DynamicValue* inputs, std::size_t input_count,
                 DynamicValue* output, char* error, std::size_t error_size) {
        if (!CheckBinaryInputs(name, inputs, input_count, DYNAMIC_OPS_INT,
                               error, error_size)) {
          return 0;
        }
        output->kind = DYNAMIC_OPS_INT;
        output->int_value = fn(inputs[0].int_value, inputs[1].int_value);
        return 1;
      });
}

void Library::impl(const std::string& name, float (*fn)(float, float)) {
  Registry::Instance().RegisterImpl(
      name,
      [name, fn](const DynamicValue* inputs, std::size_t input_count,
                 DynamicValue* output, char* error, std::size_t error_size) {
        if (!CheckBinaryInputs(name, inputs, input_count, DYNAMIC_OPS_FLOAT,
                               error, error_size)) {
          return 0;
        }
        output->kind = DYNAMIC_OPS_FLOAT;
        output->float_value = fn(inputs[0].float_value, inputs[1].float_value);
        return 1;
      });
}

LibraryRegistrar::LibraryRegistrar(const std::string& name, InitFn init) {
  Library library(name);
  init(library);
}

}  // namespace dynamic_ops

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

  const dynamic_ops::OpEntry* entry =
      dynamic_ops::Registry::Instance().Find(name);
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

  return entry->kernel(inputs, input_count, output, error, error_size);
}

int list_ops(char* output, std::size_t output_size) {
  std::ostringstream stream;
  const std::vector<dynamic_ops::OpEntry> entries =
      dynamic_ops::Registry::Instance().List();
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
