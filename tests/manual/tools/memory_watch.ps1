# ============================================================================
#  Замер памяти процесса и видеопамяти раз в N секунд (задача Т-6, долгая сессия)
#  Sampling process memory and video memory every N seconds (task T-6, long session)
#
#  Запуск / Usage:
#    powershell -ExecutionPolicy Bypass -File tests/manual/tools/memory_watch.ps1 -Name gran_gallery -Seconds 60
#  Результат — CSV в текущей папке: время, частная память, рабочий набор, выделенная и общая видеопамять (МБ),
#  размер клиентской области главного окна в пикселях (видеопамять зависит от площади окна — без размера ступени
#  на графике не объяснить; добавлено после Т-6, 2026-11-06).
#  The result is a CSV in the current folder: time, private memory, working set, dedicated and shared video memory (MB),
#  the main window client area size in pixels (the video memory depends on the window area — without the size the
#  steps on the chart cannot be explained; added after T-6, 2026-11-06).
# ============================================================================
param(
    [string]$Name = "gran_gallery",
    [int]$Seconds = 60,
    [string]$Output = "memory_watch.csv"
)

# Размер клиентской области окна. SetProcessDPIAware — чтобы Windows не пересчитывала размер окна программы,
# понимающей DPI, в «логические» пиксели этого скрипта.
# The window client area size. SetProcessDPIAware — so that Windows does not convert the size of a DPI-aware
# program window into the "logical" pixels of this script.
Add-Type -Namespace GranWatch -Name Native -MemberDefinition @'
[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
public struct Rect { public int Left; public int Top; public int Right; public int Bottom; }
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool GetClientRect(System.IntPtr window, out Rect rect);
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool SetProcessDPIAware();
'@
[void][GranWatch.Native]::SetProcessDPIAware()

# Ширина и высота клиентской области главного окна; нет окна (свёрнуто в трей, ещё не создано) — пусто.
# The width and the height of the main window client area; no window (hidden, not created yet) — empty.
function Get-WindowSize([System.Diagnostics.Process]$Process) {
    $rect = New-Object GranWatch.Native+Rect
    if ($Process.MainWindowHandle -eq [System.IntPtr]::Zero -or
        -not [GranWatch.Native]::GetClientRect($Process.MainWindowHandle, [ref]$rect)) {
        return @("", "")
    }
    return @(($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top))
}

# Сумма значений счётчика GPU для процесса (у процесса несколько адаптеров/движков — складываем).
# The sum of a GPU counter for the process (a process has several adapters/engines — they are added up).
function Get-GpuMemory([int]$ProcessId, [string]$Counter) {
    $path = "\GPU Process Memory(pid_${ProcessId}_*)\$Counter"
    try {
        $samples = (Get-Counter -Counter $path -ErrorAction Stop).CounterSamples
        return [math]::Round(($samples | Measure-Object -Property CookedValue -Sum).Sum / 1MB, 1)
    } catch {
        # Счётчика нет (старый драйвер или процесс ещё не создал устройство) — пустое значение.
        # No counter (an old driver or the process has not created a device yet) — an empty value.
        return ""
    }
}

"time,private_mb,working_set_mb,gpu_dedicated_mb,gpu_shared_mb,window_w,window_h" | Out-File -Encoding utf8 $Output
Write-Host "Writing $Output every $Seconds s; Ctrl+C to stop"
while ($true) {
    $process = Get-Process -Name $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $process) {
        Write-Host "Process $Name not found, stopping"
        break
    }
    $private = [math]::Round($process.PrivateMemorySize64 / 1MB, 1)
    $working = [math]::Round($process.WorkingSet64 / 1MB, 1)
    $dedicated = Get-GpuMemory $process.Id "Dedicated Usage"
    $shared = Get-GpuMemory $process.Id "Shared Usage"
    $size = Get-WindowSize $process
    # Инвариантная культура: в русской локали дробная часть пишется через запятую и ломает CSV.
    # The invariant culture: in the Russian locale the fraction uses a comma and breaks the CSV.
    $line = [string]::Format([Globalization.CultureInfo]::InvariantCulture, "{0},{1},{2},{3},{4},{5},{6}",
        (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $private, $working, $dedicated, $shared, $size[0], $size[1])
    $line | Out-File -Encoding utf8 -Append $Output
    Write-Host $line
    Start-Sleep -Seconds $Seconds
}
