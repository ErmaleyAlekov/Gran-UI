#!/usr/bin/env bash
# ============================================================================
#  Общие функции сборки выпуска Грани UI (П.2): версия из gran_version.h, пакеты
#  apt, раскладка архива выпуска и его упаковка. Подключается через «.» из
#  build_linux.sh и build_windows.sh; сам по себе ничего не запускает.
#
#  Раскладка архива gran-ui-<версия>-<платформа>/:
#    include/   — заголовки Грани (gran/…) и «Берёсты» (один публичный заголовок
#                 Грани подключает заголовок SAX «Берёсты», без них не собрать программу);
#    lib/       — статические libgran и libberesta, общая библиотека gran_c (C ABI);
#    bin/       — инструменты gran_rcc (ресурсы в exe) и gran_snapshot (кадр без окна);
#    schemas/   — схема разметки gran-ui.xsd и тема по умолчанию;
#    samples/   — примеры разметки (их можно сразу снять gran_snapshot);
#    docs/, LICENSE, LICENSE.beresta, README, CHANGELOG, BUILD_INFO.txt.
# ============================================================================
#  Common functions of the Gran UI release build (P.2): the version from gran_version.h, apt
#  packages, the release archive layout and its packing. Sourced with "." from
#  build_linux.sh and build_windows.sh; it runs nothing by itself.
#
#  The archive layout gran-ui-<version>-<platform>/:
#    include/   — the Gran headers (gran/…) and the Beresta ones (one public Gran header
#                 includes the Beresta SAX header, a program does not build without them);
#    lib/       — the static libgran and libberesta, the gran_c shared library (C ABI);
#    bin/       — the gran_rcc (resources in the exe) and gran_snapshot (a frame without a window) tools;
#    schemas/   — the gran-ui.xsd markup schema and the default theme;
#    samples/   — markup samples (gran_snapshot can render them right away);
#    docs/, LICENSE, LICENSE.beresta, README, CHANGELOG, BUILD_INFO.txt.
# ============================================================================

# Печатает версию движка «X.Y.Z[-суффикс]» из include/gran/gran_version.h
# (единственное место, где записан номер). Запуск — из корня Грани.
# Prints the engine version "X.Y.Z[-suffix]" from include/gran/gran_version.h
# (the only place where the number is written). Run from the Gran root.
gran_version() {
    local header="include/gran/gran_version.h" major minor patch suffix
    major="$(sed -n 's/^#define GRAN_VERSION_MAJOR \([0-9]*\).*/\1/p' "$header")"
    minor="$(sed -n 's/^#define GRAN_VERSION_MINOR \([0-9]*\).*/\1/p' "$header")"
    patch="$(sed -n 's/^#define GRAN_VERSION_PATCH \([0-9]*\).*/\1/p' "$header")"
    suffix="$(sed -n 's/^#define GRAN_VERSION_PRERELEASE "\([^"]*\)".*/\1/p' "$header")"
    # Пустое число — заголовок изменился и разбор устарел: лучше упасть, чем выпустить «..»
    # An empty number means the header changed and the parsing is stale: failing beats releasing ".."
    if [ -z "$major" ] || [ -z "$minor" ] || [ -z "$patch" ]; then
        echo "Не разобрана версия в ${header} / The version in ${header} is not parsed" >&2
        return 1
    fi
    echo "${major}.${minor}.${patch}${suffix}"
}

# Ставит пакеты apt (в контейнере Astra — от root, на раннере Ubuntu — через sudo)
# Installs apt packages (in the Astra container as root, on the Ubuntu runner through sudo)
apt_install() {
    local sudo=""
    if [ "$(id -u)" != "0" ]; then
        sudo="sudo"
    fi
    $sudo apt-get update -qq
    $sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "$@"
}

# Создаёт каталог <stage>/<name> с общей частью архива (всё, кроме lib/ и bin/,
# которые кладёт вызывающий). Аргументы: каталог сборки, имя архива, каталог «Берёсты».
# Creates the <stage>/<name> directory with the common part of the archive (everything but lib/ and bin/,
# which the caller puts). Arguments: the staging directory, the archive name, the Beresta directory.
sdk_stage() {
    local stage="$1" name="$2" beresta="$3" root="$1/$2"
    rm -rf "$root"
    mkdir -p "$root/lib" "$root/bin" "$root/include"
    cp -R include/gran "$root/include/"
    cp -R "$beresta/include/." "$root/include/"
    cp -R schemas samples docs LICENSE README.md README.en.md CHANGELOG.md CHANGELOG.en.md "$root/"
    cp "$beresta/LICENSE" "$root/LICENSE.beresta"
    # Сведения о сборке — для отчётов об ошибках: на чём и из чего собран архив
    # The build information — for bug reports: on what and from what the archive was built
    {
        echo "Gran UI $(gran_version) ${name}"
        echo "commit: $(git rev-parse HEAD 2> /dev/null || echo unknown)"
        echo "beresta: $(git -C "$beresta" describe --tags --always 2> /dev/null || echo unknown)"
        echo "compiler: ${COMPILER_INFO:-$(${CXX:-g++} --version 2> /dev/null | head -n 1)}"
        echo "date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "$root/BUILD_INFO.txt"
}

# Упаковывает <stage>/<name> в <out>/<name>.<kind> (kind: tar.gz — сохраняет права
# исполнения на Linux; zip — Windows: zip, 7z или tar.exe из Windows 10+).
# Packs <stage>/<name> into <out>/<name>.<kind> (kind: tar.gz keeps the execute
# rights on Linux; zip is for Windows: zip, 7z or tar.exe of Windows 10+).
sdk_pack() {
    local stage="$1" name="$2" out="$3" kind="$4" out_abs
    mkdir -p "$out"
    out_abs="$(cd "$out" && pwd)"
    rm -f "${out_abs}/${name}.${kind}"
    # Архив собирается из каталога сборки, чтобы внутри был один корень <name>/
    # The archive is built from the staging directory so that it has one root <name>/
    if [ "$kind" = "tar.gz" ]; then
        (cd "$stage" && tar -czf "${out_abs}/${name}.tar.gz" "$name")
    elif command -v zip > /dev/null; then
        (cd "$stage" && zip -qr "${out_abs}/${name}.zip" "$name")
    elif command -v 7z > /dev/null; then
        (cd "$stage" && 7z a -tzip "${out_abs}/${name}.zip" "$name" > /dev/null)
    else
        (cd "$stage" && /c/Windows/System32/tar.exe -a -c -f "$(cygpath -w "${out_abs}/${name}.zip")" "$name")
    fi
    echo "Архив / Archive: ${out}/${name}.${kind}"
}
