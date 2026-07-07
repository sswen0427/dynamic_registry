#include <ATen/ATen.h>
#include <torch/library.h>

at::Tensor AddTensorCpu(const at::Tensor& left, const at::Tensor& right) {
  return left + right;
}

TORCH_LIBRARY(custom_ops, module) {
  module.def("add_tensor(Tensor left, Tensor right) -> Tensor");
}

TORCH_LIBRARY_IMPL(custom_ops, CPU, module) {
  module.impl("add_tensor", AddTensorCpu);
}
