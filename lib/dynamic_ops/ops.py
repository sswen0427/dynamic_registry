import ctypes
from pathlib import Path
from typing import Union


INT = 1
FLOAT = 2


class _DynamicValueData(ctypes.Union):
    _fields_ = [
        ("int_value", ctypes.c_int),
        ("float_value", ctypes.c_float),
    ]


class _DynamicValue(ctypes.Structure):
    _anonymous_ = ("data",)
    _fields_ = [
        ("kind", ctypes.c_int),
        ("data", _DynamicValueData),
    ]


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
        registered = {item.split("\t", 1)[0]: item for item in self.list_ops()}
        if name not in registered:
            raise AttributeError(f"op not found: {name}")

        _, kind, _ = registered[name].split("\t", 2)
        return lambda *args: self._call(name, kind, args)

    def _configure_c_api(self):
        self._lib.load_plugin.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]
        self._lib.load_plugin.restype = ctypes.c_int

        self._lib.call_op.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(_DynamicValue),
            ctypes.c_size_t,
            ctypes.POINTER(_DynamicValue),
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]
        self._lib.call_op.restype = ctypes.c_int

        self._lib.list_ops.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        self._lib.list_ops.restype = ctypes.c_int

    def _call(self, name, kind, args):
        inputs = self._pack_inputs(kind, args)
        output = _DynamicValue()
        error = ctypes.create_string_buffer(1024)
        ok = self._lib.call_op(
            name.encode(),
            inputs,
            len(inputs),
            ctypes.byref(output),
            error,
            len(error),
        )
        if not ok:
            raise RuntimeError(error.value.decode())
        return self._unpack_output(output)

    def _pack_inputs(self, kind, args):
        if kind not in ("int_binary", "float_binary"):
            raise AttributeError(f"unsupported op kind: {kind}")

        values = (_DynamicValue * len(args))()
        for index, arg in enumerate(args):
            if kind == "int_binary":
                values[index].kind = INT
                values[index].int_value = int(arg)
            else:
                values[index].kind = FLOAT
                values[index].float_value = float(arg)
        return values

    def _unpack_output(self, output):
        if output.kind == INT:
            return output.int_value
        if output.kind == FLOAT:
            return output.float_value
        raise RuntimeError(f"unsupported output kind: {output.kind}")
