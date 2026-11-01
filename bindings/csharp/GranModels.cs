using System.Runtime.InteropServices;

namespace GranUI;

// Режим выделения (GRAN_SELECT_*) / The selection mode (GRAN_SELECT_*)
public enum SelectionMode { None = 0, Single = 1, Multi = 2, Extended = 3 }

// Тип столбца таблицы (GRAN_COLUMN_*) / The table column type (GRAN_COLUMN_*)
public enum ColumnType { Text = 0, Number = 1, Date = 2, Boolean = 3 }

// Команда модели выделения (GRAN_SELECTION_*) / A selection model command (GRAN_SELECTION_*)
public enum SelectionCommand { Select = 1, Deselect = 2, Toggle = 3, Range = 4, Clear = 5, All = 6, Current = 7, Click = 8 }

// Столбец таблицы / A table column
public readonly record struct Column(string Name, string Title, ColumnType Type);

// Строка плоского дерева: узел, глубина и признаки / A flat tree row: the node, the depth and the flags
public readonly record struct TreeRow(ulong Node, nuint Depth, bool Expanded, bool HasChildren);

// Объявления функций моделей 1.7 общего C ABI / The declarations of the models 1.7 functions of the shared C ABI
internal static class NativeModels
{
    const string Library = "gran_c";
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void FetchFn(IntPtr user, IntPtr model);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] internal delegate void TreeFetchFn(IntPtr user, IntPtr tree, ulong node);
    [StructLayout(LayoutKind.Sequential)] internal struct ColumnData { internal IntPtr Name, Title; internal int Type; }
    [StructLayout(LayoutKind.Sequential)] internal struct Source {
        internal uint Size; internal int Format; internal IntPtr Path, Select, Children; internal nuint Fetch;
        internal IntPtr Fields; internal nuint FieldCount;
    }
    [StructLayout(LayoutKind.Sequential)] internal struct View {
        internal uint Size; internal int Kind; internal IntPtr Name, Source, Sort, Filter; internal int Mode; internal nuint Expanded;
    }
    [StructLayout(LayoutKind.Sequential)] internal struct Row { internal ulong Node; internal nuint Depth; internal int Expanded, HasChildren; }
    [DllImport(Library)] internal static extern int gran_model_move(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, nuint from, nuint count, nuint to);
    [DllImport(Library)] internal static extern int gran_model_columns(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, IntPtr columns, nuint count);
    [DllImport(Library)] internal static extern int gran_model_load(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, ref Source source);
    [DllImport(Library)] internal static extern int gran_model_on_fetch(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, FetchFn? callback, IntPtr user);
    [DllImport(Library)] internal static extern int gran_model_set_more(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model, int more);
    [DllImport(Library)] internal static extern int gran_model_fetch_more(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string model);
    [DllImport(Library)] internal static extern int gran_tree_insert(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong parent, ref Native.Record record, out ulong result);
    [DllImport(Library)] internal static extern int gran_tree_remove(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node);
    [DllImport(Library)] internal static extern int gran_tree_move(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, ulong parent, nuint row);
    [DllImport(Library)] internal static extern int gran_tree_set(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, ref Native.Record record);
    [DllImport(Library)] internal static extern int gran_tree_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, [MarshalAs(UnmanagedType.LPUTF8Str)] string field, out IntPtr result);
    [DllImport(Library)] internal static extern int gran_tree_children(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, out nuint result);
    [DllImport(Library)] internal static extern int gran_tree_child(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, nuint row, out ulong result);
    [DllImport(Library)] internal static extern int gran_tree_parent(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, out ulong result);
    [DllImport(Library)] internal static extern int gran_tree_set_lazy(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ulong node, int lazy);
    [DllImport(Library)] internal static extern int gran_tree_on_fetch(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, TreeFetchFn? callback, IntPtr user);
    [DllImport(Library)] internal static extern int gran_tree_load(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string tree, ref Source source);
    [DllImport(Library)] internal static extern int gran_view_create(IntPtr app, ref View view);
    [DllImport(Library)] internal static extern int gran_view_sort(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string view, [MarshalAs(UnmanagedType.LPUTF8Str)] string keys);
    [DllImport(Library)] internal static extern int gran_view_filter(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string view, [MarshalAs(UnmanagedType.LPUTF8Str)] string? expression);
    [DllImport(Library)] internal static extern int gran_view_source_index(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string view, nuint row, out nuint result);
    [DllImport(Library)] internal static extern int gran_tree_row_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string view, nuint row, out Row result);
    [DllImport(Library)] internal static extern int gran_tree_expand(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string view, nuint row, int action);
    [DllImport(Library)] internal static extern int gran_selection_change(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string selection, int command, nuint index, nuint last);
    [DllImport(Library)] internal static extern int gran_selection_read(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string selection, out nuint current, IntPtr indices, ref nuint count);
    [DllImport(Library)] internal static extern int gran_selection_on_change(IntPtr app, [MarshalAs(UnmanagedType.LPUTF8Str)] string selection, FetchFn? callback, IntPtr user);
}

// Модели 1.7: таблицы, ленивая подгрузка, деревья, представления, выделение / Models 1.7: tables, lazy loading, trees, views, selection
public sealed partial class Application
{
    public void ModelMove(string model, nuint from, nuint count, nuint to) => Check(NativeModels.gran_model_move(Handle, Arguments.Text(model), from, count, to));
    public void ModelColumns(string model, IReadOnlyList<Column> columns) {
        using var args = new Arguments();
        var rows = columns.Select(column => new NativeModels.ColumnData { Name=args.Utf8(column.Name), Title=args.Utf8(column.Title), Type=(int)column.Type }).ToArray();
        Check(NativeModels.gran_model_columns(Handle, Arguments.Text(model), args.Array(rows), (nuint)rows.Length));
    }
    // Источник из файла: fields — поле → путь; fetch > 0 — ленивая подгрузка порциями / A source from a file: fields — a field → a path; fetch > 0 — lazy loading by portions
    static NativeModels.Source Source(Arguments args, string path, string select, string children, nuint fetch, IReadOnlyDictionary<string,string>? fields) {
        var rows = (fields ?? new Dictionary<string,string>()).Select(row => new Native.Field { Name=args.Utf8(row.Key), Value=args.Utf8(row.Value) }).ToArray();
        return new NativeModels.Source { Size=(uint)Marshal.SizeOf<NativeModels.Source>(), Path=args.Utf8(path), Select=args.Utf8(select),
            Children=args.Utf8(children), Fetch=fetch, Fields=args.Array(rows), FieldCount=(nuint)rows.Length };
    }
    public void LoadData(string model, string path, string select="", nuint fetch=0, IReadOnlyDictionary<string,string>? fields=null) {
        using var args = new Arguments();
        var source = Source(args, path, select, "", fetch, fields);
        Check(NativeModels.gran_model_load(Handle, Arguments.Text(model), ref source));
    }
    public void LoadTree(string tree, string path, string select="", string children="", IReadOnlyDictionary<string,string>? fields=null) {
        using var args = new Arguments();
        var source = Source(args, path, select, children, 0, fields);
        Check(NativeModels.gran_tree_load(Handle, Arguments.Text(tree), ref source));
    }
    // callback(модель) вставляет следующую порцию; конец записей — SetMore(model, false) / callback(model) inserts the next portion; the end — SetMore(model, false)
    public void OnFetch(string model, Action<string>? callback) {
        NativeModels.FetchFn? native = callback == null ? null : (_, name) => Protect(() => callback(String(name)));
        Check(NativeModels.gran_model_on_fetch(Handle, Arguments.Text(model), native, IntPtr.Zero));
        if (native != null) callbacks.Add(native);
    }
    public void SetMore(string model, bool more) => Check(NativeModels.gran_model_set_more(Handle, Arguments.Text(model), more ? 1 : 0));
    public void FetchMore(string model) => Check(NativeModels.gran_model_fetch_more(Handle, Arguments.Text(model)));
    // Узлы дерева — стабильные номера, 0 — верхний уровень / The tree nodes are stable numbers, 0 is the top level
    public ulong TreeInsert(string tree, ulong parent, IReadOnlyDictionary<string,string> fields) => TreeInsert(tree, parent, fields, nuint.MaxValue);
    public ulong TreeInsert(string tree, ulong parent, IReadOnlyDictionary<string,string> fields, nuint index) {
        using var args = new Arguments();
        var rows = fields.Select(row => new Native.Field { Name=args.Utf8(row.Key), Value=args.Utf8(row.Value) }).ToArray();
        var record = new Native.Record { Index=index, Fields=args.Array(rows), Count=(nuint)rows.Length };
        Check(NativeModels.gran_tree_insert(Handle, Arguments.Text(tree), parent, ref record, out var result)); return result;
    }
    public void TreeRemove(string tree, ulong node) => Check(NativeModels.gran_tree_remove(Handle, Arguments.Text(tree), node));
    public void TreeMove(string tree, ulong node, ulong parent, nuint row) => Check(NativeModels.gran_tree_move(Handle, Arguments.Text(tree), node, parent, row));
    public void TreeSet(string tree, ulong node, IReadOnlyDictionary<string,string> fields) {
        using var args = new Arguments();
        var rows = fields.Select(row => new Native.Field { Name=args.Utf8(row.Key), Value=args.Utf8(row.Value) }).ToArray();
        var record = new Native.Record { Fields=args.Array(rows), Count=(nuint)rows.Length };
        Check(NativeModels.gran_tree_set(Handle, Arguments.Text(tree), node, ref record));
    }
    public string TreeRead(string tree, ulong node, string field) {
        Check(NativeModels.gran_tree_read(Handle, Arguments.Text(tree), node, Arguments.Text(field), out var result)); return OwnedString(result);
    }
    public IReadOnlyList<ulong> TreeChildren(string tree, ulong node=0) {
        Check(NativeModels.gran_tree_children(Handle, Arguments.Text(tree), node, out var count));
        var result = new List<ulong>();
        for (nuint row = 0; row < count; ++row) {
            Check(NativeModels.gran_tree_child(Handle, Arguments.Text(tree), node, row, out var child)); result.Add(child);
        }
        return result;
    }
    public ulong TreeParent(string tree, ulong node) { Check(NativeModels.gran_tree_parent(Handle, Arguments.Text(tree), node, out var result)); return result; }
    public void TreeSetLazy(string tree, ulong node, bool lazy=true) => Check(NativeModels.gran_tree_set_lazy(Handle, Arguments.Text(tree), node, lazy ? 1 : 0));
    public void OnTreeFetch(string tree, Action<string, ulong> callback) {
        NativeModels.TreeFetchFn native = (_, name, node) => Protect(() => callback(String(name), node));
        Check(NativeModels.gran_tree_on_fetch(Handle, Arguments.Text(tree), native, IntPtr.Zero));
        callbacks.Add(native);
    }
    void View(int kind, string name, string source, string? sort, string? filter, SelectionMode mode, nuint expanded) {
        using var args = new Arguments();
        var view = new NativeModels.View { Size=(uint)Marshal.SizeOf<NativeModels.View>(), Kind=kind, Name=args.Utf8(name), Source=args.Utf8(source),
            Sort=sort == null ? IntPtr.Zero : args.Utf8(sort), Filter=filter == null ? IntPtr.Zero : args.Utf8(filter), Mode=(int)mode, Expanded=expanded };
        Check(NativeModels.gran_view_create(Handle, ref view));
    }
    public void CreateProxy(string name, string source, string? sort=null, string? filter=null) => View(1, name, source, sort, filter, SelectionMode.Extended, 0);
    public void CreateTreeList(string name, string tree, string? sort=null, string? filter=null, nuint expanded=0) => View(2, name, tree, sort, filter, SelectionMode.Extended, expanded);
    public void CreateSelection(string name, string source, SelectionMode mode=SelectionMode.Extended) => View(3, name, source, null, null, mode, 0);
    public void SetSort(string view, string keys) => Check(NativeModels.gran_view_sort(Handle, Arguments.Text(view), Arguments.Text(keys)));
    public void SetFilter(string view, string? expression) => Check(NativeModels.gran_view_filter(Handle, Arguments.Text(view), expression));
    public nuint SourceIndex(string view, nuint row) { Check(NativeModels.gran_view_source_index(Handle, Arguments.Text(view), row, out var result)); return result; }
    public TreeRow TreeRowOf(string view, nuint row) {
        Check(NativeModels.gran_tree_row_read(Handle, Arguments.Text(view), row, out var result));
        return new TreeRow(result.Node, result.Depth, result.Expanded != 0, result.HasChildren != 0);
    }
    // action: 1 раскрыть, 0 свернуть, -1 переключить / action: 1 expand, 0 collapse, -1 toggle
    public void TreeExpand(string view, nuint row, int action=-1) => Check(NativeModels.gran_tree_expand(Handle, Arguments.Text(view), row, action));
    public void SelectionChange(string selection, SelectionCommand command, nuint index=0, nuint last=0) =>
        Check(NativeModels.gran_selection_change(Handle, Arguments.Text(selection), (int)command, index, last));
    // Текущая запись (null — нет) и отмеченные по возрастанию / The current record (null — none) and the marked ones ascending
    public (nuint? Current, nuint[] Selected) Selection(string selection) {
        nuint count = 0;
        Check(NativeModels.gran_selection_read(Handle, Arguments.Text(selection), out var current, IntPtr.Zero, ref count));
        var result = new nuint[(int)count];
        var buffer = Marshal.AllocCoTaskMem(Math.Max(1, (int)count) * IntPtr.Size);
        try {
            Check(NativeModels.gran_selection_read(Handle, Arguments.Text(selection), out current, buffer, ref count));
            for (int index = 0; index < result.Length; ++index) result[index] = (nuint)(ulong)Marshal.ReadIntPtr(buffer, index * IntPtr.Size);
        }
        finally { Marshal.FreeCoTaskMem(buffer); }
        return (current == nuint.MaxValue ? null : current, result);
    }
    public void OnSelectionChanged(string selection, Action<string>? callback) {
        NativeModels.FetchFn? native = callback == null ? null : (_, name) => Protect(() => callback(String(name)));
        Check(NativeModels.gran_selection_on_change(Handle, Arguments.Text(selection), native, IntPtr.Zero));
        if (native != null) callbacks.Add(native);
    }
}
