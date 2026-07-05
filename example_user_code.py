from pathlib import Path
import sys


root = Path(__file__).resolve().parent
sys.path.insert(0, str(root / "lib"))

from dynamic_ops import Ops


ops = Ops(root / "build/libdynamic_ops_registry.so")
ops.load_plugin(root / "build/libcustom_ops.so")

print(ops.add_int(40, 2))
print(ops.add_float(0.5, 0.25))
