# ============================================================================
#  Создание релиза Грани UI на Gitverse через публичный API (https://api.gitverse.ru).
#  Нужен, пока нет сборки выпуска по тегу (П.2), и когда веб-форма Gitverse отвечает
#  «Сервис временно недоступен»: API вернёт настоящую причину ошибки.
#  Тег уже должен быть на Gitverse. Токен — «Настройки → Токены» на gitverse.ru с правом
#  «Репозитории → Запись»; берётся из $env:GV_API_TOKEN или спрашивается скрытым вводом.
#
#  Запуск (PowerShell, из корня проекта):
#    powershell -ExecutionPolicy Bypass -File tools/release/gitverse_release.ps1 -Tag v0.1.0
#  Без описания (если Gitverse не принимает текст): добавить -NoNotes.
#
#  Creating a Gran UI release on Gitverse through the public API (https://api.gitverse.ru).
#  It is needed until there is a release build by tag (P.2), and when the Gitverse web form answers
#  "the service is temporarily unavailable": the API returns the real cause of the error.
#  The tag must already be on Gitverse. The token is "Settings → Tokens" on gitverse.ru with the
#  "Repositories → Write" right; it is taken from $env:GV_API_TOKEN or asked with hidden input.
#
#  Run (PowerShell, from the project root):
#    powershell -ExecutionPolicy Bypass -File tools/release/gitverse_release.ps1 -Tag v0.1.0
#  Without notes (if Gitverse does not accept the text): add -NoNotes.
# ============================================================================
param(
    [Parameter(Mandatory = $true)][string]$Tag,
    [string]$Repository = "ermaleyalekov/Gran-UI",
    [switch]$NoNotes
)

$ErrorActionPreference = "Stop"

# Токен: из переменной окружения или скрытым вводом, в журнал и на экран не выводится
# The token: from the environment variable or by hidden input, never printed or logged
function Get-GitverseToken {
    if ($env:GV_API_TOKEN) {
        return $env:GV_API_TOKEN
    }
    $secure = Read-Host "Токен Gitverse / Gitverse token" -AsSecureString
    $pointer = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
    try {
        return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pointer)
    }
    finally {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pointer)
    }
}

# Тело запроса: тег, название «Грань UI X.Y.Z», описание из tools/release/notes-<тег>.md (UTF-8)
# The request body: the tag, the "Грань UI X.Y.Z" title, the notes from tools/release/notes-<tag>.md (UTF-8)
function Get-ReleaseBody {
    $notes = ""
    if (-not $NoNotes) {
        $path = Join-Path $PSScriptRoot "notes-$Tag.md"
        $notes = [IO.File]::ReadAllText($path, [Text.Encoding]::UTF8)
    }
    $title = "Грань UI " + $Tag.TrimStart("v")
    $json = @{ tag_name = $Tag; name = $title; body = $notes } | ConvertTo-Json -Compress
    # Запятая: иначе PowerShell развернёт массив байтов в список объектов, и тело уйдёт пустым
    # The comma: otherwise PowerShell unrolls the byte array into a list of objects, and the body goes empty
    return ,[Text.Encoding]::UTF8.GetBytes($json)
}

# Ответ сервера при ошибке: код и текст, чтобы видеть настоящую причину
# The server answer on an error: the code and the text, to see the real cause
function Write-HttpError($failure) {
    $response = $failure.Exception.Response
    if ($null -eq $response) {
        Write-Host "Ошибка / Error: $($failure.Exception.Message)"
        return
    }
    # PowerShell 5.1 уже прочитал поток ответа — текст лежит в ErrorDetails
    # PowerShell 5.1 has already read the response stream — the text is in ErrorDetails
    Write-Host "HTTP $([int]$response.StatusCode): $($failure.ErrorDetails.Message)"
}

$headers = @{
    "Authorization" = "Bearer " + (Get-GitverseToken)
    "Accept"        = "application/vnd.gitverse.object+json;version=1"
}
try {
    $uri = "https://api.gitverse.ru/repos/$Repository/releases"
    $result = Invoke-RestMethod -Method Post -Uri $uri -Headers $headers -ContentType "application/json; charset=utf-8" -Body (Get-ReleaseBody)
    Write-Host "Релиз создан / The release is created: id $($result.id)"
}
catch {
    Write-HttpError $_
    exit 1
}
