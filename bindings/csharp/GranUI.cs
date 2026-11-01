using System.Collections.Concurrent;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;

namespace GranUI;

// Управляемый снимок не ссылается на память дерева или callback.
// A managed snapshot references neither tree nor callback memory.
public sealed record Event(string Target, string Model, nuint Index, string Text,
    string Option, string Message, double Value, bool Checked);

// Снимок события интерфейса; методы ставят флаги, которые уходят ядру после возврата обработчика.
// A UI event snapshot; the methods set flags that go to the core after the handler returns.
public sealed class UiEvent
{
    public string Type = "", Target = "", Current = "", Related = "", Key = "", Detail = "";
    public string DragFormat = "", DragData = "", Model = "", RelatedModel = "";
    public int Phase, Button, Clicks, Modifiers;
    public nuint Index, RelatedIndex;
    public double X, Y, WindowX, WindowY, WheelX, WheelY;
    public bool Dropped;
    internal int Flags;
    public void PreventDefault() => Flags |= 1;
    public void StopPropagation() => Flags |= 2;
    public void StopImmediatePropagation() => Flags |= 6;
}

// Состояние стека отмены: можно ли отменить и повторить, «чистое» ли оно (документ сохранён).
// An undo stack state: whether undo and redo are possible, whether it is clean (the document is saved).
public readonly record struct UndoState(bool CanUndo, bool CanRedo, bool Clean)
{
    internal static UndoState From(uint flags) => new((flags & 1) != 0, (flags & 2) != 0, (flags & 4) != 0);
}

internal static class Native
{
    const string Library = "gran_c";
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void ActionFn(IntPtr user, IntPtr value);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void TaskFn(IntPtr user);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate int EventFn(IntPtr user, IntPtr value);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void UndoFn(IntPtr user, IntPtr stack, uint flags);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void ConvertFn(IntPtr user, IntPtr value, IntPtr result);
    [StructLayout(LayoutKind.Sequential)] internal struct UndoStep {
        internal uint Size; internal TaskFn Undo, Redo, Release; internal IntPtr User;
    }
    [StructLayout(LayoutKind.Sequential)] internal struct InputEvent {
        internal uint Size; internal int Phase;
        internal IntPtr Type, Target, Current, Related, Key, Detail, DragFormat, DragData, Model, RelatedModel;
        internal nuint Index, RelatedIndex;
        internal double X, Y, WindowX, WindowY, WheelX, WheelY;
        internal int Button, Clicks, Modifiers, Dropped;
    }
    [StructLayout(LayoutKind.Sequential)] internal struct Options {
        internal uint Size; internal int Backend, Debug, Warp, Watch, NoVsync; internal double ExitAfter;
    }
    [StructLayout(LayoutKind.Sequential)] internal struct Update { internal IntPtr Target, Property, Value; }
    [StructLayout(LayoutKind.Sequential)] internal struct Field { internal IntPtr Name, Value; }
    [StructLayout(LayoutKind.Sequential)] internal struct Record { internal nuint Index; internal IntPtr Fields; internal nuint Count; }
    [StructLayout(LayoutKind.Sequential)] internal struct Timer { internal double Seconds; internal int Repeat; internal TaskFn Callback; internal IntPtr User; }
    [StructLayout(LayoutKind.Sequential)] internal struct Event {
        internal IntPtr Target, Model; internal nuint Index; internal IntPtr Text, Option, Message;
        internal double Value; internal int Checked;
    }
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern uint gran_abi_version();
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern IntPtr gran_last_error();
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern void gran_string_free(IntPtr value);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_create(ref Options options, out IntPtr result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_destroy(IntPtr app);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_load(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string xml, int isFile);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_run(IntPtr app, out int exitCode);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_on_action(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, ActionFn callback, IntPtr user);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_on_message(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, ActionFn callback, IntPtr user);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_post(IntPtr app, TaskFn callback, IntPtr user);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_on_event(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, EventFn callback, IntPtr user);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_listen(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string type, int capture, EventFn callback, IntPtr user, out ulong result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_unlisten(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, ulong listener);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_dispatch(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string type, [MarshalAs(UnmanagedType.LPUTF8Str)] string? detail, out int allowed);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_timer(IntPtr app, ref Timer timer, out ulong result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_clear_timer(IntPtr app, ulong timer);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_undo_push(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string? stack, ref UndoStep step);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_undo_state(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string? stack, out uint flags);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_on_undo(IntPtr app, UndoFn? callback, IntPtr user);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern void gran_converted_set(IntPtr result, [MarshalAs(UnmanagedType.LPUTF8Str)] string text);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_variable_set_text(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_variable_set_number(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, double value);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_variable_set_boolean(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, int value);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_variable_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, out IntPtr result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_converter(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, ConvertFn? toView, ConvertFn? toSource, IntPtr user);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_refresh_bindings(IntPtr app);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_markup_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, out IntPtr result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_markup_write(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, [MarshalAs(UnmanagedType.LPUTF8Str)] string? value);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_markup_save(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string? window, out IntPtr result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string property, out IntPtr result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_apply(IntPtr app, IntPtr updates, nuint count);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_app_command(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, [MarshalAs(UnmanagedType.LPUTF8Str)] string? target, [MarshalAs(UnmanagedType.LPUTF8Str)] string? argument);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_model_count(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, out nuint result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_model_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, nuint index, [MarshalAs(UnmanagedType.LPUTF8Str)] string field, out IntPtr result);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_model_insert(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, ref Record record);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_model_set(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, ref Record record);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_model_remove(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, nuint index);
    [DllImport(Library, CallingConvention=CallingConvention.Cdecl)] internal static extern int gran_model_clear(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model);
}

// Все временные строки и массивы живут до завершения одного вызова C ABI.
// All temporary strings and arrays live until a single C ABI call completes.
internal sealed class Arguments : IDisposable
{
    readonly List<IntPtr> memory = new();
    internal static string Text(string text) {
        ArgumentNullException.ThrowIfNull(text);
        if (text.Contains('\0')) throw new ArgumentException("embedded NUL", nameof(text));
        return text;
    }
    internal IntPtr Utf8(string text) {
        var value = Marshal.StringToCoTaskMemUTF8(Text(text));
        memory.Add(value); return value;
    }
    internal IntPtr Array<T>(T[] values) where T : struct {
        int size = Marshal.SizeOf<T>();
        var result = Marshal.AllocCoTaskMem(checked(size * values.Length));
        memory.Add(result);
        for (int index = 0; index < values.Length; ++index)
            Marshal.StructureToPtr(values[index], IntPtr.Add(result, size * index), false);
        return result;
    }
    public void Dispose() { foreach (var value in memory) Marshal.FreeCoTaskMem(value); }
}

// Владелец приложения; Dispose вызывается в потоке UI после Run.
// Application owner; Dispose runs on the UI thread after Run.
public sealed class Application : IDisposable
{
    IntPtr app;
    readonly ConcurrentBag<Delegate> callbacks = new();
    // Один переходник на все задачи и таймеры: user — номер задачи (иначе каждый Post держал бы свой делегат до Dispose).
    // One trampoline for all tasks and timers: user is the task number (otherwise every Post would keep its own delegate until Dispose).
    readonly ConcurrentDictionary<long, (Action Callback, bool Repeat)> tasks = new();
    readonly ConcurrentDictionary<ulong, long> timerKeys = new();
    readonly ConcurrentDictionary<long, ulong> keyTimers = new();
    readonly Native.TaskFn taskEntry;
    long nextTask;
    // Шаги отмены: user — номер пары обработчиков; release снимает её, когда стек отбросил шаг.
    // The undo steps: user is the number of a handler pair; release removes it when the stack dropped the step.
    readonly ConcurrentDictionary<long, (Action Undo, Action Redo)> undoSteps = new();
    readonly Native.TaskFn undoEntry, redoEntry, releaseEntry;
    Native.UndoFn? undoHandler;
    ExceptionDispatchInfo? pending;
    IntPtr Handle => app != IntPtr.Zero ? app : throw new ObjectDisposedException(nameof(Application));
    static string String(IntPtr value) => Marshal.PtrToStringUTF8(value) ?? "";
    static void Check(int status) { if (status != 0) throw new InvalidOperationException(String(Native.gran_last_error())); }
    static string OwnedString(IntPtr value) {
        try { return String(value); }
        finally { Native.gran_string_free(value); }
    }
    public Application(int backend=0, double exitAfter=0, bool warp=false) {
        if (Native.gran_abi_version() != 1) throw new NotSupportedException("unsupported Gran UI ABI");
        var options = new Native.Options { Size=(uint)Marshal.SizeOf<Native.Options>(), Backend=backend, ExitAfter=exitAfter, Warp=warp ? 1 : 0 };
        taskEntry = RunTask;
        undoEntry = user => RunUndo(user, false);
        redoEntry = user => RunUndo(user, true);
        releaseEntry = user => undoSteps.TryRemove((long)user, out _);
        Check(Native.gran_app_create(ref options, out app));
    }
    public void Dispose() {
        if (app == IntPtr.Zero) return;
        Check(Native.gran_app_destroy(app));
        app = IntPtr.Zero; callbacks.Clear(); tasks.Clear(); timerKeys.Clear(); keyTimers.Clear(); undoSteps.Clear(); undoHandler = null;
    }
    public void LoadString(string xml) => Check(Native.gran_app_load(Handle, Arguments.Text(xml), 0));
    public void LoadFile(string path) => Check(Native.gran_app_load(Handle, Arguments.Text(path), 1));
    public string Read(string target, string property) {
        Check(Native.gran_app_read(Handle, Arguments.Text(target), Arguments.Text(property), out var value));
        return OwnedString(value);
    }
    // Атрибут узла в живой разметке; null — атрибута нет / The node attribute in the live markup; null — no attribute
    public string? MarkupAttribute(string target, string name) {
        Check(Native.gran_app_markup_read(Handle, Arguments.Text(target), Arguments.Text(name), out var value));
        return value == IntPtr.Zero ? null : OwnedString(value);
    }
    // Записать атрибут в разметку (null — удалить) и перестроить узел / Write the attribute into the markup (null removes it) and rebuild the node
    public void SetMarkupAttribute(string target, string name, string? value) {
        if (value != null) Arguments.Text(value);
        Check(Native.gran_app_markup_write(Handle, Arguments.Text(target), Arguments.Text(name), value));
    }
    // Живая разметка окна (null — главного) как XML / The live markup of the window (null — the main one) as XML
    public string SaveMarkup(string? window=null) {
        if (window != null) Arguments.Text(window);
        Check(Native.gran_app_markup_save(Handle, window, out var value));
        return OwnedString(value);
    }
    public void Apply(params (string Target,string Property,string Value)[] updates) {
        using var args = new Arguments();
        var rows = updates.Select(row => new Native.Update { Target=args.Utf8(row.Target), Property=args.Utf8(row.Property), Value=args.Utf8(row.Value) }).ToArray();
        Check(Native.gran_app_apply(Handle, args.Array(rows), (nuint)rows.Length));
    }
    public void Command(string name, string? target=null, string? argument=null) {
        if (target != null) Arguments.Text(target);
        if (argument != null) Arguments.Text(argument);
        Check(Native.gran_app_command(Handle, Arguments.Text(name), target, argument));
    }
    void Protect(Action callback) {
        try { callback(); }
        catch (Exception error) {
            Interlocked.CompareExchange(ref pending, ExceptionDispatchInfo.Capture(error), null);
            Native.gran_app_command(app, "close", null, null);
        }
    }
    void On(string name, Action<Event> callback, bool message) {
        ArgumentNullException.ThrowIfNull(callback);
        Native.ActionFn native = (_, pointer) => Protect(() => {
            var e = Marshal.PtrToStructure<Native.Event>(pointer);
            callback(new Event(String(e.Target), String(e.Model), e.Index, String(e.Text), String(e.Option), String(e.Message), e.Value, e.Checked != 0));
        });
        callbacks.Add(native);
        if (message) Check(Native.gran_app_on_message(Handle, Arguments.Text(name), native, IntPtr.Zero));
        else Check(Native.gran_app_on_action(Handle, Arguments.Text(name), native, IntPtr.Zero));
    }
    public void OnAction(string name, Action<Event> callback) => On(name, callback, false);
    public void OnMessage(string name, Action<Event> callback) => On(name, callback, true);
    // Переходник события: снимок, вызов под Protect, флаги — ядру / The event trampoline: a snapshot, a call under Protect, the flags go to the core
    Native.EventFn EventEntry(Action<UiEvent> callback) {
        ArgumentNullException.ThrowIfNull(callback);
        Native.EventFn native = (_, pointer) => {
            var n = Marshal.PtrToStructure<Native.InputEvent>(pointer);
            var e = new UiEvent { Type=String(n.Type), Target=String(n.Target), Current=String(n.Current), Related=String(n.Related),
                Key=String(n.Key), Detail=String(n.Detail), DragFormat=String(n.DragFormat), DragData=String(n.DragData),
                Model=String(n.Model), RelatedModel=String(n.RelatedModel), Phase=n.Phase, Button=n.Button, Clicks=n.Clicks,
                Modifiers=n.Modifiers, Index=n.Index, RelatedIndex=n.RelatedIndex, X=n.X, Y=n.Y, WindowX=n.WindowX,
                WindowY=n.WindowY, WheelX=n.WheelX, WheelY=n.WheelY, Dropped=n.Dropped != 0 };
            Protect(() => callback(e));
            return e.Flags;
        };
        callbacks.Add(native);
        return native;
    }
    public void OnEvent(string name, Action<UiEvent> callback) =>
        Check(Native.gran_app_on_event(Handle, Arguments.Text(name), EventEntry(callback), IntPtr.Zero));
    public ulong Listen(string target, string type, Action<UiEvent> callback, bool capture=false) {
        Check(Native.gran_app_listen(Handle, Arguments.Text(target), Arguments.Text(type), capture ? 1 : 0, EventEntry(callback), IntPtr.Zero, out var result));
        return result;
    }
    public void Unlisten(string target, ulong listener) => Check(Native.gran_app_unlisten(Handle, Arguments.Text(target), listener));
    public bool Dispatch(string target, string type, string? detail=null) {
        if (detail != null) Arguments.Text(detail);
        Check(Native.gran_app_dispatch(Handle, Arguments.Text(target), Arguments.Text(type), detail, out var allowed));
        return allowed != 0;
    }
    public void CapturePointer(string target) => Command("capturePointer", target);
    public void ReleasePointer(string target) => Command("releasePointer", target);
    public void RunCommand(string id) => Command("runCommand", id);
    public void SetCommandEnabled(string id, bool enabled) => Command(enabled ? "enableCommand" : "disableCommand", id);
    void RunUndo(IntPtr user, bool redo) {
        if (undoSteps.TryGetValue((long)user, out var step)) Protect(redo ? step.Redo : step.Undo);
    }
    // Записать выполненное действие парой обработчиков (стек null — основной) / Record a done action as a pair of handlers (the stack null is the main one)
    public void PushUndo(Action undo, Action redo, string? stack=null) {
        ArgumentNullException.ThrowIfNull(undo); ArgumentNullException.ThrowIfNull(redo);
        if (stack != null) Arguments.Text(stack);
        long key = Interlocked.Increment(ref nextTask);
        undoSteps[key] = (undo, redo);
        var step = new Native.UndoStep { Size=(uint)Marshal.SizeOf<Native.UndoStep>(), Undo=undoEntry, Redo=redoEntry, Release=releaseEntry, User=(IntPtr)key };
        try { Check(Native.gran_app_undo_push(Handle, stack, ref step)); }
        catch { undoSteps.TryRemove(key, out _); throw; }
    }
    public void Undo(string? stack=null) => Command("undo", stack);
    public void Redo(string? stack=null) => Command("redo", stack);
    public UndoState GetUndoState(string? stack=null) {
        if (stack != null) Arguments.Text(stack);
        Check(Native.gran_app_undo_state(Handle, stack, out var flags));
        return UndoState.From(flags);
    }
    // Текущая позиция — «документ сохранён» / The current position is "the document is saved"
    public void SetUndoClean(string? stack=null) => Command("setUndoClean", stack);
    public void ClearUndo(string? stack=null) => Command("clearUndo", stack);
    public void SetUndoLimit(int limit, string? stack=null) =>
        Command("setUndoLimit", stack, limit >= 0 ? limit.ToString(System.Globalization.CultureInfo.InvariantCulture) : throw new ArgumentOutOfRangeException(nameof(limit)));
    // Обработчик смены любого стека (имя стека и состояние); null снимает / A handler of any stack change (the stack name and the state); null removes it
    public void OnUndoChanged(Action<string, UndoState>? callback) {
        Native.UndoFn? native = null;
        if (callback != null) native = (_, stack, flags) => Protect(() => callback(String(stack), UndoState.From(flags)));
        Check(Native.gran_app_on_undo(Handle, native, IntPtr.Zero));
        undoHandler = native;
    }
    // Переменные $имя выражений {…}: текст сохраняет тип переменной, число и bool задают его / The $name variables of the {…} expressions: a text keeps the variable type, a number and a bool set it
    public void SetVariable(string name, string value) => Check(Native.gran_app_variable_set_text(Handle, Arguments.Text(name), Arguments.Text(value)));
    public void SetVariable(string name, double value) => Check(Native.gran_app_variable_set_number(Handle, Arguments.Text(name), value));
    public void SetVariable(string name, bool value) => Check(Native.gran_app_variable_set_boolean(Handle, Arguments.Text(name), value ? 1 : 0));
    // Значение переменной текстом; null — переменной нет / The variable value as text; null — no such variable
    public string? Variable(string name) {
        Check(Native.gran_app_variable_read(Handle, Arguments.Text(name), out var value));
        return value == IntPtr.Zero ? null : OwnedString(value);
    }
    // Преобразователь «{выражение ; name}»; toSource == null — привязка односторонняя / The "{expression ; name}" converter; toSource == null — the binding is one-way
    public void SetConverter(string name, Func<string, string>? toView, Func<string, string>? toSource=null) {
        Native.ConvertFn? view = Adapt(toView), source = Adapt(toSource);
        Check(Native.gran_app_converter(Handle, Arguments.Text(name), view, source, IntPtr.Zero));
        if (view != null) callbacks.Add(view);
        if (source != null) callbacks.Add(source);
    }
    Native.ConvertFn? Adapt(Func<string, string>? function) {
        if (function == null) return null;
        return (_, value, result) => Protect(() => Native.gran_converted_set(result, function(String(value))));
    }
    // Пересчитать выражения сейчас, не дожидаясь кадра / Recompute the expressions now without waiting for a frame
    public void RefreshBindings() => Check(Native.gran_app_refresh_bindings(Handle));
    long AddTask(Action callback, bool repeat) {
        ArgumentNullException.ThrowIfNull(callback);
        long key = Interlocked.Increment(ref nextTask);
        tasks[key] = (callback, repeat); return key;
    }
    void DropTask(long key) {
        tasks.TryRemove(key, out _);
        if (keyTimers.TryRemove(key, out var timer)) timerKeys.TryRemove(timer, out _);
    }
    void RunTask(IntPtr user) {
        long key = (long)user;
        if (!tasks.TryGetValue(key, out var entry)) return;
        if (!entry.Repeat) DropTask(key);
        Protect(entry.Callback);
    }
    public void Post(Action callback) {
        long key = AddTask(callback, false);
        try { Check(Native.gran_app_post(Handle, taskEntry, (IntPtr)key)); }
        catch { DropTask(key); throw; }
    }
    public ulong Timer(double seconds, Action callback, bool repeat=false) {
        long key = AddTask(callback, repeat);
        var timer = new Native.Timer { Seconds=seconds, Repeat=repeat ? 1 : 0, Callback=taskEntry, User=(IntPtr)key };
        try { Check(Native.gran_app_timer(Handle, ref timer, out var result)); timerKeys[result] = key; keyTimers[key] = result; return result; }
        catch { DropTask(key); throw; }
    }
    public void ClearTimer(ulong timer) {
        Check(Native.gran_app_clear_timer(Handle, timer));
        if (timerKeys.TryGetValue(timer, out var key)) DropTask(key);
    }
    // Число задач и таймеров, ждущих вызова (для проверок) / The number of tasks and timers waiting for a call (for checks)
    public int PendingTasks => tasks.Count;
    public nuint ModelCount(string model) {
        Check(Native.gran_model_count(Handle, Arguments.Text(model), out var result)); return result;
    }
    public string ModelRead(string model, nuint index, string field) {
        Check(Native.gran_model_read(Handle, Arguments.Text(model), index, Arguments.Text(field), out var result));
        return OwnedString(result);
    }
    void Record(bool insert, string model, nuint index, IReadOnlyDictionary<string,string> fields) {
        using var args = new Arguments();
        var rows = fields.Select(row => new Native.Field { Name=args.Utf8(row.Key), Value=args.Utf8(row.Value) }).ToArray();
        var record = new Native.Record { Index=index, Fields=args.Array(rows), Count=(nuint)rows.Length };
        if (insert) Check(Native.gran_model_insert(Handle, Arguments.Text(model), ref record));
        else Check(Native.gran_model_set(Handle, Arguments.Text(model), ref record));
    }
    public void ModelInsert(string model, IReadOnlyDictionary<string,string> fields) => Record(true, model, nuint.MaxValue, fields);
    public void ModelInsert(string model, IReadOnlyDictionary<string,string> fields, nuint index) => Record(true, model, index, fields);
    public void ModelSet(string model, nuint index, IReadOnlyDictionary<string,string> fields) => Record(false, model, index, fields);
    public void ModelRemove(string model, nuint index) => Check(Native.gran_model_remove(Handle, Arguments.Text(model), index));
    public void ModelClear(string model) => Check(Native.gran_model_clear(Handle, Arguments.Text(model)));
    public int Run() {
        try { Check(Native.gran_app_run(Handle, out var result)); return result; }
        finally { Interlocked.Exchange(ref pending, null)?.Throw(); }
    }
}
