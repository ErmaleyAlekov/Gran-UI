#!/usr/bin/env bash
# ============================================================================
#  Подготовка задачи CI на Gitverse: пакеты сборки и «Берёста» (bin/libberesta.a).
#  Облачный раннер Gitverse — контейнер Linux x86_64: компилятора может не быть, sudo может не быть
#  (задача идёт от root), артефакты между задачами не передаются — поэтому каждая задача вызывает этот скрипт.
#  «Берёста» клонируется с Gitverse (тот же облачный сервис, быстрее), при неудаче — с GitHub.
#  Запуск: gitverse_prepare.sh <full|core> <папка «Берёсты»>; выпуск — переменная BERESTA_REF.
#    full — X11, заголовки Vulkan, Mesa lavapipe (программный Vulkan для GPU-проверок), Clang;
#    core — только компиляторы: ядро без окна ОС и GPU (NATIVE=0) не должно зависеть от X11 и Vulkan.
# ============================================================================
#  Preparing a CI job on Gitverse: the build packages and Beresta (bin/libberesta.a).
#  The Gitverse cloud runner is a Linux x86_64 container: a compiler may be missing, sudo may be missing
#  (the job runs as root), artifacts are not passed between jobs — so every job calls this script.
#  Beresta is cloned from Gitverse (the same cloud service, faster), on failure — from GitHub.
#  Usage: gitverse_prepare.sh <full|core> <Beresta folder>; the release is the BERESTA_REF variable.
#    full — X11, the Vulkan headers, Mesa lavapipe (software Vulkan for the GPU checks), Clang;
#    core — the compilers only: the core without an OS window and GPU (NATIVE=0) must not depend on X11 and Vulkan.
# ============================================================================
set -eo pipefail

mode="${1:-full}"
dest="${2:-beresta}"
ref="${BERESTA_REF:-v2.2.1}"

# Команда от имени root: напрямую, если задача уже идёт от root, иначе через sudo
# A command as root: directly if the job already runs as root, otherwise through sudo
as_root() {
    if [ "$(id -u)" = "0" ]; then
        "$@"
    else
        sudo "$@"
    fi
}

# Пакеты сборки для выбранного режима (full или core)
# The build packages for the chosen mode (full or core)
install_packages() {
    local packages="g++ make git ca-certificates clang"
    if [ "$mode" = "full" ]; then
        packages="$packages python3 libx11-dev libx11-xcb-dev libxcb1-dev libvulkan-dev mesa-vulkan-drivers"
    fi
    as_root apt-get update -qq
    # Пакеты перечислены через пробел — разбиение на слова здесь намеренное
    # The packages are space-separated — word splitting is intentional here
    as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends $packages
}

# Клон «Берёсты» на выпуске ref: сначала Gitverse, затем GitHub; неудачный клон удаляется перед повтором
# A Beresta clone at the ref release: Gitverse first, then GitHub; a failed clone is removed before the retry
fetch_beresta() {
    local url
    for url in https://gitverse.ru/ermaleyalekov/beresta.git https://github.com/ErmaleyAlekov/beresta.git; do
        if git clone -q --depth 1 --branch "$ref" "$url" "$dest"; then
            echo "Beresta $ref: $url"
            return 0
        fi
        rm -rf "$dest"
    done
    echo "Beresta $ref: clone failed" >&2
    return 1
}

install_packages
g++ --version | head -1
fetch_beresta
make -C "$dest" -j"$(nproc)" bin/libberesta.a
