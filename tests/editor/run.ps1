# Сборка редактора и его интеграционных проверок из корня репозитория.
# Build the editor and its integration checks from the repository root.
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '../..')
try {
    & make -j4 bin/gran_editor.exe
    if ($LASTEXITCODE -ne 0) { throw 'Editor build failed' }
    $objects = @(Get-ChildItem obj/examples/editor/core/*.o,obj/examples/editor/ui/*.o | ForEach-Object FullName)
    & g++ -std=c++17 -Iexamples/editor -Iinclude '-I../Dom lib C++/include' tests/editor/editor_tests.cpp @objects `
        bin/libgran.a '../Dom lib C++/bin/libberesta.a' -static -ld3d11 -ld3d12 -ldxgi -ldxguid `
        -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 `
        -o scratchpad/editor-tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
    & ./scratchpad/editor-tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Editor checks failed' }
}
finally { Pop-Location }
