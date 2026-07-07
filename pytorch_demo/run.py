from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent
EXTENSION = next(ROOT.glob("**/custom_ops_ext*.so"))

torch.ops.load_library(str(EXTENSION))

left = torch.tensor([1, 2, 3])
right = torch.tensor([10, 20, 30])

print("registered op: torch.ops.custom_ops.add_tensor")
print("left =", left)
print("right =", right)
print("output =", torch.ops.custom_ops.add_tensor(left, right))
