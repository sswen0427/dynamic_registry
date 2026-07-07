from setuptools import setup
from torch.utils.cpp_extension import BuildExtension, CppExtension

setup(
    name="custom_ops_ext",
    ext_modules=[
        CppExtension(
            name="custom_ops_ext",
            sources=["custom_ops.cpp"],
        )
    ],
    cmdclass={"build_ext": BuildExtension},
)
