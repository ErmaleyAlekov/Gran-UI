# Проверка строк виртуальной таблицы клиентом UI Automation: запомнить строки, выбрать строку, PageDown ×12, сколько старых
# элементов стали недоступны (ожидается 0: они отвечают «вне экрана») и сколько пришло событий StructureChanged (ожидается > 0).
# Окно — первое окно процесса gran_gallery с таблицей (samples/grid.xml), прокрученное в начало.
# Checking the virtual table rows by a UI Automation client: remember the rows, select a row, PageDown x12, how many old
# elements became unavailable (0 expected: they answer as offscreen) and how many StructureChanged events came (> 0 expected).
# Запуск / Run: powershell -ExecutionPolicy Bypass -File tools/uia_scroll_check.ps1
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -ReferencedAssemblies UIAutomationClient,UIAutomationTypes,WindowsBase @"
using System; using System.Runtime.InteropServices; using System.Windows.Automation; using System.Threading;
public static class W {
    [DllImport("user32")] public static extern bool PostMessageW(IntPtr h, uint m, IntPtr w, IntPtr l);
    public static int Count;
    public static string Last = "";
    public static int Rows;
    public static void OnChange(object sender, StructureChangedEventArgs e) {
        Interlocked.Increment(ref Count); Last = e.StructureChangeType.ToString();
        try { if (((AutomationElement)sender).Current.ControlType == ControlType.DataItem) Interlocked.Increment(ref Rows); } catch { }
    }
    public static StructureChangedEventHandler Handler = new StructureChangedEventHandler(OnChange);
}
"@
$A = [System.Windows.Automation.AutomationElement]
$proc = (Get-Process gran_gallery | Select-Object -First 1).Id
$win = $A::RootElement.FindFirst([System.Windows.Automation.TreeScope]::Children,
    (New-Object System.Windows.Automation.PropertyCondition($A::ProcessIdProperty, $proc)))
$grid = $win.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
    (New-Object System.Windows.Automation.PropertyCondition($A::ControlTypeProperty, [System.Windows.Automation.ControlType]::DataGrid)))
$walker = [System.Windows.Automation.TreeWalker]::ControlViewWalker
function Get-Rows {
    $list = @(); $c = $walker.GetFirstChild($grid)
    while ($c -ne $null) { if ($c.Current.ControlType -eq [System.Windows.Automation.ControlType]::DataItem) { $list += $c }; $c = $walker.GetNextSibling($c) }
    return $list
}
$script:events = 0
[System.Windows.Automation.Automation]::AddStructureChangedEventHandler($grid, [System.Windows.Automation.TreeScope]::Subtree, [W]::Handler)
$rows = Get-Rows
"rows: $(($rows | ForEach-Object { $_.Current.Name.Substring(0,4) }) -join ' ')"
"rows before: $($rows.Count), first: $($rows[0].Current.Name.Substring(0,4)), runtime id: $($rows[0].GetRuntimeId() -join '.')"
$rows[3].GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
Start-Sleep -Milliseconds 400
$hwnd = [IntPtr]$win.Current.NativeWindowHandle
for ($i = 0; $i -lt 12; $i++) { [W]::PostMessageW($hwnd, 0x100, [IntPtr]0x22, [IntPtr]0) | Out-Null; Start-Sleep -Milliseconds 300 }
Start-Sleep -Milliseconds 800
$dead = 0
$names = @()
$offscreen = 0
foreach ($r in $rows) {
    try {
        # RuntimeId собирает сам UI Automation (через корень фрагмента) — проверка всей цепочки / UI Automation builds RuntimeId itself (through the fragment root) — a check of the whole chain
        $null = $r.GetRuntimeId()
        $null = [System.Windows.Automation.TreeWalker]::RawViewWalker.GetParent($r)
        if ($r.Current.IsOffscreen) { $offscreen++; $names += 'OFF' } else { $names += $r.Current.Name.Substring(0,4) }
    } catch { $dead++; $names += 'DEAD' }
}
"old rows now: $($names -join ' ')"
"after PageDown x12: unavailable $dead, offscreen $offscreen of $($rows.Count); structure events: $([W]::Count) (on rows: $([W]::Rows)) last: $([W]::Last)"
$now = Get-Rows
"rows now: $($now.Count): $(($now | ForEach-Object { $_.Current.Name.Substring(0,4) }) -join ' ')"
[System.Windows.Automation.Automation]::RemoveAllEventHandlers()
