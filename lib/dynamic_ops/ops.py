import ctypes
from pathlib import Path
from typing import Union


class Ops:
    def __init__(self, registry_so: Union[str, Path]):
        self._lib = ctypes.CDLL(str(registry_so))
        self._configure_c_api()

    def load_plugin(self, plugin_so: Union[str, Path]):
        error = ctypes.create_string_buffer(1024)
        ok = self._lib.load_plugin(str(plugin_so).encode(), error, len(error))
        if not ok:
            raise RuntimeError(error.value.decode())

    def list_ops(self):
        output = ctypes.create_string_buffer(4096)
        self._lib.list_ops(output, len(output))
        text = output.value.decode()
        return [] if not text else text.splitlines()

    def __getattr__(self, name):
        registered = {item.split(":", 1)[0]: item for item in self.list_ops()}
        if name not in registered:
            raise AttributeError(f"op not found: {name}")

        _, kind = registered[name].split(":", 1)
        if kind == "int_binary":
            return lambda left, right: self._call_int(name, left, right)
        if kind == "float_binary":
            return lambda left, right: self._call_float(name, left, right)
        raise AttributeError(f"unsupported op kind: {kind}")

    def _configure_c_api(self):
        self._lib.load_plugin.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]
        self._lib.load_plugin.restype = ctypes.c_int

        self._lib.call_int_binary.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int),
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]
        self._lib.call_int_binary.restype = ctypes.c_int

        self._lib.call_float_binary.argtypes = [
            ctypes.c_char_p,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.POINTER(ctypes.c_float),
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]
        self._lib.call_float_binary.restype = ctypes.c_int

        self._lib.list_ops.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        self._lib.list_ops.restype = ctypes.c_int

    def _call_int(self, name, left, right):
        output = ctypes.c_int()
        error = ctypes.create_string_buffer(1024)
        ok = self._lib.call_int_binary(
            name.encode(),
            int(left),
            int(right),
            ctypes.byref(output),
            error,
            len(error),
        )
        if not ok:
            raise RuntimeError(error.value.decode())
        return output.value

    def _call_float(self, name, left, right):
        output = ctypes.c_float()
        error = ctypes.create_string_buffer(1024)
        ok = self._lib.call_float_binary(
            name.encode(),
            float(left),
            float(right),
            ctypes.byref(output),
            error,
            len(error),
        )
        if not ok:
            raise RuntimeError(error.value.decode())
        return output.value
