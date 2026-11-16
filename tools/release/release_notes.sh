#!/usr/bin/env bash
# ============================================================================
#  Описание релиза из журнала изменений
#
#  Использование: release_notes.sh <версия> <файл результата>
#  Берёт раздел «## [<версия>]» из CHANGELOG.md и CHANGELOG.en.md (до следующего
#  заголовка второго уровня) и складывает их: сначала русский, затем английский.
#  Одно и то же описание получают релизы на GitHub и на Gitverse.
# ============================================================================
#  Release notes from the changelog
#
#  Usage: release_notes.sh <version> <output file>
#  Takes the "## [<version>]" section from CHANGELOG.md and CHANGELOG.en.md (up to the next
#  second-level heading) and joins them: Russian first, then English.
#  The releases on GitHub and on Gitverse get the same notes.
# ============================================================================
set -euo pipefail

# Печатает тело раздела версии без самого заголовка
# Prints the body of the version section without the heading itself
section() {
    local version="$1" file="$2"
    awk -v head="## [${version}]" '
        index($0, head) == 1 { found = 1; next }
        found && /^## / { exit }
        found { print }
    ' "$file"
}

# Точка входа: раздел каждого языка, между ними — разделитель
# The entry point: the section of every language, a separator between them
main() {
    local version="$1" out="$2" root
    root="$(cd "$(dirname "$0")/../.." && pwd)"
    {
        section "$version" "${root}/CHANGELOG.md"
        printf '\n---\n'
        section "$version" "${root}/CHANGELOG.en.md"
    } > "$out"
    if ! grep -q '[^[:space:]-]' "$out"; then
        echo "В CHANGELOG нет раздела ${version} / CHANGELOG has no ${version} section" >&2
        return 1
    fi
}

main "$@"
