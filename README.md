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
example.py                  Python code that loads the plugin and calls ops
```

## Build And Run

```bash
cd tools/dynamic_registry_demo
bash build.sh
python3 example.py
```

For a step-by-step presentation plan, see [DEMO_GUIDE.md](DEMO_GUIDE.md).

There is also a minimal real PyTorch custom operator demo in
[`pytorch_demo/`](pytorch_demo/). It uses PyTorch's own `TORCH_LIBRARY`,
`TORCH_LIBRARY_IMPL`, and `torch.ops.load_library(...)` path.

The build script is a thin wrapper around CMake:

```bash
cmake -S . -B build
cmake --build build
```

Expected output:

```text
registered ops:
  custom_ops::add_int	boxed	custom_ops::add_int(int left, int right) -> int
  custom_ops::add_float	boxed	custom_ops::add_float(float left, float right) -> float
ops.custom_ops.add_int(40, 2) = 42
ops.custom_ops.add_float(0.5, 0.25) = 0.75
```

## User Extension

As a user, write plain C++ functions and register them in a library block from
`csrc/registry.h` in `extension/custom_ops.cpp`:

```cpp
#include "registry.h"

namespace {

int AddInt(int left, int right) { return left + right; }

float AddFloat(float left, float right) { return left + right; }

DYNAMIC_OPS_LIBRARY(custom_ops, module) {
  module.def("add_int(int left, int right) -> int");
  module.def("add_float(float left, float right) -> float");

  module.impl("add_int", AddInt);
  module.impl("add_float", AddFloat);
}

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

build = root / "build"
ops = Ops(build / "libdynamic_ops_registry.so")
ops.load_plugin(build / "libcustom_ops.so")

for op in ops.list_ops():
    print(op)

print(ops.custom_ops.add_int(40, 2))
print(ops.custom_ops.add_float(0.5, 0.25))
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
custom_ops::add_int / custom_ops::add_float schemas are inserted into the C++ registry
    |
Python ops.custom_ops.add_int triggers __getattr__
    |
__getattr__ checks C++ registry and returns a callable wrapper
    |
calling the wrapper passes arguments as DynamicValue[]
    |
call_op builds a Stack from the inputs
    |
boxed kernel mutates Stack from inputs to outputs
    |
call_op returns stack[0] as the DynamicValue result
```
