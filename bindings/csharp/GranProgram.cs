using System.Collections.Concurrent;
using System.Runtime.InteropServices;

namespace GranUI;

// Объявления функций окон и программы (1.8) общего C ABI / The declarations of the windows and program (1.8) functions of the shared C ABI
internal static class NativeProgram
{
    const string Library = "gran_c";
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void WindowClosedFn(IntPtr user, IntPtr result);
    [StructLayout(LayoutKind.Sequential)] internal struct WindowOptions {
        internal uint Size; internal IntPtr Owner; internal int Modal; internal WindowClosedFn? OnClosed; internal IntPtr User;
    }
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void WorkFn(IntPtr user, IntPtr task);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void TaskDoneFn(IntPtr user, ulong task, int status, IntPtr error);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void TaskProgressFn(IntPtr user, ulong task, double fraction, IntPtr text);
    [StructLayout(LayoutKind.Sequential)] internal struct Background {
        internal uint Size; internal WorkFn Work; internal TaskDoneFn Done; internal TaskProgressFn Progress; internal IntPtr User;
    }
    [DllImport(Library)] internal static extern int gran_app_run_task(IntPtr app, ref Background task, out ulong result);
    [DllImport(Library)] internal static extern int gran_app_cancel_task(IntPtr app, ulong task, out int found);
    [DllImport(Library)] internal static extern int gran_task_cancelled(IntPtr task);
    [DllImport(Library)] internal static extern int gran_task_progress(IntPtr task, double fraction, [MarshalAs(UnmanagedType.LPUTF8Str)] string text);
    [DllImport(Library)] internal static extern int gran_task_fail(IntPtr task, [MarshalAs(UnmanagedType.LPUTF8Str)] string message);
    [DllImport(Library)] internal static extern int gran_app_open_window(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string source, int isFile, ref WindowOptions options);
    [DllImport(Library)] internal static extern int gran_app_close_window(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string? result);
}

// Чем кончилась фоновая задача (GRAN_TASK_*) / How a background task ended (GRAN_TASK_*)
public enum BackgroundStatus { Done = 0, Cancelled = 1, Failed = 2 }

// Контекст фоновой работы: действителен только внутри неё / A background work context: valid only inside it
public sealed class BackgroundTask
{
    readonly IntPtr handle;
    internal BackgroundTask(IntPtr task) { handle = task; }
    public bool Cancelled => NativeProgram.gran_task_cancelled(handle) != 0;
    public void Progress(double fraction, string text = "") => NativeProgram.gran_task_progress(handle, fraction, Arguments.Text(text));
}

// Окна и программа: владельцы, модальные окна, фоновые задачи / Windows and the program: owners, modal windows, background tasks
public sealed partial class Application
{
    // Фоновые задачи: user — номер записи, переходники общие / The background tasks: user is the entry number, the trampolines are shared
    sealed class BackgroundEntry {
        internal required Func<BackgroundTask, object?> Work;
        internal Action<BackgroundStatus, object?, string>? Done;
        internal Action<double, string>? Progress;
        internal object? Result;
    }
    readonly ConcurrentDictionary<long, BackgroundEntry> background = new();
    NativeProgram.WorkFn? workEntry;
    NativeProgram.TaskDoneFn? doneEntry;
    NativeProgram.TaskProgressFn? progressEntry;
    // Поток пула: исключение не пересекает P/Invoke — оно становится сбоем задачи / A pool thread: an exception does not cross P/Invoke — it becomes the task failure
    void RunWork(IntPtr user, IntPtr task) {
        if (!background.TryGetValue((long)user, out var entry)) return;
        try { entry.Result = entry.Work(new BackgroundTask(task)); }
        catch (Exception error) { NativeProgram.gran_task_fail(task, error.Message); }
    }
    void TaskDone(IntPtr user, ulong task, int status, IntPtr error) {
        if (!background.TryRemove((long)user, out var entry) || entry.Done == null) return;
        object? result = status == 0 ? entry.Result : null;
        string text = String(error);
        Protect(() => entry.Done((BackgroundStatus)status, result, text));
    }
    void TaskProgress(IntPtr user, ulong task, double fraction, IntPtr text) {
        if (!background.TryGetValue((long)user, out var entry) || entry.Progress == null) return;
        string caption = String(text);
        Protect(() => entry.Progress(fraction, caption));
    }
    // work — в потоке пула, done и progress — в потоке UI / work runs on a pool thread, done and progress — on the UI thread
    public ulong RunTask(Func<BackgroundTask, object?> work, Action<BackgroundStatus, object?, string>? done=null, Action<double, string>? progress=null) {
        ArgumentNullException.ThrowIfNull(work);
        workEntry ??= RunWork; doneEntry ??= TaskDone; progressEntry ??= TaskProgress;
        long key = Interlocked.Increment(ref nextTask);
        background[key] = new BackgroundEntry { Work=work, Done=done, Progress=progress };
        var task = new NativeProgram.Background { Size=(uint)Marshal.SizeOf<NativeProgram.Background>(), Work=workEntry, Done=doneEntry, Progress=progressEntry, User=(IntPtr)key };
        try { Check(NativeProgram.gran_app_run_task(Handle, ref task, out var result)); return result; }
        catch { background.TryRemove(key, out _); throw; }
    }
    // Попросить отменить; false — задачи уже нет / Ask to cancel; false — the task is gone
    public bool CancelTask(ulong task) {
        Check(NativeProgram.gran_app_cancel_task(Handle, task, out var found));
        return found != 0;
    }
    // Обработчики закрытия окон: user — номер обработчика, один переходник на все окна / The window close handlers: user is the handler number, one trampoline for all the windows
    readonly ConcurrentDictionary<long, Action<string>> windowHandlers = new();
    NativeProgram.WindowClosedFn? windowEntry;
    void WindowClosed(IntPtr user, IntPtr result) {
        if (!windowHandlers.TryRemove((long)user, out var handler)) return;
        string text = String(result);
        Protect(() => handler(text));
    }
    // Ещё одно окно; owner — id узла окна-владельца, onClosed(result) — после закрытия / One more window; owner is the id of an owner window node, onClosed(result) — after closing
    public void OpenWindow(string source, bool isFile=true, string? owner=null, bool modal=false, Action<string>? onClosed=null) {
        windowEntry ??= WindowClosed;
        using var args = new Arguments();
        long key = 0;
        if (onClosed != null) { key = Interlocked.Increment(ref nextTask); windowHandlers[key] = onClosed; }
        var options = new NativeProgram.WindowOptions { Size=(uint)Marshal.SizeOf<NativeProgram.WindowOptions>(),
            Owner=owner != null ? args.Utf8(owner) : IntPtr.Zero, Modal=modal ? 1 : 0,
            OnClosed=onClosed != null ? windowEntry : null, User=(IntPtr)key };
        try { Check(NativeProgram.gran_app_open_window(Handle, Arguments.Text(source), isFile ? 1 : 0, ref options)); }
        catch { windowHandlers.TryRemove(key, out _); throw; }
    }
    // Закрыть окно узла target с результатом для onClosed / Close the window of the target node with a result for onClosed
    public void CloseWindow(string target, string? result=null) {
        if (result != null) Arguments.Text(result);
        Check(NativeProgram.gran_app_close_window(Handle, Arguments.Text(target), result));
    }
}
