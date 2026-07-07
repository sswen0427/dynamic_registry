from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent
EXTENSION = next(ROOT.glob("**/custom_ops_ext*.so"))

torch.ops.load_library(str(EXTENSION))

input = torch.tensor([1.0, 2.0, 3.0])
scale = 10.0
shift = -3.0

print("registered op: torch.ops.custom_ops.scale_and_shift")
print("input =", input)
print("scale =", scale)
print("shift =", shift)
print("output =", torch.ops.custom_ops.scale_and_shift(input, scale, shift))
