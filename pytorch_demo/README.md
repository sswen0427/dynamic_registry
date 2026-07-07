# PyTorch Custom Op Demo

This directory is a minimal real PyTorch custom operator demo.

It shows the same idea as the tiny local registry demo, but using PyTorch's own
dispatcher:

```text
C++ function
  -> TORCH_LIBRARY schema
  -> TORCH_LIBRARY_IMPL CPU kernel
  -> compiled .so
  -> torch.ops.load_library(...)
  -> torch.ops.custom_ops.add_tensor(...)
```

## Build

```bash
cd pytorch_demo
/usr/bin/python3 build.py
```

This demo uses `/usr/bin/python3` on the current machine because PyTorch is
installed in that Python 3.9 environment. If your `python3` can import `torch`,
use `python3 build.py` instead.

`setup.py` is also provided for the standard setuptools-style extension build,
if your Python environment has `setuptools`:

```bash
python3 setup.py build_ext --inplace
```

## Run

```bash
/usr/bin/python3 run.py
```

Expected output:

```text
registered op: torch.ops.custom_ops.add_tensor
left = tensor([1, 2, 3])
right = tensor([10, 20, 30])
output = tensor([11, 22, 33])
```

## Key Code

```cpp
TORCH_LIBRARY(custom_ops, module) {
  module.def("add_tensor(Tensor left, Tensor right) -> Tensor");
}

TORCH_LIBRARY_IMPL(custom_ops, CPU, module) {
  module.impl("add_tensor", AddTensorCpu);
}
```

`TORCH_LIBRARY` registers the operator schema. `TORCH_LIBRARY_IMPL` registers a
kernel implementation for a dispatch key, here `CPU`.
