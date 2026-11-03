package granui;

import java.lang.foreign.*;
import java.lang.invoke.*;
import java.nio.file.Path;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;
import static java.lang.foreign.ValueLayout.*;

// Только вызовы общего C ABI; JDK 22+, процесс 64 bit.
// Only calls to the shared C ABI; JDK 22+, 64-bit process.
public final class Application implements AutoCloseable {
    // Классы Win32 и COM могут переживать одно приложение: библиотека живёт до выхода JVM.
    // Win32 classes and COM may outlive an application: the library lives until JVM exit.
    private static final ConcurrentMap<Path,SymbolLookup> LIBRARIES = new ConcurrentHashMap<>();
    public record Update(String target, String property, String value) {}
    public record Event(String target, String model, long index, String text, String option,
                        String message, double value, boolean checked) {}
    private final Arena lifetime = Arena.ofShared();
    private final SymbolLookup symbols;
    private final ConcurrentMap<String,MethodHandle> functions = new ConcurrentHashMap<>();
    private final Queue<Object> callbacks = new ConcurrentLinkedQueue<>();
    private final AtomicReference<Throwable> pending = new AtomicReference<>();
    // Один переходник на все задачи и таймеры: user — номер задачи (иначе каждый post оставлял бы upcall stub в арене до close).
    // One trampoline for all tasks and timers: user is the task number (otherwise every post would leave an upcall stub in the arena until close).
    private record Pending(Runnable callback, boolean repeat) {}
    private final ConcurrentMap<Long,Pending> tasks = new ConcurrentHashMap<>();
    private final ConcurrentMap<Long,Long> timerKeys = new ConcurrentHashMap<>();
    private final ConcurrentMap<Long,Long> keyTimers = new ConcurrentHashMap<>();
    private final AtomicLong nextTask = new AtomicLong();
    private MemorySegment taskEntry;
    private MemorySegment app = MemorySegment.NULL;
    // Шаги отмены: user — номер пары обработчиков; release снимает её, когда стек отбросил шаг.
    // The undo steps: user is the number of a handler pair; release removes it when the stack dropped the step.
    private record UndoPair(Runnable undo, Runnable redo) {}
    private final ConcurrentMap<Long,UndoPair> undoSteps = new ConcurrentHashMap<>();
    private MemorySegment undoEntry, redoEntry, releaseEntry;
    // Состояние стека отмены: можно ли отменить и повторить, «чистое» ли оно (документ сохранён).
    // An undo stack state: whether undo and redo are possible, whether it is clean (the document is saved).
    public record UndoState(boolean canUndo, boolean canRedo, boolean clean) {
        static UndoState of(int flags) { return new UndoState((flags & 1) != 0, (flags & 2) != 0, (flags & 4) != 0); }
    }
    // Обработчик смены стека: имя стека и состояние / A stack change handler: the stack name and the state
    public interface UndoListener { void changed(String stack, UndoState state); }

    public Application(String library) { this(library, 0, 0, false); }
    public Application(String library, int backend, double exitAfter, boolean warp) {
        if (ADDRESS.byteSize() != 8) { lifetime.close(); throw new UnsupportedOperationException("64-bit process required"); }
        try {
            Path path=Path.of(library).toAbsolutePath().normalize();
            symbols = LIBRARIES.computeIfAbsent(path, key -> SymbolLookup.libraryLookup(key,Arena.global()));
            if ((int)invoke("gran_abi_version", JAVA_INT) != 1) throw new UnsupportedOperationException("unsupported Gran UI ABI");
            try (Arena args = Arena.ofConfined()) {
                MemorySegment options = args.allocate(32, 8);
                options.set(JAVA_INT, 0, 32); options.set(JAVA_INT, 4, backend);
                options.set(JAVA_INT, 12, warp ? 1 : 0); options.set(JAVA_DOUBLE, 24, exitAfter);
                MemorySegment output = args.allocate(ADDRESS);
                check((int)invoke("gran_app_create", JAVA_INT, options, output));
                app = output.get(ADDRESS, 0);
            }
        } catch (RuntimeException | Error failure) { lifetime.close(); throw failure; }
    }

    // Описатель строится по типам C ABI; данные UI никогда не интерпретируются Java.
    // The descriptor follows C ABI types; Java never interprets UI data.
    private Object invoke(String name, MemoryLayout result, Object... args) {
        try {
            MethodHandle function = functions.get(name);
            if (function == null) {
                MemoryLayout[] layouts = new MemoryLayout[args.length];
                for (int i=0; i<args.length; ++i) {
                    if (args[i] instanceof MemorySegment) layouts[i] = ADDRESS;
                    else if (args[i] instanceof Integer) layouts[i] = JAVA_INT;
                    else if (args[i] instanceof Long) layouts[i] = JAVA_LONG;
                    else if (args[i] instanceof Double) layouts[i] = JAVA_DOUBLE;
                    else throw new IllegalArgumentException("unsupported ABI argument");
                }
                FunctionDescriptor descriptor = result == null ? FunctionDescriptor.ofVoid(layouts) : FunctionDescriptor.of(result, layouts);
                function = Linker.nativeLinker().downcallHandle(symbols.find(name).orElseThrow(), descriptor);
                functions.put(name, function);
            }
            return function.invokeWithArguments(args);
        } catch (RuntimeException | Error error) { throw error; }
        catch (Throwable error) { throw new IllegalStateException(error); }
    }
    private static String text(MemorySegment value) {
        if (value.address() == 0) return "";
        return value.reinterpret(1L << 30).getString(0);
    }
    private static MemorySegment utf8(Arena arena, String value) {
        if (value == null) return MemorySegment.NULL;
        if (value.indexOf('\0') >= 0) throw new IllegalArgumentException("embedded NUL");
        return arena.allocateFrom(value);
    }
    private void check(int status) {
        if (status != 0) throw new IllegalStateException(text((MemorySegment)invoke("gran_last_error", ADDRESS)));
    }
    private void ensureOpen() {
        if (app.address() == 0) throw new IllegalStateException("application is closed");
    }
    private void status(String name, Object... args) { ensureOpen(); check((int)invoke(name, JAVA_INT, args)); }
    private String ownedString(String name, Object... args) {
        ensureOpen();
        try (Arena scope = Arena.ofConfined()) {
            MemorySegment output = scope.allocate(ADDRESS);
            Object[] params = Arrays.copyOf(args, args.length+1); params[args.length] = output;
            status(name, params);
            MemorySegment value = output.get(ADDRESS, 0);
            try { return text(value); }
            finally { invoke("gran_string_free", null, value); }
        }
    }
    public void loadString(String xml) { load(xml, 0); }
    public void loadFile(String path) { load(path, 1); }
    private void load(String source, int isFile) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_load", app, utf8(scope, source), isFile); }
    }
    public String read(String target, String property) {
        try (Arena scope=Arena.ofConfined()) { return ownedString("gran_app_read", app, utf8(scope,target), utf8(scope,property)); }
    }
    // Атрибут узла в живой разметке; null — атрибута нет / The node attribute in the live markup; null — no attribute
    public String markupAttribute(String target, String name) {
        ensureOpen();
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(ADDRESS);
            status("gran_app_markup_read",app,utf8(scope,target),utf8(scope,name),output);
            MemorySegment value=output.get(ADDRESS,0);
            if (value.address() == 0) return null;
            try { return text(value); }
            finally { invoke("gran_string_free", null, value); }
        }
    }
    // Записать атрибут в разметку (null — удалить) и перестроить узел / Write the attribute into the markup (null removes it) and rebuild the node
    public void setMarkupAttribute(String target, String name, String value) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_markup_write",app,utf8(scope,target),utf8(scope,name),utf8(scope,value)); }
    }
    // Живая разметка окна (null — главного) как XML / The live markup of the window (null — the main one) as XML
    public String saveMarkup() { return saveMarkup(null); }
    public String saveMarkup(String window) {
        try (Arena scope=Arena.ofConfined()) { return ownedString("gran_app_markup_save",app,utf8(scope,window)); }
    }
    // Сгенерированная XSD библиотеки компонентов ("" — такой нет) / The generated XSD of a component library ("" — no such one)
    public String componentSchema(String namespaceUri) { return componentSchema(namespaceUri, null); }
    public String componentSchema(String namespaceUri, String window) {
        try (Arena scope=Arena.ofConfined()) { return ownedString("gran_app_component_schema",app,utf8(scope,namespaceUri),utf8(scope,window)); }
    }
    public void apply(Update... updates) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment rows = updates.length == 0 ? MemorySegment.NULL : scope.allocate(Math.multiplyExact(24L,updates.length),8);
            for (int i=0; i<updates.length; ++i) {
                rows.set(ADDRESS,24L*i,utf8(scope,updates[i].target()));
                rows.set(ADDRESS,24L*i+8,utf8(scope,updates[i].property()));
                rows.set(ADDRESS,24L*i+16,utf8(scope,updates[i].value()));
            }
            status("gran_app_apply",app,rows,(long)updates.length);
        }
    }
    public void command(String name) { command(name,null,null); }
    public void command(String name, String target, String argument) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_command",app,utf8(scope,name),utf8(scope,target),utf8(scope,argument)); }
    }
    private void protect(Runnable callback) {
        try { callback.run(); }
        catch (Throwable error) {
            pending.compareAndSet(null,error);
            try { command("close"); } catch (Throwable ignored) { /* Ошибка сохранена. / The error is retained. */ }
        }
    }
    private final class Callback {
        private final Consumer<Event> action;
        private final Runnable task;
        Callback(Consumer<Event> action, Runnable task) { this.action=action; this.task=task; }
        void action(MemorySegment user, MemorySegment pointer) {
            protect(() -> {
                MemorySegment e=pointer.reinterpret(64);
                action.accept(new Event(text(e.get(ADDRESS,0)),text(e.get(ADDRESS,8)),e.get(JAVA_LONG,16),
                    text(e.get(ADDRESS,24)),text(e.get(ADDRESS,32)),text(e.get(ADDRESS,40)),e.get(JAVA_DOUBLE,48),e.get(JAVA_INT,56)!=0));
            });
        }
        void task(MemorySegment user) { protect(task); }
    }
    private MemorySegment stub(Callback callback, boolean action) {
        ensureOpen();
        try {
            MethodType type = action ? MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class) : MethodType.methodType(void.class,MemorySegment.class);
            FunctionDescriptor descriptor = action ? FunctionDescriptor.ofVoid(ADDRESS,ADDRESS) : FunctionDescriptor.ofVoid(ADDRESS);
            MethodHandle method=MethodHandles.lookup().findVirtual(Callback.class, action ? "action" : "task", type).bindTo(callback);
            MemorySegment result=Linker.nativeLinker().upcallStub(method,descriptor,lifetime);
            callbacks.add(callback); return result;
        } catch (ReflectiveOperationException error) { throw new IllegalStateException(error); }
    }
    public void onAction(String name, Consumer<Event> callback) { on(name,callback,false); }
    public void onMessage(String name, Consumer<Event> callback) { on(name,callback,true); }
    private void on(String name, Consumer<Event> callback, boolean message) {
        MemorySegment function=stub(new Callback(Objects.requireNonNull(callback),null),true);
        try (Arena scope=Arena.ofConfined()) {
            status(message ? "gran_app_on_message" : "gran_app_on_action",app,utf8(scope,name),function,MemorySegment.NULL);
        }
    }
    // Снимок события интерфейса (gran_input_event, 168 байт); методы ставят флаги, которые уходят ядру после возврата.
    // A UI event snapshot (gran_input_event, 168 bytes); the methods set flags that go to the core after the return.
    public static final class UiEvent {
        public final String type, target, current, related, key, detail, dragFormat, dragData, model, relatedModel;
        public final int phase, button, clicks, modifiers;
        public final long index, relatedIndex;
        public final double x, y, windowX, windowY, wheelX, wheelY;
        public final boolean dropped;
        int flags;
        UiEvent(MemorySegment e) {
            phase=e.get(JAVA_INT,4); type=text(e.get(ADDRESS,8)); target=text(e.get(ADDRESS,16));
            current=text(e.get(ADDRESS,24)); related=text(e.get(ADDRESS,32)); key=text(e.get(ADDRESS,40));
            detail=text(e.get(ADDRESS,48)); dragFormat=text(e.get(ADDRESS,56)); dragData=text(e.get(ADDRESS,64));
            model=text(e.get(ADDRESS,72)); relatedModel=text(e.get(ADDRESS,80));
            index=e.get(JAVA_LONG,88); relatedIndex=e.get(JAVA_LONG,96);
            x=e.get(JAVA_DOUBLE,104); y=e.get(JAVA_DOUBLE,112); windowX=e.get(JAVA_DOUBLE,120); windowY=e.get(JAVA_DOUBLE,128);
            wheelX=e.get(JAVA_DOUBLE,136); wheelY=e.get(JAVA_DOUBLE,144);
            button=e.get(JAVA_INT,152); clicks=e.get(JAVA_INT,156); modifiers=e.get(JAVA_INT,160); dropped=e.get(JAVA_INT,164)!=0;
        }
        public void preventDefault() { flags |= 1; }
        public void stopPropagation() { flags |= 2; }
        public void stopImmediatePropagation() { flags |= 6; }
    }
    private final class EventCallback {
        private final Consumer<UiEvent> handler;
        EventCallback(Consumer<UiEvent> handler) { this.handler=handler; }
        int event(MemorySegment user, MemorySegment pointer) {
            UiEvent event=new UiEvent(pointer.reinterpret(168));
            protect(() -> handler.accept(event));
            return event.flags;
        }
    }
    private MemorySegment eventStub(Consumer<UiEvent> handler) {
        ensureOpen();
        try {
            EventCallback callback=new EventCallback(Objects.requireNonNull(handler));
            MethodHandle method=MethodHandles.lookup().findVirtual(EventCallback.class,"event",
                MethodType.methodType(int.class,MemorySegment.class,MemorySegment.class)).bindTo(callback);
            MemorySegment result=Linker.nativeLinker().upcallStub(method,FunctionDescriptor.of(JAVA_INT,ADDRESS,ADDRESS),lifetime);
            callbacks.add(callback); return result;
        } catch (ReflectiveOperationException error) { throw new IllegalStateException(error); }
    }
    public void onEvent(String name, Consumer<UiEvent> handler) {
        MemorySegment function=eventStub(handler);
        try (Arena scope=Arena.ofConfined()) { status("gran_app_on_event",app,utf8(scope,name),function,MemorySegment.NULL); }
    }
    public long listen(String target, String type, Consumer<UiEvent> handler) { return listen(target,type,handler,false); }
    public long listen(String target, String type, Consumer<UiEvent> handler, boolean capture) {
        MemorySegment function=eventStub(handler);
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_LONG);
            status("gran_app_listen",app,utf8(scope,target),utf8(scope,type),capture ? 1 : 0,function,MemorySegment.NULL,output);
            return output.get(JAVA_LONG,0);
        }
    }
    public void unlisten(String target, long listener) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_unlisten",app,utf8(scope,target),listener); }
    }
    public boolean dispatch(String target, String type) { return dispatch(target,type,null); }
    public boolean dispatch(String target, String type, String detail) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_INT);
            status("gran_app_dispatch",app,utf8(scope,target),utf8(scope,type),utf8(scope,detail),output);
            return output.get(JAVA_INT,0) != 0;
        }
    }
    public void capturePointer(String target) { command("capturePointer",target,null); }
    public void releasePointer(String target) { command("releasePointer",target,null); }
    public void runCommand(String id) { command("runCommand",id,null); }
    public void setCommandEnabled(String id, boolean enabled) { command(enabled ? "enableCommand" : "disableCommand",id,null); }
    private synchronized MemorySegment taskEntry() {
        ensureOpen();
        if (taskEntry == null) {
            try {
                MethodHandle method=MethodHandles.lookup().findVirtual(Application.class,"runTask",
                    MethodType.methodType(void.class,MemorySegment.class)).bindTo(this);
                taskEntry=Linker.nativeLinker().upcallStub(method,FunctionDescriptor.ofVoid(ADDRESS),lifetime);
            } catch (ReflectiveOperationException error) { throw new IllegalStateException(error); }
        }
        return taskEntry;
    }
    private MemorySegment entry(String name, MethodType type, FunctionDescriptor descriptor, Object target) {
        try {
            MethodHandle method=MethodHandles.lookup().findVirtual(target.getClass(),name,type).bindTo(target);
            return Linker.nativeLinker().upcallStub(method,descriptor,lifetime);
        } catch (ReflectiveOperationException error) { throw new IllegalStateException(error); }
    }
    private synchronized void undoEntries() {
        ensureOpen();
        if (undoEntry != null) return;
        MethodType type=MethodType.methodType(void.class,MemorySegment.class);
        FunctionDescriptor descriptor=FunctionDescriptor.ofVoid(ADDRESS);
        undoEntry=entry("runUndo",type,descriptor,this);
        redoEntry=entry("runRedo",type,descriptor,this);
        releaseEntry=entry("releaseUndo",type,descriptor,this);
    }
    private void runUndo(MemorySegment user) { UndoPair pair=undoSteps.get(user.address()); if (pair != null) protect(pair.undo()); }
    private void runRedo(MemorySegment user) { UndoPair pair=undoSteps.get(user.address()); if (pair != null) protect(pair.redo()); }
    private void releaseUndo(MemorySegment user) { undoSteps.remove(user.address()); }
    // Записать выполненное действие парой обработчиков (стек null — основной) / Record a done action as a pair of handlers (the stack null is the main one)
    public void pushUndo(Runnable undo, Runnable redo) { pushUndo(undo,redo,null); }
    public void pushUndo(Runnable undo, Runnable redo, String stack) {
        undoEntries();
        long key=nextTask.incrementAndGet();
        undoSteps.put(key,new UndoPair(Objects.requireNonNull(undo),Objects.requireNonNull(redo)));
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment step=scope.allocate(40,8);
            step.set(JAVA_INT,0,40); step.set(ADDRESS,8,undoEntry); step.set(ADDRESS,16,redoEntry);
            step.set(ADDRESS,24,releaseEntry); step.set(ADDRESS,32,MemorySegment.ofAddress(key));
            status("gran_app_undo_push",app,utf8(scope,stack),step);
        } catch (RuntimeException | Error failure) { undoSteps.remove(key); throw failure; }
    }
    public void undo() { undo(null); }
    public void undo(String stack) { command("undo",stack,null); }
    public void redo() { redo(null); }
    public void redo(String stack) { command("redo",stack,null); }
    public UndoState undoState() { return undoState(null); }
    public UndoState undoState(String stack) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_INT);
            status("gran_app_undo_state",app,utf8(scope,stack),output);
            return UndoState.of(output.get(JAVA_INT,0));
        }
    }
    // Текущая позиция — «документ сохранён» / The current position is "the document is saved"
    public void setUndoClean(String stack) { command("setUndoClean",stack,null); }
    public void clearUndo(String stack) { command("clearUndo",stack,null); }
    public void setUndoLimit(int limit, String stack) {
        if (limit < 0) throw new IllegalArgumentException("limit");
        command("setUndoLimit",stack,Integer.toString(limit));
    }
    private final class UndoCallback {
        private final UndoListener listener;
        UndoCallback(UndoListener listener) { this.listener=listener; }
        void changed(MemorySegment user, MemorySegment stack, int flags) {
            protect(() -> listener.changed(text(stack),UndoState.of(flags)));
        }
    }
    // Обработчик смены любого стека; null снимает / A handler of any stack change; null removes it
    public void onUndoChanged(UndoListener listener) {
        MemorySegment function=MemorySegment.NULL;
        if (listener != null) {
            UndoCallback callback=new UndoCallback(listener);
            function=entry("changed",MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class,int.class),
                FunctionDescriptor.ofVoid(ADDRESS,ADDRESS,JAVA_INT),callback);
            callbacks.add(callback);
        }
        status("gran_app_on_undo",app,function,MemorySegment.NULL);
    }
    // Переменные $имя выражений {…}: текст сохраняет тип переменной, число и boolean задают его / The $name variables of the {…} expressions: a text keeps the variable type, a number and a boolean set it
    public void setVariable(String name, String value) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_variable_set_text",app,utf8(scope,name),utf8(scope,value)); }
    }
    public void setVariable(String name, double value) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_variable_set_number",app,utf8(scope,name),value); }
    }
    public void setVariable(String name, boolean value) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_variable_set_boolean",app,utf8(scope,name),value ? 1 : 0); }
    }
    // Значение переменной текстом; null — переменной нет / The variable value as text; null — no such variable
    public String variable(String name) {
        ensureOpen();
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(ADDRESS);
            status("gran_app_variable_read",app,utf8(scope,name),output);
            MemorySegment value=output.get(ADDRESS,0);
            if (value.address() == 0) return null;
            try { return text(value); }
            finally { invoke("gran_string_free", null, value); }
        }
    }
    private final class ConvertCallback {
        private final java.util.function.UnaryOperator<String> function;
        ConvertCallback(java.util.function.UnaryOperator<String> function) { this.function=function; }
        void convert(MemorySegment user, MemorySegment value, MemorySegment result) {
            protect(() -> {
                try (Arena scope=Arena.ofConfined()) { invoke("gran_converted_set",null,result,utf8(scope,function.apply(text(value)))); }
            });
        }
    }
    private MemorySegment converterEntry(java.util.function.UnaryOperator<String> function) {
        if (function == null) return MemorySegment.NULL;
        ConvertCallback callback=new ConvertCallback(function);
        callbacks.add(callback);
        return entry("convert",MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class,MemorySegment.class),
            FunctionDescriptor.ofVoid(ADDRESS,ADDRESS,ADDRESS),callback);
    }
    // Преобразователь «{выражение ; name}»; toSource == null — привязка односторонняя / The "{expression ; name}" converter; toSource == null — the binding is one-way
    public void setConverter(String name, java.util.function.UnaryOperator<String> toView, java.util.function.UnaryOperator<String> toSource) {
        try (Arena scope=Arena.ofConfined()) {
            status("gran_app_converter",app,utf8(scope,name),converterEntry(toView),converterEntry(toSource),MemorySegment.NULL);
        }
    }
    // Пересчитать выражения сейчас, не дожидаясь кадра / Recompute the expressions now without waiting for a frame
    public void refreshBindings() { status("gran_app_refresh_bindings",app); }
    private long addTask(Runnable callback, boolean repeat) {
        long key=nextTask.incrementAndGet();
        tasks.put(key,new Pending(Objects.requireNonNull(callback),repeat)); return key;
    }
    private void dropTask(long key) {
        tasks.remove(key);
        Long timer=keyTimers.remove(key);
        if (timer != null) timerKeys.remove(timer);
    }
    private void runTask(MemorySegment user) {
        long key=user.address();
        Pending entry=tasks.get(key);
        if (entry == null) return;
        if (!entry.repeat()) dropTask(key);
        protect(entry.callback());
    }
    public void post(Runnable callback) {
        long key=addTask(callback,false);
        try { status("gran_app_post",app,taskEntry(),MemorySegment.ofAddress(key)); }
        catch (RuntimeException | Error failure) { dropTask(key); throw failure; }
    }
    public long timer(double seconds, Runnable callback) { return timer(seconds,callback,false); }
    public long timer(double seconds, Runnable callback, boolean repeat) {
        long key=addTask(callback,repeat);
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment timer=scope.allocate(32,8), output=scope.allocate(JAVA_LONG);
            timer.set(JAVA_DOUBLE,0,seconds); timer.set(JAVA_INT,8,repeat ? 1 : 0);
            timer.set(ADDRESS,16,taskEntry()); timer.set(ADDRESS,24,MemorySegment.ofAddress(key));
            status("gran_app_timer",app,timer,output);
            long id=output.get(JAVA_LONG,0);
            timerKeys.put(id,key); keyTimers.put(key,id); return id;
        } catch (RuntimeException | Error failure) { dropTask(key); throw failure; }
    }
    public void clearTimer(long timer) {
        status("gran_app_clear_timer",app,timer);
        Long key=timerKeys.get(timer);
        if (key != null) dropTask(key);
    }
    // Число задач и таймеров, ждущих вызова (для проверок) / The number of tasks and timers waiting for a call (for checks)
    public int pendingTasks() { return tasks.size(); }
    public long modelCount(String model) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_LONG);
            status("gran_model_count",app,utf8(scope,model),output); return output.get(JAVA_LONG,0);
        }
    }
    public String modelRead(String model, long index, String field) {
        try (Arena scope=Arena.ofConfined()) { return ownedString("gran_model_read",app,utf8(scope,model),index,utf8(scope,field)); }
    }
    private void record(boolean insert, String model, long index, Map<String,String> fields) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment rows=fields.isEmpty() ? MemorySegment.NULL : scope.allocate(Math.multiplyExact(16L,fields.size()),8);
            long offset=0;
            for (var field:fields.entrySet()) {
                rows.set(ADDRESS,offset,utf8(scope,field.getKey())); rows.set(ADDRESS,offset+8,utf8(scope,field.getValue())); offset+=16;
            }
            MemorySegment record=scope.allocate(24,8);
            record.set(JAVA_LONG,0,index); record.set(ADDRESS,8,rows); record.set(JAVA_LONG,16,(long)fields.size());
            status(insert ? "gran_model_insert" : "gran_model_set",app,utf8(scope,model),record);
        }
    }
    public void modelInsert(String model, Map<String,String> fields) { record(true,model,-1L,fields); }
    public void modelInsert(String model, long index, Map<String,String> fields) { record(true,model,index,fields); }
    public void modelSet(String model, long index, Map<String,String> fields) { record(false,model,index,fields); }
    public void modelRemove(String model, long index) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_remove",app,utf8(scope,model),index); }
    }
    public void modelClear(String model) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_clear",app,utf8(scope,model)); }
    }
    // ---- Окна и программа 1.8: владельцы и модальные окна ----
    // ---- Windows and the program 1.8: owners and modal windows ----
    // Обработчики закрытия окон: user — номер обработчика, один переходник на все окна / The window close handlers: user is the handler number, one trampoline for all the windows
    private final ConcurrentMap<Long,Consumer<String>> windowHandlers = new ConcurrentHashMap<>();
    private MemorySegment windowEntry;
    private void windowClosed(MemorySegment user, MemorySegment result) {
        Consumer<String> handler=windowHandlers.remove(user.address());
        if (handler == null) return;
        String value=text(result);
        protect(() -> handler.accept(value));
    }
    private synchronized MemorySegment windowEntry() {
        ensureOpen();
        if (windowEntry == null)
            windowEntry=entry("windowClosed",MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class),
                FunctionDescriptor.ofVoid(ADDRESS,ADDRESS),this);
        return windowEntry;
    }
    // Ещё одно окно; owner — id узла окна-владельца (null — без владельца), onClosed(result) — после закрытия
    // One more window; owner is the id of an owner window node (null — without an owner), onClosed(result) — after closing
    public void openWindow(String source, boolean isFile, String owner, boolean modal, Consumer<String> onClosed) {
        long key=0;
        if (onClosed != null) { key=nextTask.incrementAndGet(); windowHandlers.put(key,onClosed); }
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment options=scope.allocate(40,8);
            options.set(JAVA_INT,0,40); options.set(ADDRESS,8,utf8(scope,owner)); options.set(JAVA_INT,16,modal ? 1 : 0);
            options.set(ADDRESS,24,onClosed != null ? windowEntry() : MemorySegment.NULL);
            options.set(ADDRESS,32,MemorySegment.ofAddress(key));
            status("gran_app_open_window",app,utf8(scope,source),isFile ? 1 : 0,options);
        } catch (RuntimeException | Error failure) { windowHandlers.remove(key); throw failure; }
    }
    public void openWindow(String path) { openWindow(path,true,null,false,null); }
    // Закрыть окно узла target с результатом для onClosed (null — "") / Close the window of the target node with a result for onClosed (null — "")
    public void closeWindow(String target, String result) {
        try (Arena scope=Arena.ofConfined()) { status("gran_app_close_window",app,utf8(scope,target),utf8(scope,result)); }
    }
    // ---- Журнал и отчёт о сбое 1.8 (на весь процесс) ----
    // ---- The log and the crash report 1.8 (process-wide) ----
    public static final int LOG_SEVERE=0, LOG_WARNING=1, LOG_INFO=2, LOG_VERBOSE=3;
    // Журнал процесса в файл и перехват падения (directory null — каталог по умолчанию) / The process log file and the crash catching (directory null — the default directory)
    public void openLog(String appName, String directory, boolean crashReports) {
        try (Arena scope=Arena.ofConfined()) {
            check((int)invoke("gran_log_open",JAVA_INT,utf8(scope,appName),utf8(scope,directory),crashReports ? 1 : 0));
        }
    }
    public void log(int level, String category, String text) {
        try (Arena scope=Arena.ofConfined()) {
            check((int)invoke("gran_log_write",JAVA_INT,level,utf8(scope,category),utf8(scope,text)));
        }
    }
    private String processString(String name) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(ADDRESS);
            check((int)invoke(name,JAVA_INT,output));
            MemorySegment value=output.get(ADDRESS,0);
            try { return text(value); }
            finally { invoke("gran_string_free",null,value); }
        }
    }
    public String logPath() { return processString("gran_log_path"); }
    // Отчёт о сбое прошлого запуска ("" — нет) / The previous run crash report ("" — none)
    public String previousCrash() { return processString("gran_log_previous_crash"); }
    // ---- Фоновые задачи 1.8: work — в потоке пула, done и progress — в потоке UI ----
    // ---- Background tasks 1.8: work runs on a pool thread, done and progress — on the UI thread ----
    // Состояния GRAN_TASK_* / The GRAN_TASK_* statuses
    public static final int TASK_DONE=0, TASK_CANCELLED=1, TASK_FAILED=2;
    // Контекст фоновой работы: действителен только внутри неё / A background work context: valid only inside it
    public final class BackgroundTask {
        private final MemorySegment handle;
        private BackgroundTask(MemorySegment task) { handle=task; }
        public boolean cancelled() { return (int)invoke("gran_task_cancelled",JAVA_INT,handle) != 0; }
        public void progress(double fraction, String text) {
            try (Arena scope=Arena.ofConfined()) { invoke("gran_task_progress",JAVA_INT,handle,fraction,utf8(scope,text)); }
        }
    }
    public interface BackgroundWork { Object run(BackgroundTask task) throws Exception; }
    public interface TaskDone { void done(int status, Object result, String error); }
    public interface TaskProgress { void progress(double fraction, String text); }
    private static final class BackgroundEntry {
        final BackgroundWork work; final TaskDone done; final TaskProgress progress; volatile Object result;
        BackgroundEntry(BackgroundWork work, TaskDone done, TaskProgress progress) { this.work=work; this.done=done; this.progress=progress; }
    }
    // Фоновые задачи: user — номер записи, переходники общие / The background tasks: user is the entry number, the trampolines are shared
    private final ConcurrentMap<Long,BackgroundEntry> background = new ConcurrentHashMap<>();
    private MemorySegment workEntry, doneEntry, progressEntry;
    // Поток пула: исключение не пересекает FFM — оно становится сбоем задачи / A pool thread: an exception does not cross FFM — it becomes the task failure
    private void runWork(MemorySegment user, MemorySegment task) {
        BackgroundEntry entry=background.get(user.address());
        if (entry == null) return;
        try { entry.result=entry.work.run(new BackgroundTask(task)); }
        catch (Throwable error) {
            String message=error.getMessage() != null ? error.getMessage() : error.getClass().getSimpleName();
            try (Arena scope=Arena.ofConfined()) { invoke("gran_task_fail",JAVA_INT,task,utf8(scope,message)); }
        }
    }
    private void taskDone(MemorySegment user, long task, int status, MemorySegment error) {
        BackgroundEntry entry=background.remove(user.address());
        if (entry == null || entry.done == null) return;
        Object result=status == TASK_DONE ? entry.result : null;
        String text=text(error);
        protect(() -> entry.done.done(status,result,text));
    }
    private void taskProgress(MemorySegment user, long task, double fraction, MemorySegment caption) {
        BackgroundEntry entry=background.get(user.address());
        if (entry == null || entry.progress == null) return;
        String text=text(caption);
        protect(() -> entry.progress.progress(fraction,text));
    }
    private synchronized void taskEntries() {
        ensureOpen();
        if (workEntry != null) return;
        workEntry=entry("runWork",MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class),
            FunctionDescriptor.ofVoid(ADDRESS,ADDRESS),this);
        doneEntry=entry("taskDone",MethodType.methodType(void.class,MemorySegment.class,long.class,int.class,MemorySegment.class),
            FunctionDescriptor.ofVoid(ADDRESS,JAVA_LONG,JAVA_INT,ADDRESS),this);
        progressEntry=entry("taskProgress",MethodType.methodType(void.class,MemorySegment.class,long.class,double.class,MemorySegment.class),
            FunctionDescriptor.ofVoid(ADDRESS,JAVA_LONG,JAVA_DOUBLE,ADDRESS),this);
    }
    public long runTask(BackgroundWork work, TaskDone done, TaskProgress progress) {
        Objects.requireNonNull(work);
        taskEntries();
        long key=nextTask.incrementAndGet();
        background.put(key,new BackgroundEntry(work,done,progress));
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment task=scope.allocate(40,8), output=scope.allocate(JAVA_LONG);
            task.set(JAVA_INT,0,40); task.set(ADDRESS,8,workEntry); task.set(ADDRESS,16,doneEntry);
            task.set(ADDRESS,24,progressEntry); task.set(ADDRESS,32,MemorySegment.ofAddress(key));
            status("gran_app_run_task",app,task,output);
            return output.get(JAVA_LONG,0);
        } catch (RuntimeException | Error failure) { background.remove(key); throw failure; }
    }
    // Попросить отменить; false — задачи уже нет / Ask to cancel; false — the task is gone
    public boolean cancelTask(long task) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment found=scope.allocate(JAVA_INT);
            status("gran_app_cancel_task",app,task,found);
            return found.get(JAVA_INT,0) != 0;
        }
    }
    // ---- Модели 1.7: таблицы, ленивая подгрузка, деревья, представления, выделение ----
    // ---- Models 1.7: tables, lazy loading, trees, views, selection ----
    // Строка плоского дерева: узел, глубина и признаки / A flat tree row: the node, the depth and the flags
    public record TreeRow(long node, long depth, boolean expanded, boolean hasChildren) {}
    // Текущая запись (-1 — нет) и отмеченные по возрастанию / The current record (-1 — none) and the marked ones ascending
    public record SelectionState(long current, long[] selected) {}
    // Обработчик с именем модели / A handler with the model name
    private final class NameCallback {
        private final Consumer<String> function;
        NameCallback(Consumer<String> function) { this.function=function; }
        void call(MemorySegment user, MemorySegment name) { protect(() -> function.accept(text(name))); }
    }
    // Подгрузчик детей ленивого узла: имя дерева и номер узла / A lazy node children fetcher: the tree name and the node number
    public interface TreeFetcher { void fetch(String tree, long node); }
    private final class TreeCallback {
        private final TreeFetcher function;
        TreeCallback(TreeFetcher function) { this.function=function; }
        void call(MemorySegment user, MemorySegment name, long node) { protect(() -> function.fetch(text(name),node)); }
    }
    private MemorySegment nameEntry(Consumer<String> function) {
        if (function == null) return MemorySegment.NULL;
        NameCallback callback=new NameCallback(function);
        callbacks.add(callback);
        return entry("call",MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class),
            FunctionDescriptor.ofVoid(ADDRESS,ADDRESS),callback);
    }
    // gran_record (index, fields, count) в памяти scope / gran_record (index, fields, count) in the scope memory
    private static MemorySegment recordOf(Arena scope, long index, Map<String,String> fields) {
        MemorySegment rows=fields.isEmpty() ? MemorySegment.NULL : scope.allocate(Math.multiplyExact(16L,fields.size()),8);
        long offset=0;
        for (var field:fields.entrySet()) {
            rows.set(ADDRESS,offset,utf8(scope,field.getKey())); rows.set(ADDRESS,offset+8,utf8(scope,field.getValue())); offset+=16;
        }
        MemorySegment record=scope.allocate(24,8);
        record.set(JAVA_LONG,0,index); record.set(ADDRESS,8,rows); record.set(JAVA_LONG,16,(long)fields.size());
        return record;
    }
    // gran_source: path, select, children, fetch и поля (поле → путь) / gran_source: path, select, children, fetch and the fields (a field → a path)
    private static MemorySegment sourceOf(Arena scope, String path, String select, String children, long fetch, Map<String,String> fields) {
        MemorySegment record=recordOf(scope,0,fields), source=scope.allocate(56,8);
        source.set(JAVA_INT,0,56); source.set(JAVA_INT,4,0); source.set(ADDRESS,8,utf8(scope,path));
        source.set(ADDRESS,16,utf8(scope,select)); source.set(ADDRESS,24,utf8(scope,children)); source.set(JAVA_LONG,32,fetch);
        source.set(ADDRESS,40,record.get(ADDRESS,8)); source.set(JAVA_LONG,48,(long)fields.size());
        return source;
    }
    public void modelMove(String model, long from, long count, long to) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_move",app,utf8(scope,model),from,count,to); }
    }
    // Столбцы: имя, заголовок, тип 0 text, 1 number, 2 date, 3 boolean / The columns: the name, the title, the type 0 text, 1 number, 2 date, 3 boolean
    public void modelColumns(String model, List<String[]> columns, int[] types) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment rows=columns.isEmpty() ? MemorySegment.NULL : scope.allocate(Math.multiplyExact(24L,columns.size()),8);
            for (int index=0; index<columns.size(); ++index) {
                rows.set(ADDRESS,24L*index,utf8(scope,columns.get(index)[0])); rows.set(ADDRESS,24L*index+8,utf8(scope,columns.get(index)[1]));
                rows.set(JAVA_INT,24L*index+16,types[index]);
            }
            status("gran_model_columns",app,utf8(scope,model),rows,(long)columns.size());
        }
    }
    public void loadData(String model, String path, String select, long fetch, Map<String,String> fields) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_load",app,utf8(scope,model),sourceOf(scope,path,select,"",fetch,fields)); }
    }
    public void loadTree(String tree, String path, String select, String children, Map<String,String> fields) {
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_load",app,utf8(scope,tree),sourceOf(scope,path,select,children,0,fields)); }
    }
    // callback(модель) вставляет следующую порцию; конец — setMore(model, false); null снимает / callback(model) inserts the next portion; the end — setMore(model, false); null drops it
    public void onFetch(String model, Consumer<String> callback) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_on_fetch",app,utf8(scope,model),nameEntry(callback),MemorySegment.NULL); }
    }
    public void setMore(String model, boolean more) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_set_more",app,utf8(scope,model),more ? 1 : 0); }
    }
    public void fetchMore(String model) {
        try (Arena scope=Arena.ofConfined()) { status("gran_model_fetch_more",app,utf8(scope,model)); }
    }
    // Узлы дерева — стабильные номера, 0 — верхний уровень / The tree nodes are stable numbers, 0 is the top level
    public long treeInsert(String tree, long parent, Map<String,String> fields) { return treeInsert(tree,parent,-1L,fields); }
    public long treeInsert(String tree, long parent, long index, Map<String,String> fields) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_LONG);
            status("gran_tree_insert",app,utf8(scope,tree),parent,recordOf(scope,index,fields),output); return output.get(JAVA_LONG,0);
        }
    }
    public void treeRemove(String tree, long node) {
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_remove",app,utf8(scope,tree),node); }
    }
    public void treeMove(String tree, long node, long parent, long row) {
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_move",app,utf8(scope,tree),node,parent,row); }
    }
    public void treeSet(String tree, long node, Map<String,String> fields) {
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_set",app,utf8(scope,tree),node,recordOf(scope,0,fields)); }
    }
    public String treeRead(String tree, long node, String field) {
        try (Arena scope=Arena.ofConfined()) { return ownedString("gran_tree_read",app,utf8(scope,tree),node,utf8(scope,field)); }
    }
    public long[] treeChildren(String tree, long node) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_LONG);
            status("gran_tree_children",app,utf8(scope,tree),node,output);
            long[] result=new long[(int)output.get(JAVA_LONG,0)];
            for (int row=0; row<result.length; ++row) {
                status("gran_tree_child",app,utf8(scope,tree),node,(long)row,output); result[row]=output.get(JAVA_LONG,0);
            }
            return result;
        }
    }
    public long treeParent(String tree, long node) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_LONG);
            status("gran_tree_parent",app,utf8(scope,tree),node,output); return output.get(JAVA_LONG,0);
        }
    }
    public void treeSetLazy(String tree, long node, boolean lazy) {
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_set_lazy",app,utf8(scope,tree),node,lazy ? 1 : 0); }
    }
    public void onTreeFetch(String tree, TreeFetcher callback) {
        TreeCallback holder=new TreeCallback(Objects.requireNonNull(callback));
        callbacks.add(holder);
        MemorySegment stub=entry("call",MethodType.methodType(void.class,MemorySegment.class,MemorySegment.class,long.class),
            FunctionDescriptor.ofVoid(ADDRESS,ADDRESS,JAVA_LONG),holder);
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_on_fetch",app,utf8(scope,tree),stub,MemorySegment.NULL); }
    }
    // gran_view: 1 прокси, 2 плоское дерево, 3 выделение (mode 0..3) / gran_view: 1 a proxy, 2 a flat tree, 3 a selection (mode 0..3)
    private void view(int kind, String name, String source, String sort, String filter, int mode, long expanded) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment view=scope.allocate(56,8);
            view.set(JAVA_INT,0,56); view.set(JAVA_INT,4,kind); view.set(ADDRESS,8,utf8(scope,name)); view.set(ADDRESS,16,utf8(scope,source));
            view.set(ADDRESS,24,utf8(scope,sort)); view.set(ADDRESS,32,utf8(scope,filter)); view.set(JAVA_INT,40,mode); view.set(JAVA_LONG,48,expanded);
            status("gran_view_create",app,view);
        }
    }
    public void createProxy(String name, String source, String sort, String filter) { view(1,name,source,sort,filter,3,0); }
    public void createTreeList(String name, String tree, String sort, String filter, long expanded) { view(2,name,tree,sort,filter,3,expanded); }
    // mode: 0 none, 1 single, 2 multi, 3 extended
    public void createSelection(String name, String source, int mode) { view(3,name,source,null,null,mode,0); }
    public void setSort(String view, String keys) {
        try (Arena scope=Arena.ofConfined()) { status("gran_view_sort",app,utf8(scope,view),utf8(scope,keys)); }
    }
    public void setFilter(String view, String expression) {
        try (Arena scope=Arena.ofConfined()) { status("gran_view_filter",app,utf8(scope,view),utf8(scope,expression)); }
    }
    public long sourceIndex(String view, long row) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_LONG);
            status("gran_view_source_index",app,utf8(scope,view),row,output); return output.get(JAVA_LONG,0);
        }
    }
    public TreeRow treeRow(String view, long row) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(24,8);
            status("gran_tree_row_read",app,utf8(scope,view),row,output);
            return new TreeRow(output.get(JAVA_LONG,0),output.get(JAVA_LONG,8),output.get(JAVA_INT,16) != 0,output.get(JAVA_INT,20) != 0);
        }
    }
    // action: 1 раскрыть, 0 свернуть, -1 переключить / action: 1 expand, 0 collapse, -1 toggle
    public void treeExpand(String view, long row, int action) {
        try (Arena scope=Arena.ofConfined()) { status("gran_tree_expand",app,utf8(scope,view),row,action); }
    }
    // command: 1 select, 2 deselect, 3 toggle, 4 range, 5 clear, 6 all, 7 current, 8 click (last — 1 Shift, 2 Ctrl)
    public void selectionChange(String selection, int command, long index, long last) {
        try (Arena scope=Arena.ofConfined()) { status("gran_selection_change",app,utf8(scope,selection),command,index,last); }
    }
    public SelectionState selection(String selection) {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment current=scope.allocate(JAVA_LONG), count=scope.allocate(JAVA_LONG);
            count.set(JAVA_LONG,0,0L);
            status("gran_selection_read",app,utf8(scope,selection),current,MemorySegment.NULL,count);
            long[] result=new long[(int)count.get(JAVA_LONG,0)];
            MemorySegment indices=scope.allocate(JAVA_LONG,Math.max(1,result.length));
            status("gran_selection_read",app,utf8(scope,selection),current,indices,count);
            for (int index=0; index<result.length; ++index) result[index]=indices.getAtIndex(JAVA_LONG,index);
            return new SelectionState(current.get(JAVA_LONG,0),result);
        }
    }
    public void onSelectionChanged(String selection, Consumer<String> callback) {
        try (Arena scope=Arena.ofConfined()) { status("gran_selection_on_change",app,utf8(scope,selection),nameEntry(callback),MemorySegment.NULL); }
    }
    public int run() {
        try (Arena scope=Arena.ofConfined()) {
            MemorySegment output=scope.allocate(JAVA_INT);
            status("gran_app_run",app,output); return output.get(JAVA_INT,0);
        } finally {
            Throwable error=pending.getAndSet(null);
            if (error instanceof RuntimeException runtime) throw runtime;
            if (error instanceof Error fatal) throw fatal;
            if (error != null) throw new IllegalStateException(error);
        }
    }
    public void close() {
        if (app.address() == 0) return;
        status("gran_app_destroy",app);
        app=MemorySegment.NULL; callbacks.clear(); tasks.clear(); timerKeys.clear(); keyTimers.clear(); undoSteps.clear(); lifetime.close();
    }
}
