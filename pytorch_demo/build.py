from pathlib import Path
import subprocess

from torch.utils.cpp_extension import include_paths, library_paths


ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / "build"
BUILD_DIR.mkdir(exist_ok=True)

include_flags = [f"-I{path}" for path in include_paths()]
library_dirs = library_paths()
library_flags = [f"-L{path}" for path in library_dirs]
rpath_flags = [f"-Wl,-rpath,{path}" for path in library_dirs]

command = [
    "clang++",
    "-std=c++17",
    "-O2",
    "-Wno-deprecated-declarations",
    "-Wno-deprecated-builtins",
    "-dynamiclib",
    "-undefined",
    "dynamic_lookup",
    str(ROOT / "custom_ops.cpp"),
    "-o",
    str(BUILD_DIR / "custom_ops_ext.so"),
    *include_flags,
    *library_flags,
    *rpath_flags,
    "-ltorch",
    "-ltorch_cpu",
    "-lc10",
]

print(" ".join(command))
subprocess.run(command, check=True)
