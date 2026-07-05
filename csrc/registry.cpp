#include "registry.h"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
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

}  // namespace

Registry& Registry::Instance() {
  static Registry registry;
  return registry;
}

void Registry::RegisterIntBinary(const std::string& name,
                                 const std::string& schema, IntBinaryFn fn) {
  std::lock_guard<std::mutex> lock(RegistryMutex());
  entries_.push_back(
      OpEntry{name, schema, OpKind::kIntBinary, reinterpret_cast<void*>(fn)});
}

void Registry::RegisterFloatBinary(const std::string& name,
                                   const std::string& schema,
                                   FloatBinaryFn fn) {
  std::lock_guard<std::mutex> lock(RegistryMutex());
  entries_.push_back(
      OpEntry{name, schema, OpKind::kFloatBinary, reinterpret_cast<void*>(fn)});
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

void Library::def(const std::string& schema, IntBinaryFn fn) {
  Registry::Instance().RegisterIntBinary(ParseOpName(schema), schema, fn);
}

void Library::def(const std::string& schema, FloatBinaryFn fn) {
  Registry::Instance().RegisterFloatBinary(ParseOpName(schema), schema, fn);
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

int call_op(const char* name, const DynamicValue* inputs,
            std::size_t input_count, DynamicValue* output, char* error,
            std::size_t error_size) {
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

  if (input_count != 2) {
    dynamic_ops::CopyMessage(std::string("op expects 2 inputs: ") + name, error,
                             error_size);
    return 0;
  }

  if (entry->kind == dynamic_ops::OpKind::kIntBinary) {
    if (inputs[0].kind != DYNAMIC_OPS_INT ||
        inputs[1].kind != DYNAMIC_OPS_INT) {
      dynamic_ops::CopyMessage(std::string("op expects int inputs: ") + name,
                               error, error_size);
      return 0;
    }
    auto fn = reinterpret_cast<dynamic_ops::IntBinaryFn>(entry->fn);
    output->kind = DYNAMIC_OPS_INT;
    output->int_value = fn(inputs[0].int_value, inputs[1].int_value);
    return 1;
  }

  if (entry->kind == dynamic_ops::OpKind::kFloatBinary) {
    if (inputs[0].kind != DYNAMIC_OPS_FLOAT ||
        inputs[1].kind != DYNAMIC_OPS_FLOAT) {
      dynamic_ops::CopyMessage(std::string("op expects float inputs: ") + name,
                               error, error_size);
      return 0;
    }
    auto fn = reinterpret_cast<dynamic_ops::FloatBinaryFn>(entry->fn);
    output->kind = DYNAMIC_OPS_FLOAT;
    output->float_value = fn(inputs[0].float_value, inputs[1].float_value);
    return 1;
  }

  dynamic_ops::CopyMessage(std::string("unsupported op kind: ") + name, error,
                           error_size);
  return 0;
}

int list_ops(char* output, std::size_t output_size) {
  std::ostringstream stream;
  const std::vector<dynamic_ops::OpEntry> entries =
      dynamic_ops::Registry::Instance().List();
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i > 0) {
      stream << "\n";
    }
    stream << entries[i].name << "\t"
           << (entries[i].kind == dynamic_ops::OpKind::kIntBinary
                   ? "int_binary"
                   : "float_binary")
           << "\t" << entries[i].schema;
  }
  dynamic_ops::CopyMessage(stream.str(), output, output_size);
  return static_cast<int>(entries.size());
}
}
