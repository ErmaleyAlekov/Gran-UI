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

# Модели 1.7: столбцы, источник из файла, представление, строка плоского дерева / Models 1.7: columns, a file source, a view, a flat tree row
class _Column(C.Structure):
    _fields_ = [("name", C.c_char_p), ("title", C.c_char_p), ("type", C.c_int32)]

class _Source(C.Structure):
    _fields_ = [("size", C.c_uint32), ("format", C.c_int32), ("path", C.c_char_p), ("select", C.c_char_p),
                ("children", C.c_char_p), ("fetch", C.c_size_t), ("fields", C.POINTER(_Field)), ("field_count", C.c_size_t)]

class _View(C.Structure):
    _fields_ = [("size", C.c_uint32), ("kind", C.c_int32), ("name", C.c_char_p), ("source", C.c_char_p),
                ("sort", C.c_char_p), ("filter", C.c_char_p), ("mode", C.c_int32), ("expanded", C.c_size_t)]

class _TreeRow(C.Structure):
    _fields_ = [("node", C.c_uint64), ("depth", C.c_size_t), ("expanded", C.c_int32), ("has_children", C.c_int32)]

_FetchFn = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p)
_TreeFetchFn = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p, C.c_uint64)
_SelectionFn = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p)
_COLUMNS = {"text": 0, "number": 1, "date": 2, "boolean": 3}
_FORMATS = {"auto": 0, "json": 1, "xml": 2}
_MODES = {"none": 0, "single": 1, "multi": 2, "extended": 3}
_SELECTION = {"select": 1, "deselect": 2, "toggle": 3, "range": 4, "clear": 5, "all": 6, "current": 7, "click": 8}

class _InputEvent(C.Structure):
    _fields_ = [("size", C.c_uint32), ("phase", C.c_int32)] + [(name, C.c_char_p) for name in (
                "type", "target", "current", "related", "key", "detail", "drag_format", "drag_data", "model",
                "related_model")] + [("index", C.c_size_t), ("related_index", C.c_size_t)] + [(name, C.c_double)
                for name in ("x", "y", "window_x", "window_y", "wheel_x", "wheel_y")] + [(name, C.c_int32)
                for name in ("button", "clicks", "modifiers", "dropped")]

_EventFn = C.CFUNCTYPE(C.c_int32, C.c_void_p, C.POINTER(_InputEvent))

class Event:
    """Снимок события интерфейса; методы ставят флаги, которые уходят ядру после возврата обработчика.
    A UI event snapshot; the methods set flags that go to the core after the handler returns."""
    _TEXT = ("type", "target", "current", "related", "key", "detail", "drag_format", "drag_data", "model", "related_model")
    _NUMBERS = ("phase", "index", "related_index", "x", "y", "window_x", "window_y", "wheel_x", "wheel_y",
                "button", "clicks", "modifiers")

    def __init__(self, native):
        for field in self._TEXT:
            setattr(self, field, (getattr(native, field) or b"").decode("utf-8"))
        for field in self._NUMBERS:
            setattr(self, field, getattr(native, field))
        self.dropped, self.flags = bool(native.dropped), 0

    def prevent_default(self):
        self.flags |= 1

    def stop_propagation(self):
        self.flags |= 2

    def stop_immediate_propagation(self):
        self.flags |= 6
_Task = C.CFUNCTYPE(None, C.c_void_p)

class _Timer(C.Structure):
    _fields_ = [("seconds", C.c_double), ("repeat", C.c_int32), ("callback", _Task), ("user", C.c_void_p)]

class _UndoStep(C.Structure):
    _fields_ = [("size", C.c_uint32), ("undo", _Task), ("redo", _Task), ("release", _Task), ("user", C.c_void_p)]

_UndoFn = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p, C.c_uint32)
_WindowClosedFn = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p)

_WorkFn = C.CFUNCTYPE(None, C.c_void_p, C.c_void_p)
_TaskDoneFn = C.CFUNCTYPE(None, C.c_void_p, C.c_uint64, C.c_int32, C.c_char_p)
_TaskProgressFn = C.CFUNCTYPE(None, C.c_void_p, C.c_uint64, C.c_double, C.c_char_p)

class _Background(C.Structure):
    _fields_ = [("size", C.c_uint32), ("work", _WorkFn), ("done", _TaskDoneFn), ("progress", _TaskProgressFn),
                ("user", C.c_void_p)]

TASK_STATUS = ("done", "cancelled", "failed")

class BackgroundTask:
    """Контекст фоновой работы: действителен только внутри неё. / A background work context: valid only inside it."""
    def __init__(self, lib, handle):
        self._lib, self._handle = lib, handle

    def cancelled(self):
        return bool(self._lib.gran_task_cancelled(self._handle))

    def progress(self, fraction, text=""):
        self._lib.gran_task_progress(self._handle, float(fraction), _encode(text))

class _WindowOptions(C.Structure):
    _fields_ = [("size", C.c_uint32), ("owner", C.c_char_p), ("modal", C.c_int32),
                ("on_closed", _WindowClosedFn), ("user", C.c_void_p)]
_ConvertFn = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p, C.c_void_p)

class UndoState(tuple):
    """Состояние стека отмены: можно ли отменить и повторить, «чистое» ли оно (документ сохранён).
    An undo stack state: whether undo and redo are possible, whether it is clean (the document is saved)."""
    __slots__ = ()

    def __new__(cls, flags):
        return tuple.__new__(cls, (bool(flags & 1), bool(flags & 2), bool(flags & 4)))

    can_undo = property(lambda self: self[0])
    can_redo = property(lambda self: self[1])
    clean = property(lambda self: self[2])

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
        "app_on_event": (C.c_int32, [p, s, _EventFn, p]),
        "app_listen": (C.c_int32, [p, s, s, C.c_int32, _EventFn, p, C.POINTER(C.c_uint64)]),
        "app_unlisten": (C.c_int32, [p, s, C.c_uint64]),
        "app_dispatch": (C.c_int32, [p, s, s, s, C.POINTER(C.c_int32)]),
        "app_timer": (C.c_int32, [p, C.POINTER(_Timer), C.POINTER(C.c_uint64)]),
        "app_undo_push": (C.c_int32, [p, s, C.POINTER(_UndoStep)]),
        "app_undo_state": (C.c_int32, [p, s, C.POINTER(C.c_uint32)]),
        "app_on_undo": (C.c_int32, [p, _UndoFn, p]),
        "app_clear_timer": (C.c_int32, [p, C.c_uint64]),
        "app_read": (C.c_int32, [p, s, s, C.POINTER(p)]),
        "app_markup_read": (C.c_int32, [p, s, s, C.POINTER(p)]), "app_markup_write": (C.c_int32, [p, s, s, s]),
        "app_markup_save": (C.c_int32, [p, s, C.POINTER(p)]),
        "app_component_schema": (C.c_int32, [p, s, s, C.POINTER(p)]),
        "converted_set": (None, [p, s]),
        "app_variable_set_text": (C.c_int32, [p, s, s]), "app_variable_set_number": (C.c_int32, [p, s, C.c_double]),
        "app_variable_set_boolean": (C.c_int32, [p, s, C.c_int32]), "app_variable_read": (C.c_int32, [p, s, C.POINTER(p)]),
        "app_converter": (C.c_int32, [p, s, _ConvertFn, _ConvertFn, p]), "app_refresh_bindings": (C.c_int32, [p]),
        "app_apply": (C.c_int32, [p, C.POINTER(_Update), z]), "app_command": (C.c_int32, [p, s, s, s]),
        "model_count": (C.c_int32, [p, s, C.POINTER(z)]),
        "model_read": (C.c_int32, [p, s, z, s, C.POINTER(p)]),
        "model_insert": (C.c_int32, [p, s, C.POINTER(_Record)]),
        "model_set": (C.c_int32, [p, s, C.POINTER(_Record)]),
        "model_remove": (C.c_int32, [p, s, z]), "model_clear": (C.c_int32, [p, s]),
        "model_move": (C.c_int32, [p, s, z, z, z]), "model_columns": (C.c_int32, [p, s, C.POINTER(_Column), z]),
        "model_load": (C.c_int32, [p, s, C.POINTER(_Source)]), "model_on_fetch": (C.c_int32, [p, s, _FetchFn, p]),
        "model_set_more": (C.c_int32, [p, s, C.c_int32]), "model_fetch_more": (C.c_int32, [p, s]),
        "tree_insert": (C.c_int32, [p, s, C.c_uint64, C.POINTER(_Record), C.POINTER(C.c_uint64)]),
        "tree_remove": (C.c_int32, [p, s, C.c_uint64]), "tree_move": (C.c_int32, [p, s, C.c_uint64, C.c_uint64, z]),
        "tree_set": (C.c_int32, [p, s, C.c_uint64, C.POINTER(_Record)]),
        "tree_read": (C.c_int32, [p, s, C.c_uint64, s, C.POINTER(p)]),
        "tree_children": (C.c_int32, [p, s, C.c_uint64, C.POINTER(z)]),
        "tree_child": (C.c_int32, [p, s, C.c_uint64, z, C.POINTER(C.c_uint64)]),
        "tree_parent": (C.c_int32, [p, s, C.c_uint64, C.POINTER(C.c_uint64)]),
        "tree_set_lazy": (C.c_int32, [p, s, C.c_uint64, C.c_int32]), "tree_on_fetch": (C.c_int32, [p, s, _TreeFetchFn, p]),
        "tree_load": (C.c_int32, [p, s, C.POINTER(_Source)]),
        "view_create": (C.c_int32, [p, C.POINTER(_View)]), "view_sort": (C.c_int32, [p, s, s]),
        "view_filter": (C.c_int32, [p, s, s]), "view_source_index": (C.c_int32, [p, s, z, C.POINTER(z)]),
        "tree_row_read": (C.c_int32, [p, s, z, C.POINTER(_TreeRow)]), "tree_expand": (C.c_int32, [p, s, z, C.c_int32]),
        "selection_change": (C.c_int32, [p, s, C.c_int32, z, z]),
        "selection_read": (C.c_int32, [p, s, C.POINTER(z), C.POINTER(z), C.POINTER(z)]),
        "selection_on_change": (C.c_int32, [p, s, _SelectionFn, p]),
        "app_open_window": (C.c_int32, [p, s, C.c_int32, C.POINTER(_WindowOptions)]),
        "app_close_window": (C.c_int32, [p, s, s]),
        "app_run_task": (C.c_int32, [p, C.POINTER(_Background), C.POINTER(C.c_uint64)]),
        "app_cancel_task": (C.c_int32, [p, C.c_uint64, C.POINTER(C.c_int32)]),
        "task_cancelled": (C.c_int32, [p]), "task_progress": (C.c_int32, [p, C.c_double, s]),
        "task_fail": (C.c_int32, [p, s]),
        "resources_mount": (C.c_int32, [p, z]),
        "settings_load": (C.c_int32, [p, s, s]), "settings_get": (C.c_int32, [p, s, C.POINTER(C.c_int32), C.POINTER(p)]),
        "settings_set": (C.c_int32, [p, s, C.c_int32, s]), "settings_list_item": (C.c_int32, [p, s, z, C.POINTER(p)]),
        "settings_set_list": (C.c_int32, [p, s, C.POINTER(s), z]), "settings_remove": (C.c_int32, [p, s]),
        "settings_save": (C.c_int32, [p]),
        "log_open": (C.c_int32, [s, s, C.c_int32]), "log_write": (C.c_int32, [C.c_int32, s, s]),
        "log_path": (C.c_int32, [C.POINTER(p)]), "log_previous_crash": (C.c_int32, [C.POINTER(p)]),
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
        # Шаги отмены: user — номер пары обработчиков; release снимает её, когда стек отбросил шаг.
        # The undo steps: user is the number of a handler pair; release removes it when the stack dropped the step.
        self._undo_steps, self._undo_keys = {}, itertools.count(1)
        self._undo_entry = _Task(lambda user: self._run_undo(user, 0))
        self._redo_entry = _Task(lambda user: self._run_undo(user, 1))
        self._release_entry = _Task(lambda user: self._undo_steps.pop(user or 0, None))
        self._undo_handler = None
        self._converters = {}
        # Обработчики закрытия окон: user — номер обработчика, один переходник на все окна.
        # The window close handlers: user is the handler number, one trampoline for all the windows.
        self._windows = {}
        self._window_entry = _WindowClosedFn(self._window_closed)
        # Фоновые задачи: user — номер записи [work, done, progress, результат], переходники общие.
        # The background tasks: user is the number of an entry [work, done, progress, result], the trampolines are shared.
        self._background = {}
        self._work_entry = _WorkFn(self._run_work)
        self._done_entry = _TaskDoneFn(self._task_done)
        self._progress_entry = _TaskProgressFn(self._task_progress)
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

    def markup_attribute(self, target, name):
        """Атрибут узла в живой разметке; None — нет атрибута. / The node attribute in the live markup; None — no attribute."""
        output = C.c_void_p()
        self._call("app_markup_read", _encode(target), _encode(name), C.byref(output))
        if not output.value:
            return None
        try:
            return C.string_at(output).decode("utf-8")
        finally:
            self._lib.gran_string_free(output)

    def set_markup_attribute(self, target, name, value):
        """Записать атрибут в разметку (None — удалить) и перестроить узел; ошибка схемы — RuntimeError.
        Write the attribute into the markup (None removes it) and rebuild the node; a schema error is RuntimeError."""
        self._call("app_markup_write", _encode(target), _encode(name), _encode(value))

    def save_markup(self, window=None):
        """Живая разметка окна (None — главного) как XML. / The live markup of the window (None — the main one) as XML."""
        return self._string("app_markup_save", _encode(window))

    def component_schema(self, namespace, window=None):
        """Сгенерированная XSD библиотеки компонентов; "" — такой нет. / The generated XSD of a component library; "" — no such one."""
        return self._string("app_component_schema", _encode(namespace), _encode(window))

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

    def _event_entry(self, callback):
        # Флаги события возвращаются ядру; исключение — как у действий (закрытие и повтор в run).
        # The event flags return to the core; an exception — as for actions (closing and re-raising in run).
        def invoke(user, pointer):
            event = Event(pointer.contents)
            self._protect(callback, event)
            return event.flags
        wrapped = _EventFn(invoke)
        self._callbacks.append(wrapped)
        return wrapped

    def on_event(self, name, callback):
        """Обработчик имени из разметки (onPointerDown="name"). / A markup name handler (onPointerDown="name")."""
        self._call("app_on_event", _encode(name), self._event_entry(callback), None)

    def listen(self, target, type, callback, capture=False):
        """Слушатель события у узла; возвращает номер для unlisten. / A node event listener; returns the number for unlisten."""
        result = C.c_uint64()
        self._call("app_listen", _encode(target), _encode(type), 1 if capture else 0, self._event_entry(callback),
                   None, C.byref(result))
        return result.value

    def unlisten(self, target, listener):
        self._call("app_unlisten", _encode(target), listener)

    def dispatch(self, target, type, detail=None):
        """Своё событие; False — обработчик отменил действие. / A custom event; False — a handler cancelled the action."""
        allowed = C.c_int32()
        self._call("app_dispatch", _encode(target), _encode(type), _encode(detail), C.byref(allowed))
        return bool(allowed.value)

    def capture_pointer(self, target):
        self.command("capturePointer", target)

    def release_pointer(self, target):
        self.command("releasePointer", target)

    def run_command(self, command_id):
        """Выполнить <Command id> (выключенная — ничего). / Run a <Command id> (a disabled one does nothing)."""
        self.command("runCommand", command_id)

    def set_command_enabled(self, command_id, enabled):
        """Включить/выключить команду и её кнопки и пункты меню. / Enable/disable a command and its buttons and menu items."""
        self.command("enableCommand" if enabled else "disableCommand", command_id)

    def _run_undo(self, user, side):
        entry = self._undo_steps.get(user or 0)
        if entry is not None:
            self._protect(entry[side])

    def push_undo(self, undo, redo, stack=None):
        """Записать выполненное действие парой обработчиков (стек None — основной).
        Record a done action as a pair of handlers (the stack None is the main one)."""
        if not callable(undo) or not callable(redo):
            raise TypeError("undo and redo must be callable")
        key = next(self._undo_keys)
        self._undo_steps[key] = (undo, redo)
        step = _UndoStep(C.sizeof(_UndoStep), self._undo_entry, self._redo_entry, self._release_entry, C.c_void_p(key))
        try:
            self._call("app_undo_push", _encode(stack), C.byref(step))
        except BaseException:
            self._undo_steps.pop(key, None)
            raise

    def undo(self, stack=None):
        self.command("undo", stack)

    def redo(self, stack=None):
        self.command("redo", stack)

    def undo_state(self, stack=None):
        """UndoState(can_undo, can_redo, clean) стека. / The stack UndoState(can_undo, can_redo, clean)."""
        flags = C.c_uint32()
        self._call("app_undo_state", _encode(stack), C.byref(flags))
        return UndoState(flags.value)

    def set_undo_clean(self, stack=None):
        """Текущая позиция — «документ сохранён». / The current position is "the document is saved"."""
        self.command("setUndoClean", stack)

    def clear_undo(self, stack=None):
        self.command("clearUndo", stack)

    def set_undo_limit(self, limit, stack=None):
        """Глубина стека (0 — без предела). / The stack depth (0 — no limit)."""
        self.command("setUndoLimit", stack, str(int(limit)))

    def on_undo_changed(self, callback):
        """callback(stack, UndoState) после каждой смены любого стека; None снимает.
        callback(stack, UndoState) after every change of any stack; None removes it."""
        if callback is None:
            self._call("app_on_undo", _UndoFn(), None)
            self._undo_handler = None
            return
        def invoke(user, stack, flags):
            self._protect(callback, (stack or b"").decode("utf-8"), UndoState(flags))
        wrapped = _UndoFn(invoke)
        self._call("app_on_undo", wrapped, None)
        self._undo_handler = wrapped

    def set_variable(self, name, value):
        """Переменная $name выражений {…}: bool — логическая, int/float — число, иначе текст с прежним типом.
        The $name variable of the {…} expressions: bool is boolean, int/float is a number, otherwise a text keeping the type."""
        if isinstance(value, bool):
            self._call("app_variable_set_boolean", _encode(name), int(value))
        elif isinstance(value, (int, float)):
            self._call("app_variable_set_number", _encode(name), float(value))
        else:
            self._call("app_variable_set_text", _encode(name), _encode(value))

    def variable(self, name):
        """Значение переменной текстом; None — переменной нет. / The variable value as text; None — no such variable."""
        output = C.c_void_p()
        self._call("app_variable_read", _encode(name), C.byref(output))
        if not output.value:
            return None
        try:
            return C.string_at(output).decode("utf-8")
        finally:
            self._lib.gran_string_free(output)

    def set_converter(self, name, to_view=None, to_source=None):
        """Преобразователь «{выражение ; name}»: to_view(str) -> str, to_source(str) -> str (None — односторонний).
        The "{expression ; name}" converter: to_view(str) -> str, to_source(str) -> str (None — one-way)."""
        def adapt(function):
            if function is None:
                return _ConvertFn()
            def invoke(user, value, result):
                def answer():
                    self._lib.gran_converted_set(result, _encode(function((value or b"").decode("utf-8"))))
                self._protect(answer)
            return _ConvertFn(invoke)
        pair = (adapt(to_view), adapt(to_source))
        self._call("app_converter", _encode(name), pair[0], pair[1], None)
        self._converters[name] = pair

    def refresh_bindings(self):
        """Пересчитать выражения сейчас, не дожидаясь кадра. / Recompute the expressions now without waiting for a frame."""
        self._call("app_refresh_bindings")

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

    def _window_closed(self, user, result):
        handler = self._windows.pop(user or 0, None)
        if handler is not None:
            self._protect(handler, (result or b"").decode("utf-8"))

    def open_window(self, source, is_file=True, owner=None, modal=False, on_closed=None):
        """Ещё одно окно; owner — id узла окна-владельца, on_closed(result) — после закрытия.
        One more window; owner is the id of an owner window node, on_closed(result) — after closing."""
        key = 0
        if on_closed is not None:
            with self._task_lock:
                key = next(self._task_keys)
            self._windows[key] = on_closed
        options = _WindowOptions(C.sizeof(_WindowOptions), _encode(owner), 1 if modal else 0,
                                 self._window_entry if on_closed is not None else _WindowClosedFn(), C.c_void_p(key))
        try:
            self._call("app_open_window", _encode(source), 1 if is_file else 0, C.byref(options))
        except BaseException:
            self._windows.pop(key, None)
            raise

    def _run_work(self, user, handle):
        # Поток пула: исключение не пересекает ctypes — оно становится сбоем задачи.
        # A pool thread: an exception does not cross ctypes — it becomes the task failure.
        entry = self._background.get(user or 0)
        if entry is None:
            return
        try:
            entry[3] = entry[0](BackgroundTask(self._lib, handle))
        except BaseException as error:
            self._lib.gran_task_fail(handle, _encode(str(error) or type(error).__name__))

    def _task_done(self, user, task, status, error):
        entry = self._background.pop(user or 0, None)
        if entry is not None and entry[1] is not None:
            result = entry[3] if status == 0 else None
            self._protect(entry[1], TASK_STATUS[status], result, (error or b"").decode("utf-8"))

    def _task_progress(self, user, task, fraction, text):
        entry = self._background.get(user or 0)
        if entry is not None and entry[2] is not None:
            self._protect(entry[2], fraction, (text or b"").decode("utf-8"))

    def run_task(self, work, done=None, progress=None):
        """work(task) -> результат в потоке пула; done(status, result, error) и progress(fraction, text) — в потоке UI.
        work(task) -> a result on a pool thread; done(status, result, error) and progress(fraction, text) — on the UI thread."""
        with self._task_lock:
            key = next(self._task_keys)
        self._background[key] = [work, done, progress, None]
        task = _Background(C.sizeof(_Background), self._work_entry, self._done_entry, self._progress_entry, C.c_void_p(key))
        result = C.c_uint64()
        try:
            self._call("app_run_task", C.byref(task), C.byref(result))
        except BaseException:
            self._background.pop(key, None)
            raise
        return result.value

    def cancel_task(self, task):
        """Попросить отменить; False — задачи уже нет. / Ask to cancel; False — the task is gone."""
        found = C.c_int32()
        self._call("app_cancel_task", C.c_uint64(task), C.byref(found))
        return bool(found.value)

    def mount_resources(self, blob):
        """Подключить блок gran_rcc --blob (bytes или путь к файлу): пути res:/ для всего процесса.
        Mount a gran_rcc --blob block (bytes or a file path): the res:/ paths for the whole process."""
        if not isinstance(blob, (bytes, bytearray)):
            with open(blob, "rb") as stream:
                blob = stream.read()
        data = C.create_string_buffer(bytes(blob), len(blob))
        self._check(self._lib.gran_resources_mount(data, len(blob)))

    def load_settings(self, app_name=None, path=None):
        """Файл настроек: path или по имени программы. / The settings file: path or by the program name."""
        self._call("settings_load", _encode(app_name), _encode(path))

    def setting(self, name, default=None):
        """Значение настройки (str, float, bool или list) или default. / A setting value (str, float, bool or list) or default."""
        kind, output = C.c_int32(), C.c_void_p()
        self._call("settings_get", _encode(name), C.byref(kind), C.byref(output))
        if kind.value < 0:
            return default
        if kind.value == 3:
            return self._setting_list(name)
        try:
            text = C.string_at(output).decode("utf-8")
        finally:
            self._lib.gran_string_free(output)
        if kind.value == 1:
            return float(text)
        if kind.value == 2:
            return text == "true"
        return text

    def _setting_list(self, name):
        items, index = [], 0
        while True:
            output = C.c_void_p()
            self._call("settings_list_item", _encode(name), index, C.byref(output))
            if not output.value:
                return items
            try:
                items.append(C.string_at(output).decode("utf-8"))
            finally:
                self._lib.gran_string_free(output)
            index += 1

    def set_setting(self, name, value):
        """Тип — по значению: bool, число, список строк или строка. / The type — by the value: bool, a number, a list of strings or a string."""
        if isinstance(value, (list, tuple)):
            data = (C.c_char_p * max(1, len(value)))(*[_encode(item) for item in value])
            self._call("settings_set_list", _encode(name), data, len(value))
        elif isinstance(value, bool):
            self._call("settings_set", _encode(name), 2, b"true" if value else b"false")
        elif isinstance(value, (int, float)):
            self._call("settings_set", _encode(name), 1, repr(float(value)).encode("ascii"))
        else:
            self._call("settings_set", _encode(name), 0, _encode(value))

    def remove_setting(self, name):
        self._call("settings_remove", _encode(name))

    def save_settings(self):
        self._call("settings_save")

    LOG_LEVELS = ("severe", "warning", "info", "verbose")

    def open_log(self, app_name, directory=None, crash_reports=True):
        """Журнал процесса в файл и перехват падения. / The process log file and the crash catching."""
        self._check(self._lib.gran_log_open(_encode(app_name), _encode(directory), 1 if crash_reports else 0))

    def log(self, level, category, text):
        """level — "severe" / "warning" / "info" / "verbose". / level is "severe" / "warning" / "info" / "verbose"."""
        self._check(self._lib.gran_log_write(self.LOG_LEVELS.index(level), _encode(category), _encode(text)))

    def _process_string(self, name):
        output = C.c_void_p()
        self._check(getattr(self._lib, "gran_" + name)(C.byref(output)))
        try:
            return C.string_at(output).decode("utf-8")
        finally:
            self._lib.gran_string_free(output)

    def log_path(self):
        return self._process_string("log_path")

    def previous_crash(self):
        """Отчёт о сбое прошлого запуска ("" — нет). / The previous run crash report ("" — none)."""
        return self._process_string("log_previous_crash")

    def close_window(self, target, result=None):
        """Закрыть окно узла target с результатом для on_closed. / Close the window of the target node with a result for on_closed."""
        self._call("app_close_window", _encode(target), _encode(result))

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

    # ---- Модели 1.7 / Models 1.7 ----

    def model_move(self, model, start, count, to):
        """Переставить записи [start, start + count) так, чтобы они начинались с to. / Move the records so that they start at to."""
        self._call("model_move", _encode(model), start, count, to)

    def model_columns(self, model, columns):
        """Столбцы таблицы: [(имя, заголовок, "text"|"number"|"date"|"boolean")]. / The table columns."""
        rows = [_Column(_encode(name), _encode(title), _COLUMNS[kind]) for name, title, kind in columns]
        self._call("model_columns", _encode(model), (_Column * len(rows))(*rows), len(rows))

    def _source(self, path, select, children, fetch, fields, format):
        pairs = [(_encode(name), _encode(value)) for name, value in (fields or {}).items()]
        data = (_Field * len(pairs))(*(_Field(*row) for row in pairs))
        source = _Source(C.sizeof(_Source), _FORMATS[format], _encode(path), _encode(select), _encode(children),
                         fetch, data, len(pairs))
        return source, data

    def load_data(self, model, path, select="", fetch=0, fields=None, format="auto"):
        """Список из файла JSON/XML; fields — {поле: путь}; fetch > 0 — лениво. / A list from a JSON/XML file; fetch > 0 — lazily."""
        source, keep = self._source(path, select, "", fetch, fields, format)
        self._call("model_load", _encode(model), C.byref(source))

    def load_tree(self, tree, path, select="", children="", fields=None, format="auto"):
        """Дерево из файла: children — имя элемента XML или члена JSON с детьми. / A tree from a file."""
        source, keep = self._source(path, select, children, 0, fields, format)
        self._call("tree_load", _encode(tree), C.byref(source))

    def on_fetch(self, model, callback):
        """callback(model) вставляет следующую порцию; конец — set_more(model, False). None снимает. / callback inserts the next portion."""
        if callback is None:
            self._call("model_on_fetch", _encode(model), C.cast(None, _FetchFn), None)
            return
        native = _FetchFn(lambda user, name: self._protect(callback, name.decode("utf-8")))
        self._callbacks.append(native)
        self._call("model_on_fetch", _encode(model), native, None)

    def set_more(self, model, more):
        self._call("model_set_more", _encode(model), int(bool(more)))

    def fetch_more(self, model):
        self._call("model_fetch_more", _encode(model))

    def tree_insert(self, tree, parent, fields, index=SIZE_MAX):
        """Новый узел ребёнком parent (0 — верхний уровень); результат — номер узла. / A new node as a child of parent; the result is the node number."""
        values = [(_encode(key), _encode(value)) for key, value in fields.items()]
        data = (_Field * len(values))(*(_Field(*row) for row in values))
        record, result = _Record(index, data, len(data)), C.c_uint64()
        self._call("tree_insert", _encode(tree), parent, C.byref(record), C.byref(result))
        return result.value

    def tree_remove(self, tree, node):
        self._call("tree_remove", _encode(tree), node)

    def tree_move(self, tree, node, parent, row):
        self._call("tree_move", _encode(tree), node, parent, row)

    def tree_set(self, tree, node, fields):
        values = [(_encode(key), _encode(value)) for key, value in fields.items()]
        data = (_Field * len(values))(*(_Field(*row) for row in values))
        self._call("tree_set", _encode(tree), node, C.byref(_Record(0, data, len(data))))

    def tree_read(self, tree, node, field):
        return self._string("tree_read", _encode(tree), node, _encode(field))

    def tree_children(self, tree, node=0):
        """Номера детей узла (0 — верхний уровень). / The child numbers of the node (0 — the top level)."""
        count, child = C.c_size_t(), C.c_uint64()
        self._call("tree_children", _encode(tree), node, C.byref(count))
        result = []
        for row in range(count.value):
            self._call("tree_child", _encode(tree), node, row, C.byref(child))
            result.append(child.value)
        return result

    def tree_parent(self, tree, node):
        result = C.c_uint64()
        self._call("tree_parent", _encode(tree), node, C.byref(result))
        return result.value

    def tree_set_lazy(self, tree, node, lazy=True):
        self._call("tree_set_lazy", _encode(tree), node, int(bool(lazy)))

    def on_tree_fetch(self, tree, callback):
        """callback(tree, node) вставляет детей ленивого узла при первом раскрытии. / callback inserts the lazy node children."""
        native = _TreeFetchFn(lambda user, name, node: self._protect(callback, name.decode("utf-8"), node))
        self._callbacks.append(native)
        self._call("tree_on_fetch", _encode(tree), native, None)

    def _view(self, kind, name, source, sort=None, filter=None, mode="extended", expanded=0):
        view = _View(C.sizeof(_View), kind, _encode(name), _encode(source), _encode(sort), _encode(filter),
                     _MODES[mode], expanded)
        self._call("view_create", C.byref(view))

    def create_proxy(self, name, source, sort=None, filter=None):
        """Прокси сортировки и фильтра над списком source. / A sort and filter proxy over the source list."""
        self._view(1, name, source, sort, filter)

    def create_tree_list(self, name, tree, sort=None, filter=None, expanded=0):
        """Плоское представление дерева для списков. / A flat tree view for lists."""
        self._view(2, name, tree, sort, filter, expanded=expanded)

    def create_selection(self, name, source, mode="extended"):
        """Модель выделения: "none" | "single" | "multi" | "extended". / A selection model."""
        self._view(3, name, source, mode=mode)

    def set_sort(self, view, keys):
        self._call("view_sort", _encode(view), _encode(keys))

    def set_filter(self, view, expression):
        self._call("view_filter", _encode(view), _encode(expression))

    def source_index(self, view, row):
        result = C.c_size_t()
        self._call("view_source_index", _encode(view), row, C.byref(result))
        return result.value

    def tree_row(self, view, row):
        """Узел и признаки строки плоского дерева. / The node and the flags of a flat tree row."""
        result = _TreeRow()
        self._call("tree_row_read", _encode(view), row, C.byref(result))
        return {"node": result.node, "depth": result.depth, "expanded": bool(result.expanded),
                "has_children": bool(result.has_children)}

    def tree_expand(self, view, row, action="toggle"):
        self._call("tree_expand", _encode(view), row, {"expand": 1, "collapse": 0, "toggle": -1}[action])

    def selection_change(self, selection, command, index=0, last=0):
        """command: select, deselect, toggle, range, clear, all, current, click (last — 1 Shift, 2 Ctrl). / The selection command."""
        self._call("selection_change", _encode(selection), _SELECTION[command], index, last)

    def selection(self, selection):
        """(текущая или None, [отмеченные]). / (the current one or None, [the marked ones])."""
        current, count = C.c_size_t(), C.c_size_t(0)
        self._call("selection_read", _encode(selection), C.byref(current), None, C.byref(count))
        marked = (C.c_size_t * max(count.value, 1))()
        self._call("selection_read", _encode(selection), C.byref(current), marked, C.byref(count))
        return (None if current.value == SIZE_MAX else current.value), list(marked[:count.value])

    def on_selection_changed(self, selection, callback):
        """callback(имя) при любом изменении выделения; None снимает. / callback(name) on any selection change."""
        if callback is None:
            self._call("selection_on_change", _encode(selection), C.cast(None, _SelectionFn), None)
            return
        native = _SelectionFn(lambda user, name: self._protect(callback, name.decode("utf-8")))
        self._callbacks.append(native)
        self._call("selection_on_change", _encode(selection), native, None)

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
            self._undo_steps.clear()
            self._undo_handler = None
            with self._task_lock:
                self._tasks.clear()
                self._timer_keys.clear()
                self._key_timers.clear()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
