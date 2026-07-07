#include <ATen/ATen.h>
#include <torch/library.h>

at::Tensor ScaleAndShiftCpu(const at::Tensor& input, double scale,
                            double shift) {
  return input * scale + shift;
}

TORCH_LIBRARY(custom_ops, module) {
  module.def("scale_and_shift(Tensor input, float scale, float shift) -> Tensor");
}

TORCH_LIBRARY_IMPL(custom_ops, CPU, module) {
  module.impl("scale_and_shift", ScaleAndShiftCpu);
}
