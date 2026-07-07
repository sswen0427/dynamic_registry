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
  -> torch.ops.custom_ops.scale_and_shift(...)
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
input = tensor([1., 2., 3.])
scale = 10.0
shift = -3.0

before torch.ops.load_library:
error = '_OpNamespace' object has no attribute 'scale_and_shift'

load extension: /path/to/pytorch_demo/build/custom_ops_ext.so

after torch.ops.load_library:
output = tensor([ 7., 17., 27.])
```

## Key Code

```cpp
TORCH_LIBRARY(custom_ops, module) {
  module.def("scale_and_shift(Tensor input, float scale, float shift) -> Tensor");
}

TORCH_LIBRARY_IMPL(custom_ops, CPU, module) {
  module.impl("scale_and_shift", ScaleAndShiftCpu);
}
```

`TORCH_LIBRARY` registers the operator schema. `TORCH_LIBRARY_IMPL` registers a
kernel implementation for a dispatch key, here `CPU`.
