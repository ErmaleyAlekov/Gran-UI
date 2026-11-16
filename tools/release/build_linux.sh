#!/usr/bin/env bash
# ============================================================================
#  Сборка архива выпуска Грани UI для Linux x86_64 (П.2)
#
#  Использование (из корня Грани): build_linux.sh <каталог «Берёсты»> <каталог архивов>
#  Архив называется по старшей версии GCC: gran-ui-<версия>-linux-x86_64-gcc<N>.tar.gz.
#  Статические библиотеки C++ привязаны к компилятору (std::filesystem GCC 8 несовместим
#  с GCC 9+), поэтому release.yml запускает скрипт через docker run в двух образах:
#  - Astra Linux SE 1.7 (GCC 8.3, glibc 2.28) — для программ на GCC 8 (Astra 1.7, РЕД ОС 7.3);
#  - Альт p10 (GCC 10.3, glibc 2.32) — для программ на GCC 10 и новее.
#  Инструменты и libgran_c.so годятся из любого архива на системах с glibc не старше образа.
#  Скрипт можно запустить и прямо на системе (архив тогда под её GCC и glibc); release.yml так не делает.
# ============================================================================
#  Building the Gran UI release archive for Linux x86_64 (P.2)
#
#  Usage (from the Gran root): build_linux.sh <Beresta directory> <archive directory>
#  The archive is named by the GCC major version: gran-ui-<version>-linux-x86_64-gcc<N>.tar.gz.
#  Static C++ libraries are bound to the compiler (the GCC 8 std::filesystem is incompatible
#  with GCC 9+), so release.yml runs the script through docker run in two images:
#  - Astra Linux SE 1.7 (GCC 8.3, glibc 2.28) — for programs on GCC 8 (Astra 1.7, RED OS 7.3);
#  - ALT p10 (GCC 10.3, glibc 2.32) — for programs on GCC 10 and newer.
#  The tools and libgran_c.so fit from either archive on systems with a glibc not older than the image.
#  The script can also run right on a system (the archive then fits its GCC and glibc); release.yml does not do that.
# ============================================================================
set -euo pipefail

. "$(dirname "$0")/lib/release_common.sh"

# Пакеты сборки: компилятор, X11 и заголовки Vulkan (загрузчик Vulkan программа
# открывает сама через dlopen, поэтому от его версии архив не зависит). Debian и Astra —
# имена пакетов deb; в Альте apt-get тот же, но пакеты — RPM с другими именами
# The build packages: the compiler, X11 and the Vulkan headers (the program opens the
# Vulkan loader itself through dlopen, so the archive does not depend on its version). Debian and Astra
# have deb package names; ALT has the same apt-get, but the packages are RPMs with other names
linux_tools() {
    if command -v dpkg > /dev/null; then
        apt_install g++ make git libx11-dev libvulkan-dev
    else
        apt_install gcc-c++ make git libX11-devel libvulkan-devel
    fi
    g++ --version | head -n 1
}

# Сборка «Берёсты» и Грани, модульные тесты; в журнал — наибольшая версия glibc,
# которую требуют общая библиотека и инструменты (по ней видно, где архив заработает)
# Builds Beresta and Gran, the unit tests; the log gets the highest glibc version
# the shared library and the tools require (it shows where the archive will run)
build_gran() {
    local beresta="$1" jobs
    jobs="$(nproc 2> /dev/null || echo 2)"
    make -C "$beresta" -j"$jobs" bin/libberesta.a
    make BERESTA_DIR="$beresta" platform
    make -j"$jobs" BERESTA_DIR="$beresta" all bindings
    make BERESTA_DIR="$beresta" test
    # Повторный make ничего не компилирует: в make 4.2.1 (Astra 1.7) отметка флагов obj/.flags
    # не совпадала сама с собой, и каждый вызов пересобирал движок целиком (найдено 2026-11-15)
    # A repeated make compiles nothing: in make 4.2.1 (Astra 1.7) the obj/.flags flags stamp
    # did not match itself, and every call rebuilt the whole engine (found 2026-11-15)
    if [ "$(make -n BERESTA_DIR="$beresta" all bindings | grep -c '^g++')" != "0" ]; then
        echo "Повторный make пересобирает файлы / A repeated make rebuilds files" >&2
        return 1
    fi
    echo "glibc:$(objdump -T bin/libgran_c.so bin/gran_snapshot bin/gran_rcc \
        | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -n 1)"
}

# Раскладка и упаковка архива gran-ui-<версия>-linux-x86_64-gcc<N>.tar.gz
# Lays out and packs the gran-ui-<version>-linux-x86_64-gcc<N>.tar.gz archive
package_linux() {
    local beresta="$1" out="$2" name stage
    name="gran-ui-$(gran_version)-linux-x86_64-gcc$(g++ -dumpversion | cut -d. -f1)"
    stage="$(mktemp -d)"
    sdk_stage "$stage" "$name" "$beresta"
    cp bin/libgran.a bin/libgran_c.so "$beresta/bin/libberesta.a" "$stage/$name/lib/"
    cp bin/gran_rcc bin/gran_snapshot "$stage/$name/bin/"
    sdk_pack "$stage" "$name" "$out" tar.gz
    rm -rf "$stage"
}

# Точка входа. Каталог «Берёсты» переводится в абсолютный путь: make Грани
# вызывается из её корня, а «Берёста» может лежать рядом
# The entry point. The Beresta directory becomes an absolute path: the Gran make
# runs from its root, and Beresta may lie next to it
main() {
    local beresta out
    beresta="$(cd "$1" && pwd)"
    out="$2"
    # Владелец рабочего дерева в контейнере — не root: git иначе отказывается читать его
    # The work tree owner in the container is not root: git refuses to read it otherwise
    git config --global --add safe.directory '*' 2> /dev/null || true
    linux_tools
    build_gran "$beresta"
    package_linux "$beresta" "$out"
}

main "$@"
