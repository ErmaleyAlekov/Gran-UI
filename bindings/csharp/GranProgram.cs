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
    [DllImport(Library)] internal static extern int gran_app_open_window(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string source, int isFile, ref WindowOptions options);
    [DllImport(Library)] internal static extern int gran_app_close_window(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string target, [MarshalAs(UnmanagedType.LPUTF8Str)] string? result);
}

// Окна и программа: владельцы и модальные окна / Windows and the program: owners and modal windows
public sealed partial class Application
{
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
