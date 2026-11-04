# ============================================================================
#  Замер памяти процесса и видеопамяти раз в N секунд (задача Т-6, долгая сессия)
#  Sampling process memory and video memory every N seconds (task T-6, long session)
#
#  Запуск / Usage:
#    powershell -ExecutionPolicy Bypass -File tests/manual/tools/memory_watch.ps1 -Name gran_gallery -Seconds 60
#  Результат — CSV в текущей папке: время, частная память, рабочий набор, выделенная и общая видеопамять (МБ).
#  The result is a CSV in the current folder: time, private memory, working set, dedicated and shared video memory (MB).
# ============================================================================
param(
    [string]$Name = "gran_gallery",
    [int]$Seconds = 60,
    [string]$Output = "memory_watch.csv"
)

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

"time,private_mb,working_set_mb,gpu_dedicated_mb,gpu_shared_mb" | Out-File -Encoding utf8 $Output
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
    # Инвариантная культура: в русской локали дробная часть пишется через запятую и ломает CSV.
    # The invariant culture: in the Russian locale the fraction uses a comma and breaks the CSV.
    $line = [string]::Format([Globalization.CultureInfo]::InvariantCulture, "{0},{1},{2},{3},{4}",
        (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $private, $working, $dedicated, $shared)
    $line | Out-File -Encoding utf8 -Append $Output
    Write-Host $line
    Start-Sleep -Seconds $Seconds
}
