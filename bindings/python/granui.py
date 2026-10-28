"""Тонкая обёртка C ABI Gran UI. / Thin wrapper of the Gran UI C ABI."""
import ctypes as C
import itertools
import os
import threading
from pathlib import Path
import sys

SIZE_MAX = C.c_size_t(-1).value

class _Options(C.Structure):
    _fields_ = [("size", C.c_uint32), ("backend", C.c_int32), ("debug", C.c_int32),
                ("warp", C.c_int32), ("watch", C.c_int32), ("no_vsync", C.c_int32), ("exit_after", C.c_double)]

class _Update(C.Structure):
    _fields_ = [("target", C.c_char_p), ("property", C.c_char_p), ("value", C.c_char_p)]

class _Field(C.Structure):
    _fields_ = [("name", C.c_char_p), ("value", C.c_char_p)]

class _Record(C.Structure):
    _fields_ = [("index", C.c_size_t), ("fields", C.POINTER(_Field)), ("count", C.c_size_t)]

class _Event(C.Structure):
    _fields_ = [("target", C.c_char_p), ("model", C.c_char_p), ("index", C.c_size_t),
                ("text", C.c_char_p), ("option", C.c_char_p), ("message", C.c_char_p),
                ("value", C.c_double), ("checked", C.c_int32)]

_Action = C.CFUNCTYPE(None, C.c_void_p, C.POINTER(_Event))
_Task = C.CFUNCTYPE(None, C.c_void_p)

class _Timer(C.Structure):
    _fields_ = [("seconds", C.c_double), ("repeat", C.c_int32), ("callback", _Task), ("user", C.c_void_p)]

def _encode(value):
    if value is None:
        return None
    value = str(value)
    if "\0" in value:
        raise ValueError("embedded NUL")
    return value.encode("utf-8")

def _configure(lib):
    p, s, z = C.c_void_p, C.c_char_p, C.c_size_t
    declarations = {
        "abi_version": (C.c_uint32, []), "last_error": (s, []), "string_free": (None, [p]),
        "app_create": (C.c_int32, [C.POINTER(_Options), C.POINTER(p)]),
        "app_destroy": (C.c_int32, [p]), "app_load": (C.c_int32, [p, s, C.c_int32]),
        "app_run": (C.c_int32, [p, C.POINTER(C.c_int32)]),
        "app_on_action": (C.c_int32, [p, s, _Action, p]), "app_on_message": (C.c_int32, [p, s, _Action, p]),
        "app_post": (C.c_int32, [p, _Task, p]),
        "app_timer": (C.c_int32, [p, C.POINTER(_Timer), C.POINTER(C.c_uint64)]),
        "app_clear_timer": (C.c_int32, [p, C.c_uint64]),
        "app_read": (C.c_int32, [p, s, s, C.POINTER(p)]),
        "app_apply": (C.c_int32, [p, C.POINTER(_Update), z]), "app_command": (C.c_int32, [p, s, s, s]),
        "model_count": (C.c_int32, [p, s, C.POINTER(z)]),
        "model_read": (C.c_int32, [p, s, z, s, C.POINTER(p)]),
        "model_insert": (C.c_int32, [p, s, C.POINTER(_Record)]),
        "model_set": (C.c_int32, [p, s, C.POINTER(_Record)]),
        "model_remove": (C.c_int32, [p, s, z]), "model_clear": (C.c_int32, [p, s]),
    }
    for name, (result, args) in declarations.items():
        function = getattr(lib, "gran_" + name)
        function.restype, function.argtypes = result, args
    if lib.gran_abi_version() != 1:
        raise RuntimeError("unsupported Gran UI ABI")

class Application:
    """Владелец приложения, без финализатора другого потока. / Application owner without a cross-thread finalizer."""
    def __init__(self, library=None, backend=0, exit_after=0, warp=False):
        name = "gran_c.dll" if os.name == "nt" else ("libgran_c.dylib" if sys.platform == "darwin" else "libgran_c.so")
        path = library or os.environ.get("GRAN_UI_LIBRARY") or Path(__file__).resolve().parents[2] / "bin" / name
        self._lib = C.CDLL(str(path))
        _configure(self._lib)
        self._app, self._callbacks, self._error = C.c_void_p(), [], None
        # Один переходник на все задачи и таймеры: user — номер задачи в таблице (иначе каждый
        # post оставлял бы свой переходник до close). / One trampoline for all tasks and timers: user is
        # the task number in the table (otherwise every post would keep its own trampoline until close).
        self._tasks, self._timer_keys, self._key_timers = {}, {}, {}
        self._task_lock, self._task_keys = threading.Lock(), itertools.count(1)
        self._task_entry = _Task(self._run_task)
        options = _Options(C.sizeof(_Options), backend, 0, warp, 0, 0, exit_after)
        self._check(self._lib.gran_app_create(C.byref(options), C.byref(self._app)))

    def _check(self, status):
        if status:
            raise RuntimeError(self._lib.gran_last_error().decode("utf-8", errors="replace"))

    def _call(self, name, *args):
        if not self._app:
            raise RuntimeError("application is closed")
        self._check(getattr(self._lib, "gran_" + name)(self._app, *args))

    def _string(self, name, *args):
        output = C.c_void_p()
        self._call(name, *args, C.byref(output))
        try:
            return C.string_at(output).decode("utf-8")
        finally:
            self._lib.gran_string_free(output)

    def load_string(self, xml):
        self._call("app_load", _encode(xml), 0)

    def load_file(self, path):
        self._call("app_load", _encode(path), 1)

    def read(self, target, property):
        return self._string("app_read", _encode(target), _encode(property))

    def apply(self, updates):
        values = [tuple(_encode(value) for value in row) for row in updates]
        data = (_Update * len(values))(*(_Update(*row) for row in values))
        self._call("app_apply", data, len(data))

    def command(self, name, target=None, argument=None):
        self._call("app_command", _encode(name), _encode(target), _encode(argument))

    def _protect(self, callback, *args):
        # Исключение не должно пересечь callback ctypes.
        # An exception must never escape a ctypes callback.
        try:
            callback(*args)
        except BaseException as error:
            if self._error is None:
                self._error = error
            self._lib.gran_app_command(self._app, b"close", None, None)

    def _on(self, kind, name, callback):
        def invoke(user, pointer):
            def deliver():
                event = pointer.contents
                snapshot = {field: (getattr(event, field) or b"").decode("utf-8")
                            for field in ("target", "model", "text", "option", "message")}
                snapshot.update(index=event.index, value=event.value, checked=bool(event.checked))
                callback(snapshot)
            self._protect(deliver)
        wrapped = _Action(invoke)
        self._callbacks.append(wrapped)
        self._call(kind, _encode(name), wrapped, None)

    def on_action(self, name, callback):
        self._on("app_on_action", name, callback)

    def on_message(self, name, callback):
        self._on("app_on_message", name, callback)

    def _add_task(self, callback, repeat):
        if not callable(callback):
            raise TypeError("callback must be callable")
        key = next(self._task_keys)
        with self._task_lock:
            self._tasks[key] = (callback, repeat)
        return key

    def _drop_task(self, key):
        with self._task_lock:
            self._tasks.pop(key, None)
            timer = self._key_timers.pop(key, None)
            if timer is not None:
                self._timer_keys.pop(timer, None)

    def _run_task(self, user):
        key = user or 0
        with self._task_lock:
            entry = self._tasks.get(key)
        if entry is None:
            return
        if not entry[1]:
            self._drop_task(key)
        self._protect(entry[0])

    def post(self, callback):
        key = self._add_task(callback, False)
        try:
            self._call("app_post", self._task_entry, C.c_void_p(key))
        except BaseException:
            self._drop_task(key)
            raise

    def timer(self, seconds, callback, repeat=False):
        key = self._add_task(callback, bool(repeat))
        timer = _Timer(seconds, repeat, self._task_entry, C.c_void_p(key))
        result = C.c_uint64()
        try:
            self._call("app_timer", C.byref(timer), C.byref(result))
        except BaseException:
            self._drop_task(key)
            raise
        with self._task_lock:
            self._timer_keys[result.value], self._key_timers[key] = key, result.value
        return result.value

    def clear_timer(self, timer):
        self._call("app_clear_timer", timer)
        with self._task_lock:
            key = self._timer_keys.get(timer)
        if key is not None:
            self._drop_task(key)

    def pending_tasks(self):
        """Число задач и таймеров, ждущих вызова (для проверок). / The number of tasks and timers waiting for a call (for checks)."""
        with self._task_lock:
            return len(self._tasks)

    def model_count(self, model):
        count = C.c_size_t()
        self._call("model_count", _encode(model), C.byref(count))
        return count.value

    def model_read(self, model, index, field):
        return self._string("model_read", _encode(model), index, _encode(field))

    def _record(self, operation, model, index, fields):
        values = [(_encode(key), _encode(value)) for key, value in fields.items()]
        data = (_Field * len(values))(*(_Field(*row) for row in values))
        record = _Record(index, data, len(data))
        self._call(operation, _encode(model), C.byref(record))

    def model_insert(self, model, fields, index=SIZE_MAX):
        self._record("model_insert", model, index, fields)

    def model_set(self, model, index, fields):
        self._record("model_set", model, index, fields)

    def model_remove(self, model, index):
        self._call("model_remove", _encode(model), index)

    def model_clear(self, model):
        self._call("model_clear", _encode(model))

    def run(self):
        result = C.c_int32()
        try:
            self._call("app_run", C.byref(result))
        finally:
            if self._error is not None:
                error, self._error = self._error, None
                raise error
        return result.value

    def close(self):
        if self._app:
            self._call("app_destroy")
            self._app = C.c_void_p()
            self._callbacks.clear()
            with self._task_lock:
                self._tasks.clear()
                self._timer_keys.clear()
                self._key_timers.clear()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
