#include "registry.h"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <sstream>

namespace dynamic_demo {
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

}  // namespace

Registry& Registry::Instance() {
  static Registry registry;
  return registry;
}

void Registry::RegisterIntBinary(const std::string& name, IntBinaryFn fn) {
  std::lock_guard<std::mutex> lock(RegistryMutex());
  entries_.push_back(OpEntry{name, OpKind::kIntBinary,
                             reinterpret_cast<void*>(fn)});
}

void Registry::RegisterFloatBinary(const std::string& name, FloatBinaryFn fn) {
  std::lock_guard<std::mutex> lock(RegistryMutex());
  entries_.push_back(OpEntry{name, OpKind::kFloatBinary,
                             reinterpret_cast<void*>(fn)});
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

IntBinaryRegistrar::IntBinaryRegistrar(const std::string& name,
                                       IntBinaryFn fn) {
  Registry::Instance().RegisterIntBinary(name, fn);
}

FloatBinaryRegistrar::FloatBinaryRegistrar(const std::string& name,
                                           FloatBinaryFn fn) {
  Registry::Instance().RegisterFloatBinary(name, fn);
}

}  // namespace dynamic_demo

extern "C" {

int load_plugin(const char* path, char* error, std::size_t error_size) {
  if (path == nullptr) {
    dynamic_demo::CopyMessage("plugin path is null", error, error_size);
    return 0;
  }

  void* handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
  if (handle == nullptr) {
    dynamic_demo::CopyMessage(dlerror(), error, error_size);
    return 0;
  }
  return 1;
}

int call_int_binary(const char* name, int left, int right, int* output,
                    char* error, std::size_t error_size) {
  if (name == nullptr || output == nullptr) {
    dynamic_demo::CopyMessage("invalid int op call argument", error,
                              error_size);
    return 0;
  }

  const dynamic_demo::OpEntry* entry =
      dynamic_demo::Registry::Instance().Find(name);
  if (entry == nullptr) {
    dynamic_demo::CopyMessage(std::string("op not found: ") + name, error,
                              error_size);
    return 0;
  }
  if (entry->kind != dynamic_demo::OpKind::kIntBinary) {
    dynamic_demo::CopyMessage(std::string("op is not int binary: ") + name,
                              error, error_size);
    return 0;
  }

  auto fn = reinterpret_cast<dynamic_demo::IntBinaryFn>(entry->fn);
  *output = fn(left, right);
  return 1;
}

int call_float_binary(const char* name, float left, float right, float* output,
                      char* error, std::size_t error_size) {
  if (name == nullptr || output == nullptr) {
    dynamic_demo::CopyMessage("invalid float op call argument", error,
                              error_size);
    return 0;
  }

  const dynamic_demo::OpEntry* entry =
      dynamic_demo::Registry::Instance().Find(name);
  if (entry == nullptr) {
    dynamic_demo::CopyMessage(std::string("op not found: ") + name, error,
                              error_size);
    return 0;
  }
  if (entry->kind != dynamic_demo::OpKind::kFloatBinary) {
    dynamic_demo::CopyMessage(std::string("op is not float binary: ") + name,
                              error, error_size);
    return 0;
  }

  auto fn = reinterpret_cast<dynamic_demo::FloatBinaryFn>(entry->fn);
  *output = fn(left, right);
  return 1;
}

int list_ops(char* output, std::size_t output_size) {
  std::ostringstream stream;
  const std::vector<dynamic_demo::OpEntry> entries =
      dynamic_demo::Registry::Instance().List();
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i > 0) {
      stream << "\n";
    }
    stream << entries[i].name << ":"
           << (entries[i].kind == dynamic_demo::OpKind::kIntBinary
                   ? "int_binary"
                   : "float_binary");
  }
  dynamic_demo::CopyMessage(stream.str(), output, output_size);
  return static_cast<int>(entries.size());
}

}
