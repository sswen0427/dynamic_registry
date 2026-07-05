# Dynamic Ops

This is a tiny PyTorch-free dynamic operator library.

The library provides:

```text
csrc/registry.h/.cpp        C++ registry and registrar API
lib/dynamic_ops/            Python loader and dynamic op lookup
```

The user provides:

```text
extension/custom_ops.cpp    C++ extension with user-defined registered funcs
example_user_code.py        Python code that loads the plugin and calls ops
```

## Build And Run

```bash
cd tools/dynamic_registry_demo
sh build.sh
python3 example_user_code.py
```

The build script is a thin wrapper around CMake:

```bash
cmake -S . -B build
cmake --build build
```

Expected output:

```text
42
0.75
```

## User Extension

As a user, write plain C++ functions and register them with macros from
`csrc/registry.h` in `extension/custom_ops.cpp`:

```cpp
#include "registry.h"

namespace {

int AddInt(int left, int right) { return left + right; }

float AddFloat(float left, float right) { return left + right; }

DYNAMIC_OPS_REGISTER_INT_BINARY("add_int", AddInt);
DYNAMIC_OPS_REGISTER_FLOAT_BINARY("add_float", AddFloat);

}  // namespace
```

`build.sh` compiles this file into `build/libcustom_ops.so`. When this
extension `.so` is loaded, global static registrar objects run and insert the
functions into the C++ registry.

## Python Usage

```python
from pathlib import Path
import sys

root = Path(__file__).resolve().parent
sys.path.insert(0, str(root / "lib"))

from dynamic_ops import Ops

ops = Ops(root / "build/libdynamic_ops_registry.so")
ops.load_plugin(root / "build/libcustom_ops.so")

print(ops.add_int(40, 2))
print(ops.add_float(0.5, 0.25))
```

Python does not import `add_int` or `add_float` from generated bindings. It
discovers them dynamically through `Ops.__getattr__`, which checks the C++
registry at runtime.

## Flow

```text
Python creates dynamic_ops.Ops
    |
loads libdynamic_ops_registry.so
    |
Python calls ops.load_plugin("libcustom_ops.so")
    |
dlopen loads user extension
    |
global static registrar objects in extension/custom_ops.cpp run
    |
add_int / add_float are inserted into the C++ registry
    |
Python ops.add_int triggers __getattr__
    |
__getattr__ checks C++ registry and returns a callable wrapper
    |
calling the wrapper invokes the C++ function pointer
```
