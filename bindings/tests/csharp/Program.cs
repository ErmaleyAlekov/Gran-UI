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

            // Модели 1.7: дерево, плоское представление, прокси с сортировкой, выделение
            // Models 1.7: a tree, a flat view, a sorted proxy, a selection
            ulong top = app.TreeInsert("org", 0, new Dictionary<string,string> { ["name"] = "Б" });
            app.TreeInsert("org", top, new Dictionary<string,string> { ["name"] = "Б1" });
            app.TreeInsert("org", 0, new Dictionary<string,string> { ["name"] = "А" });
            app.CreateTreeList("orgRows", "org", "name", null, 1);
            Assert(app.ModelCount("orgRows") == 3 && app.ModelRead("orgRows", 0, "name") == "А" && app.TreeRowOf("orgRows", 2).Depth == 1);
            foreach (var number in new[] { "2", "10", "1" })
                app.ModelInsert("nums", new Dictionary<string,string> { ["n"] = number });
            app.CreateProxy("sorted", "nums", "n:number desc");
            Assert(app.ModelRead("sorted", 0, "n") == "10" && app.SourceIndex("sorted", 0) == 1);
            app.CreateSelection("pick", "sorted");
            string picked = "";
            app.OnSelectionChanged("pick", name => picked = name);
            app.SelectionChange("pick", GranUI.SelectionCommand.Range, 0, 1);
            var chosen = app.Selection("pick");
            Assert(picked == "pick" && chosen.Current == null && chosen.Selected.Length == 2);

            // Своё событие всплывает от go к root; отменённое — Dispatch возвращает false
            // A custom event bubbles from go to root; a cancelled one — Dispatch returns false
            var seen = new List<string>();
            ulong listener = app.Listen("root", "ping", e => { seen.Add(e.Type + ":" + e.Target + ":" + e.Current + ":" + e.Phase + ":" + e.Detail); e.PreventDefault(); });
            Assert(!app.Dispatch("go", "ping", "data"));
            Assert(seen.Count == 1 && seen[0] == "ping:go:root:3:data");
            app.Unlisten("root", listener);
            Assert(app.Dispatch("go", "ping"));

            // Стек отмены: шаг кладётся выполненным, Undo/Redo вызывают обработчики, «чистое» состояние
            // The undo stack: a step is pushed done, Undo/Redo call the handlers, the clean state
            int value = 1;
            string changed = "-";
            app.OnUndoChanged((stack, state) => changed = stack);
            app.PushUndo(() => value = 0, () => value = 1);
            Assert(app.GetUndoState() == new GranUI.UndoState(true, false, false) && changed == "");
            app.Undo();
            Assert(value == 0 && app.GetUndoState().Clean && app.GetUndoState().CanRedo);
            app.Redo();
            Assert(value == 1);
            app.SetUndoClean();
            app.SetUndoLimit(10, "doc");
            app.ClearUndo();
            Assert(!app.GetUndoState().CanUndo);
            app.OnUndoChanged(null);

            // Живая разметка: запись атрибута, удаление, XML с правкой / The live markup: writing an attribute, removing it, XML with the edit
            app.SetMarkupAttribute("root", "padding", "6");
            Assert(app.MarkupAttribute("root", "padding") == "6" && app.SaveMarkup().Contains("padding=\"6\""));
            Assert(app.ComponentSchema("urn:none") == "");
            app.SetMarkupAttribute("root", "padding", null);
            Assert(app.MarkupAttribute("root", "padding") == null);

            // Переменные и преобразователь выражений {…} / The variables and the converter of the {…} expressions
            app.SetConverter("twice", value => value + value);
            app.SetVariable("count", 21.0);
            app.SetVariable("flag", true);
            app.SetVariable("flag", "false");
            app.RefreshBindings();
            Assert(app.Variable("count") == "21" && app.Variable("flag") == "false" && app.Variable("missing") == null);

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

            // Модальное окно с владельцем: результат CloseWindow приходит в onClosed / A modal window with an owner: the CloseWindow result comes to onClosed
            using var windows = new GranUI.Application(backend:Backend(), exitAfter:3.0, warp:true);
            windows.LoadString(xml);
            string? closedWith = null;
            windows.Timer(0.05, () => windows.OpenWindow("<Window title=\"Dialog\" width=\"200\" height=\"120\" kind=\"dialog\"><Text id=\"dlg\">?</Text></Window>",
                isFile:false, owner:"root", modal:true, onClosed:result => { closedWith = result; windows.Command("close"); }));
            windows.Timer(0.4, () => windows.CloseWindow("dlg", "ok"));
            windows.Run();
            Assert(closedWith == "ok");

            // Фоновая задача: результат, прогресс и сбой — в потоке UI / A background task: the result, the progress and the failure — on the UI thread
            using var tasks = new GranUI.Application(backend:Backend(), exitAfter:3.0, warp:true);
            tasks.LoadString(xml);
            var outcomes = new List<(GranUI.BackgroundStatus, object?, string)>();
            double lastStep = 0;
            Action<GranUI.BackgroundStatus, object?, string> finish = (status, result, error) => {
                outcomes.Add((status, result, error));
                if (outcomes.Count == 2) tasks.Command("close");
            };
            tasks.RunTask(task => { for (int step = 1; step <= 10; ++step) task.Progress(step / 10.0, "шаг"); return 42; }, finish, (fraction, _) => lastStep = fraction);
            tasks.RunTask(_ => throw new InvalidOperationException("broken"), finish);
            tasks.Run();
            Assert(outcomes.Contains((GranUI.BackgroundStatus.Done, 42, "")));
            Assert(outcomes.Contains((GranUI.BackgroundStatus.Failed, null, "broken")));
            Assert(lastStep == 1.0);
            Assert(!tasks.CancelTask(12345));

            // Журнал процесса: запись попадает в файл / The process log: a record goes into the file
            string logFolder = Path.Combine(Path.GetTempPath(), "gran_log_cs_" + Environment.ProcessId);
            tasks.OpenLog("GranCSharpTest", logFolder, crashReports:false);
            tasks.Log(GranUI.LogLevel.Info, "test", "запись журнала");
            Assert(tasks.LogPath().EndsWith("GranCSharpTest.log"));
            using (var reader = new StreamReader(new FileStream(tasks.LogPath(), FileMode.Open, FileAccess.Read, FileShare.ReadWrite)))
                Assert(reader.ReadToEnd().Contains("| test | запись журнала"));
            Assert(tasks.PreviousCrash() == "");

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
