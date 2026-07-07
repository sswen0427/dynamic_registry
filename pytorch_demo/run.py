from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent
EXTENSION = next(ROOT.glob("custom_ops_ext*.so"))

input = torch.tensor([1.0, 2.0, 3.0])
scale = 10.0
shift = -3.0


def call_custom_op():
    return torch.ops.custom_ops.scale_and_shift(input, scale, shift)


print("input =", input)
print("scale =", scale)
print("shift =", shift)

print("\nbefore torch.ops.load_library:")
try:
    print("output =", call_custom_op())
except AttributeError as error:
    print("error =", error)

print("\nload extension:", EXTENSION)
torch.ops.load_library(str(EXTENSION))

print("\nafter torch.ops.load_library:")
print("output =", call_custom_op())
