#!/usr/bin/env bash
# ============================================================================
#  Сборка архива выпуска Грани UI для Windows x64 (П.2), в Git Bash раннера
#
#  Использование (из корня Грани): build_windows.sh <mingw|msvc|msvc-md> <каталог «Берёсты»> <каталог архивов>
#  - mingw — Makefile и MinGW-w64 GCC: libgran.a, libberesta.a, gran_c.dll (рантайм
#    GCC внутри, -static), архив gran-ui-<версия>-windows-x86_64-mingw.zip;
#  - msvc  — CMake и MSVC x64 (Release, статический рантайм /MT, как у «Берёсты»):
#    gran.lib, beresta_static.lib, gran_c.dll + gran_c.lib, архив …-windows-x86_64-msvc.zip;
#  - msvc-md — то же с динамическим рантаймом /MD (как у нового проекта Visual Studio), …-msvc-md.zip.
#  Статические библиотеки MinGW, MSVC /MT и MSVC /MD несовместимы между собой (LNK2038 — Т-53), поэтому архивов три;
#  gran_c.dll с C ABI годится из любого.
# ============================================================================
#  Building the Gran UI release archive for Windows x64 (P.2), in the runner Git Bash
#
#  Usage (from the Gran root): build_windows.sh <mingw|msvc|msvc-md> <Beresta directory> <archive directory>
#  - mingw — the Makefile and MinGW-w64 GCC: libgran.a, libberesta.a, gran_c.dll (the GCC
#    runtime inside, -static), the gran-ui-<version>-windows-x86_64-mingw.zip archive;
#  - msvc  — CMake and MSVC x64 (Release, the static /MT runtime, as Beresta has):
#    gran.lib, beresta_static.lib, gran_c.dll + gran_c.lib, the …-windows-x86_64-msvc.zip archive;
#  - msvc-md — the same with the dynamic /MD runtime (as in a new Visual Studio project), …-msvc-md.zip.
#  The MinGW, MSVC /MT and MSVC /MD static libraries are incompatible with each other (LNK2038 — T-53), hence three archives;
#  gran_c.dll with the C ABI fits from either.
# ============================================================================
set -euo pipefail

. "$(dirname "$0")/lib/release_common.sh"

# MinGW: «Берёста», Грань с примерами и C ABI, модульные тесты; файлы — в архив
# MinGW: Beresta, Gran with the examples and the C ABI, the unit tests; the files go to the archive
build_mingw() {
    local beresta="$1" root="$2" make_cmd
    # На раннере make из MinGW называется mingw32-make, в своей установке GCC — часто make
    # On the runner the MinGW make is called mingw32-make, in a custom GCC install it is often make
    make_cmd="$(command -v mingw32-make || command -v make)"
    "$make_cmd" -C "$beresta" -j"$NUMBER_OF_PROCESSORS" bin/libberesta.a
    "$make_cmd" BERESTA_DIR="$beresta" platform
    "$make_cmd" -j"$NUMBER_OF_PROCESSORS" BERESTA_DIR="$beresta" all bindings
    "$make_cmd" BERESTA_DIR="$beresta" test
    cp bin/libgran.a bin/gran_c.dll "$beresta/bin/libberesta.a" "$root/lib/"
    cp bin/gran_rcc.exe bin/gran_snapshot.exe bin/gran_lint.exe "$root/bin/"
}

# Ищет в каталоге сборки CMake файл выпускной конфигурации по имени (ровно один,
# иначе ошибка: два одноимённых файла значат, что раскладка сборки изменилась)
# Finds a Release configuration file by name in the CMake build directory (exactly one,
# otherwise an error: two files of the same name mean the build layout has changed)
find_release_file() {
    local build="$1" file="$2" found
    found="$(find "$build" -path '*Release*' -name "$file" -not -path '*CMakeFiles*')"
    if [ -z "$found" ] || [ "$(echo "$found" | wc -l)" != "1" ]; then
        echo "В ${build} не один ${file} / Not exactly one ${file} in ${build}: ${found}" >&2
        return 1
    fi
    echo "$found"
}

# MSVC: CMake (Visual Studio, x64, Release), модульные тесты через ctest; файлы — в архив.
# Третий аргумент — GRAN_MSVC_DYNAMIC_RUNTIME (OFF — /MT, ON — /MD), у каждого рантайма своя папка сборки
# MSVC: CMake (Visual Studio, x64, Release), the unit tests through ctest; the files go to the archive.
# The third argument is GRAN_MSVC_DYNAMIC_RUNTIME (OFF — /MT, ON — /MD), each runtime has its own build folder
build_msvc() {
    local beresta="$1" root="$2" dynamic="${3:-OFF}" build="build-release-msvc" file
    if [ "$dynamic" = "ON" ]; then
        build="build-release-msvc-md"
    fi
    cmake -S . -B "$build" -A x64 -DGRAN_BERESTA_DIR="$beresta" -DGRAN_MSVC_DYNAMIC_RUNTIME="$dynamic"
    cmake --build "$build" --config Release --parallel
    ctest --test-dir "$build" -C Release -R '^gran_tests$' --output-on-failure
    for file in gran.lib beresta_static.lib gran_c.dll gran_c.lib; do
        cp "$(find_release_file "$build" "$file")" "$root/lib/"
    done
    for file in gran_rcc.exe gran_snapshot.exe gran_lint.exe; do
        cp "$(find_release_file "$build" "$file")" "$root/bin/"
    done
    # Точная версия cl.exe известна только после настройки CMake — дописывается в сведения о сборке
    # The exact cl.exe version is known only after the CMake configuration — it is appended to the build information
    grep -h 'set(CMAKE_CXX_COMPILER_VERSION' "$build"/CMakeFiles/*/CMakeCXXCompiler.cmake \
        | head -n 1 | sed 's/.*"\(.*\)".*/msvc: \1/' >> "$root/BUILD_INFO.txt"
    echo "msvc dynamic runtime (/MD): ${dynamic}" >> "$root/BUILD_INFO.txt"
}

# MSVC с /MD / MSVC with /MD
build_msvc_md() {
    build_msvc "$1" "$2" ON
}

# Точка входа: сборка выбранным компилятором прямо в раскладку архива, затем упаковка
# The entry point: a build with the chosen compiler right into the archive layout, then packing
main() {
    local mode="$1" beresta out name stage
    # pwd -W даёт путь вида C:/…: его понимают нативные mingw32-make, g++ и cmake
    # pwd -W gives a C:/… path: the native mingw32-make, g++ and cmake understand it
    beresta="$(cd "$2" && pwd -W)"
    out="$3"
    if [ "$mode" != "mingw" ] && [ "$mode" != "msvc" ] && [ "$mode" != "msvc-md" ]; then
        echo "Режим mingw, msvc или msvc-md / The mode is mingw, msvc or msvc-md: ${mode}" >&2
        return 1
    fi
    name="gran-ui-$(gran_version)-windows-x86_64-${mode}"
    stage="$(mktemp -d)"
    if [ "$mode" != "mingw" ]; then
        COMPILER_INFO="MSVC x64, $(cmake --version | head -n 1)"
    fi
    sdk_stage "$stage" "$name" "$beresta"
    "build_${mode//-/_}" "$beresta" "$stage/$name"
    sdk_pack "$stage" "$name" "$out" zip
    rm -rf "$stage"
}

main "$@"
