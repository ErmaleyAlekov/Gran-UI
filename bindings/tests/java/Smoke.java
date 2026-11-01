import granui.Application;
import java.nio.file.Paths;
import java.util.Map;
import java.lang.foreign.*;
import static java.lang.foreign.ValueLayout.*;

public class Smoke {
    private static void pressButton() {
        try (Arena scope=Arena.ofConfined()) {
            var lookup=SymbolLookup.libraryLookup("user32",scope);
            var find=Linker.nativeLinker().downcallHandle(lookup.find("FindWindowW").orElseThrow(),FunctionDescriptor.of(ADDRESS,ADDRESS,ADDRESS));
            var post=Linker.nativeLinker().downcallHandle(lookup.find("PostMessageW").orElseThrow(),FunctionDescriptor.of(JAVA_INT,ADDRESS,JAVA_INT,JAVA_LONG,JAVA_LONG));
            var title=scope.allocateFrom(JAVA_CHAR,"BindingsJava\0".toCharArray());
            MemorySegment window=(MemorySegment)find.invokeExact(MemorySegment.NULL,title);
            localStaticCheck(window.address()!=0);
            localStaticCheck((int)post.invokeExact(window,0x100,9L,0L)!=0);
            localStaticCheck((int)post.invokeExact(window,0x100,13L,0L)!=0);
        } catch (Throwable error) { throw new IllegalStateException(error); }
    }
    // Windows — D3D11 и ввод клавиш через PostMessage; Linux — Vulkan (CPU), ввод клавиш пропускается
    // Windows — D3D11 and key input through PostMessage; Linux — Vulkan (CPU), key input is skipped
    private static final boolean WINDOWS = System.getProperty("os.name").startsWith("Windows");
    private static int backend() {
        if (WINDOWS) return 1;
        return 3;
    }
    public static void main(String[] args) {
        if (args.length != 1) {
            System.err.println("Usage: java Smoke <library_absolute_path>");
            System.exit(1);
        }

        String libraryPath = args[0];
        try {
            // Initialize the Application with the provided library path
            Application app = new Application(libraryPath, backend(), 0.3, true);

            // Load XML UI definition
            String xml="<Window title=\"BindingsJava\"><Stack id=\"root\"><Button id=\"go\" text=\"Пуск 🌲\" onClick=\"business\"/><Text id=\"text\">Привет 🌲</Text></Stack></Window>";
            app.loadString(xml);

            // Check if the application is still open
            localStaticCheck(app.read("text","text").equals("Привет 🌲"));

            // Своё событие всплывает от go к root; отменённое — dispatch возвращает false
            // A custom event bubbles from go to root; a cancelled one — dispatch returns false
            java.util.List<String> seen=new java.util.ArrayList<>();
            long listener=app.listen("root","ping",e -> { seen.add(e.type+":"+e.target+":"+e.current+":"+e.phase+":"+e.detail); e.preventDefault(); });
            localStaticCheck(!app.dispatch("go","ping","data"));
            localStaticCheck(seen.size()==1 && seen.get(0).equals("ping:go:root:3:data"));
            app.unlisten("root",listener);
            localStaticCheck(app.dispatch("go","ping"));

            // Стек отмены: шаг кладётся выполненным, undo/redo вызывают обработчики, «чистое» состояние
            // The undo stack: a step is pushed done, undo/redo call the handlers, the clean state
            int[] value={1};
            String[] changed={"-"};
            app.onUndoChanged((stack,state) -> changed[0]=stack);
            app.pushUndo(() -> value[0]=0, () -> value[0]=1);
            localStaticCheck(app.undoState().equals(new Application.UndoState(true,false,false)) && changed[0].isEmpty());
            app.undo();
            localStaticCheck(value[0]==0 && app.undoState().clean() && app.undoState().canRedo());
            app.redo();
            localStaticCheck(value[0]==1);
            app.setUndoClean(null);
            app.setUndoLimit(10,"doc");
            app.clearUndo(null);
            localStaticCheck(!app.undoState().canUndo());
            app.onUndoChanged(null);

            // Живая разметка: запись атрибута, удаление, XML с правкой / The live markup: writing an attribute, removing it, XML with the edit
            app.setMarkupAttribute("root","padding","6");
            localStaticCheck("6".equals(app.markupAttribute("root","padding")) && app.saveMarkup().contains("padding=\"6\""));
            app.setMarkupAttribute("root","padding",null);
            localStaticCheck(app.markupAttribute("root","padding")==null);

            // Переменные и преобразователь выражений {…} / The variables and the converter of the {…} expressions
            app.setConverter("twice",text -> text + text,null);
            app.setVariable("count",21.0);
            app.setVariable("flag",true);
            app.setVariable("flag","false");
            app.refreshBindings();
            localStaticCheck("21".equals(app.variable("count")) && "false".equals(app.variable("flag")) && app.variable("missing")==null);

            // Apply updates to the UI
            app.apply(new Application.Update("text", "text", "Hello World 🌍"));

            // Read the updated text
            String text = app.read("text", "text");
            localStaticCheck(text.equals("Hello World 🌍"));
            System.out.println("Updated text: " + text);

            // Use a Map to demonstrate model operations
            Map<String, String> modelData = Map.of("name", "Alice", "age", "30");
            app.modelInsert("users", modelData);
            long count = app.modelCount("users");
            localStaticCheck(count==1);
            app.modelSet("users",0,Map.of("name","Анна 🌲"));
            localStaticCheck(app.modelRead("users",0,"name").equals("Анна 🌲"));
            app.modelRemove("users",0);
            localStaticCheck(app.modelCount("users")==0);
            app.modelInsert("users",Map.of()); app.modelClear("users");
            localStaticCheck(app.modelCount("users")==0);
            System.out.println("Model count: " + count);

            // Set a timer to check the posted text
            boolean[] timerHappened = {false};
            boolean[] acted={false};
            app.onAction("business",e -> {
                localStaticCheck(e.target().equals("go") && e.text().equals("Пуск 🌲") && e.model().isEmpty() && e.index()==-1L);
                acted[0]=true; app.command("close");
            });
            app.post(() -> app.apply(new Application.Update("text","text","posted 🌲")));
            app.timer(0.01, () -> {
                String postedText = app.read("text", "text");
                System.out.println("Posted text: " + postedText);
                timerHappened[0] = true;
                localStaticCheck(postedText.equals("posted 🌲"));
                if (!WINDOWS) {
                    System.out.println("key injection skipped (Windows only)");
                    acted[0]=true; app.command("close"); return;
                }
                pressButton();
            }, false);
            int[] posted={0};
            for (int index=0; index<1000; ++index) app.post(() -> ++posted[0]);
            long interval=app.timer(0.005,() -> {},true);
            app.timer(0.02,() -> app.clearTimer(interval),false);
            System.gc();

            // Run the application
            int result = app.run();
            System.out.println("Application run result: " + result);

            // Assert that the timer happened
            localStaticCheck(timerHappened[0] && acted[0]);
            localStaticCheck(posted[0]==1000);
            // Одноразовые задачи и снятый таймер не держат переходников / One-shot tasks and a cleared timer keep no trampolines
            localStaticCheck(app.pendingTasks()==0);

            // Close the application
            app.close();
            try (Application failing=new Application(libraryPath,backend(),0.3,true)) {
                failing.loadString(xml);
                failing.timer(0.01,() -> {throw new IllegalArgumentException("marker");},false);
                boolean failed=false;
                try { failing.run(); } catch (IllegalArgumentException error) { failed=error.getMessage().equals("marker"); }
                localStaticCheck(failed);
            }
            try (Application busy=new Application(libraryPath,backend(),0.3,true)) {
                busy.loadString(xml);
                boolean[] survived={false};
                busy.timer(0.01,() -> {
                    try {busy.close();} catch (IllegalStateException error) {survived[0]=true;}
                    localStaticCheck(busy.read("text","text").equals("Привет 🌲"));
                    busy.command("close");
                },false);
                busy.run(); localStaticCheck(survived[0]);
            }

            // Print success message
            System.out.println("Java smoke OK");

        } catch (AssertionError e) {
            System.err.println("Assertion failed: " + e.getMessage());
            System.exit(1);
        } catch (Exception e) {
            System.err.println("An error occurred: " + e.getMessage());
            System.exit(1);
        }
    }

    private static void localStaticCheck(boolean condition) throws AssertionError {
        if (!condition) {
            throw new AssertionError("Local static check failed");
        }
    }
}
