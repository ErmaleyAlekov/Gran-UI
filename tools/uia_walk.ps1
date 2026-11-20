# Дерево UI Automation окна программы (как в Inspect, «сырой вид»): тип, имя, элемент управления ли, AutomationId.
# The UI Automation tree of a program window (as in Inspect, the "raw view"): the type, the name, whether a control, AutomationId.
# Запуск / Run: powershell -ExecutionPolicy Bypass -File tools/uia_walk.ps1 [-Process gran_gallery] [-Depth 7] [-Limit 60]
param([string]$Process = "gran_gallery", [int]$Depth = 7, [int]$Limit = 60)
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$proc = (Get-Process $Process | Select-Object -First 1).Id
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ProcessIdProperty, $proc)
$win = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
if ($win -eq $null) { "window not found"; exit 1 }
$walker = [System.Windows.Automation.TreeWalker]::RawViewWalker
$script:count = 0
function Show-Node($el, $level) {
    if ($script:count -ge $Limit -or $level -gt $Depth) { return }
    $script:count++
    $c = $el.Current
    $pad = "  " * $level
    $type = $c.ControlType.ProgrammaticName
    $name = $c.Name
    if ($name.Length -gt 40) { $name = $name.Substring(0, 40) }
    "$pad$type '$name' ctl=$($c.IsControlElement) id=$($c.AutomationId)"
    $child = $walker.GetFirstChild($el)
    while ($child -ne $null -and $script:count -lt $Limit) {
        Show-Node $child ($level + 1)
        $child = $walker.GetNextSibling($child)
    }
}
Show-Node $win 0
