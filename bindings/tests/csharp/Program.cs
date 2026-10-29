using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

class Program
{
    [DllImport("user32", CharSet=CharSet.Unicode)]
    static extern IntPtr FindWindowW(string? name, string title);
    [DllImport("user32")]
    static extern bool PostMessageW(IntPtr window, uint message, nuint key, nint data);
    static void Assert(bool condition)
    {
        if (!condition)
            throw new Exception("Assertion failed");
    }

    // Windows — D3D11 и ввод клавиш через PostMessage; Linux — Vulkan (CPU), ввод клавиш пропускается
    // Windows — D3D11 and key input through PostMessage; Linux — Vulkan (CPU), key input is skipped
    static readonly bool Windows = OperatingSystem.IsWindows();
    static int Backend()
    {
        if (Windows)
            return 1;
        return 3;
    }

    static void Main()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();

        var app = new GranUI.Application(backend: Backend(), exitAfter: 0.3, warp: true);

        try
        {
            const string xml = "<Window title=\"BindingsCSharp\"><Stack id=\"root\"><Button id=\"go\" text=\"Пуск 🌲\" onClick=\"business\"/><Text id=\"text\">Привет 🌲</Text></Stack></Window>";
            app.LoadString(xml);

            Assert(app.Read("text", "text") == "Привет 🌲");

            // Своё событие всплывает от go к root; отменённое — Dispatch возвращает false
            // A custom event bubbles from go to root; a cancelled one — Dispatch returns false
            var seen = new List<string>();
            ulong listener = app.Listen("root", "ping", e => { seen.Add(e.Type + ":" + e.Target + ":" + e.Current + ":" + e.Phase + ":" + e.Detail); e.PreventDefault(); });
            Assert(!app.Dispatch("go", "ping", "data"));
            Assert(seen.Count == 1 && seen[0] == "ping:go:root:3:data");
            app.Unlisten("root", listener);
            Assert(app.Dispatch("go", "ping"));

            var model = new Dictionary<string, string>
            {
                { "key1", "value1" },
                { "key2", "value2" }
            };

            app.ModelInsert("model", model);
            Assert(app.ModelCount("model") == 1);
            Assert(app.ModelRead("model", 0, "key1") == "value1");
            Assert(app.ModelRead("model", 0, "key2") == "value2");

            app.ModelSet("model", 0, new Dictionary<string, string> { { "key1", "new_value1" } });
            Assert(app.ModelRead("model", 0, "key1") == "new_value1");

            app.ModelRemove("model", 0);
            Assert(app.ModelCount("model") == 0);

            app.ModelClear("model");
            Assert(app.ModelCount("model") == 0);

            bool flag = false;
            bool acted = false;
            app.OnAction("business", e => {
                Assert(e.Target == "go" && e.Text == "Пуск 🌲" && e.Model == "" && e.Index == nuint.MaxValue);
                acted = true;
                app.Command("close");
            });
            app.Post(() =>
            {
                app.Apply(("text", "text", "Updated Text 🌳"));
            });

            app.Timer(0.01, () =>
            {
                Assert(app.Read("text", "text") == "Updated Text 🌳");
                flag = true;
                if (!Windows)
                {
                    Console.WriteLine("key injection skipped (Windows only)");
                    acted = true;
                    app.Command("close");
                    return;
                }
                IntPtr window = FindWindowW(null, "BindingsCSharp");
                Assert(window != IntPtr.Zero);
                Assert(PostMessageW(window, 0x100, 9, 0));
                Assert(PostMessageW(window, 0x100, 13, 0));
            });
            int posted = 0;
            for (int index = 0; index < 1000; ++index)
                app.Post(() => ++posted);
            ulong interval = app.Timer(0.005, () => { }, true);
            app.Timer(0.02, () => app.ClearTimer(interval));

            GC.Collect();
            GC.WaitForPendingFinalizers();
            app.Run();
            Assert(flag && acted);
            Assert(posted == 1000);
            // Одноразовые задачи и снятый таймер не держат делегатов / One-shot tasks and a cleared timer keep no delegates
            Assert(app.PendingTasks == 0);

            using var failing = new GranUI.Application(backend:Backend(), exitAfter:0.3, warp:true);
            failing.LoadString(xml);
            failing.Timer(0.01, () => { throw new InvalidOperationException("marker"); });
            bool failed = false;
            try
            {
                failing.Run();
            }
            catch (InvalidOperationException ex)
            {
                Assert(ex.Message == "marker");
                failed = true;
            }
            Assert(failed);

            using var busy = new GranUI.Application(backend:Backend(), exitAfter:0.3, warp:true);
            busy.LoadString(xml);
            bool survived = false;
            busy.Timer(0.01, () => {
                try { busy.Dispose(); }
                catch (InvalidOperationException) { survived = true; }
                Assert(busy.Read("text","text") == "Привет 🌲");
                busy.Command("close");
            });
            busy.Run();
            Assert(survived);

            app.Dispose();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Test failed: {ex.Message}");
            Console.WriteLine(ex.StackTrace);
            Environment.ExitCode = 1;
            return;
        }

        Console.WriteLine("CSharp smoke OK");
    }
}
