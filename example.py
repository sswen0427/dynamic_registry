import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "lib"))

from dynamic_ops import Ops


def main():
    build = ROOT / "build"
    ops = Ops(build / "libdynamic_ops_registry.so")
    ops.load_plugin(build / "libcustom_ops.so")

    print("registered ops:")
    for op in ops.list_ops():
        print(" ", op)

    print("ops.custom_ops.add_int(40, 2) =", ops.custom_ops.add_int(40, 2))
    print(
        "ops.custom_ops.add_float(0.5, 0.25) =",
        ops.custom_ops.add_float(0.5, 0.25),
    )


if __name__ == "__main__":
    main()
