#!/usr/bin/env bash
# ============================================================================
#  Скачивание файлов релиза GitHub для зеркалирования на Gitverse
#
#  Использование: github_assets.sh <тег> <каталог> [минут ожидания]
#  Релиз на GitHub собирает release.yml (Linux и Windows, ~30–40 минут). Скрипт
#  ждёт, пока релиз появится, и скачивает все его файлы в каталог.
#  gh release create публикует релиз только после загрузки всех файлов, поэтому
#  видимый релиз уже полный. Не дождался — код 3, каталог остаётся пустым.
#  Список файлов берётся из API, а если API не отвечает 200 (у общего IP облачных
#  раннеров быстро кончается анонимный лимит — 60 запросов в час), — со страницы
#  файлов релиза на github.com (releases/expanded_assets), у неё этого лимита нет.
#  Нужны только curl и python3; токен GitHub не нужен (репозиторий открытый).
# ============================================================================
#  Downloading the GitHub release files to mirror them on Gitverse
#
#  Usage: github_assets.sh <tag> <directory> [minutes to wait]
#  The GitHub release is built by release.yml (Linux and Windows, ~30–40 minutes). The script
#  waits until the release appears and downloads all its files into the directory.
#  gh release create publishes a release only after all files are uploaded, so a
#  visible release is already complete. Not waited out — exit code 3, the directory stays empty.
#  The file list comes from the API, and if the API does not answer 200 (the anonymous limit of
#  60 requests per hour runs out fast on the shared IP of cloud runners), — from the release
#  files page on github.com (releases/expanded_assets), which has no such limit.
#  Only curl and python3 are needed; no GitHub token (the repository is public).
# ============================================================================
set -euo pipefail

# Репозиторий на GitHub можно переопределить переменной окружения
# The GitHub repository can be overridden by an environment variable
GH_REPOSITORY="${GH_REPOSITORY:-ErmaleyAlekov/Gran-UI}"
# Интерпретатор Python (в Windows обычно python, а не python3)
# The Python interpreter (on Windows usually python, not python3)
PYTHON="${PYTHON:-python3}"

# Печатает ссылки на файлы релиза из API; не 200 — печатает код в stderr и возвращает 1
# Prints the release file links from the API; not 200 — prints the code to stderr and returns 1
api_urls() {
    local tag="$1" json code
    json="$(mktemp)"
    code=$(curl -s -o "$json" -w '%{http_code}' -H 'Accept: application/vnd.github+json' \
        "https://api.github.com/repos/${GH_REPOSITORY}/releases/tags/${tag}" || true)
    if [ "$code" != "200" ]; then
        echo "GitHub API: HTTP ${code} $(head -c 160 "$json" | tr -d '\r\n')" >&2
        rm -f "$json"
        return 1
    fi
    # Python в Windows завершает строки CR LF — tr срезает CR
    # Python on Windows ends lines with CR LF — tr cuts the CR off
    "$PYTHON" -c 'import json,sys; [print(a["browser_download_url"]) for a in json.load(open(sys.argv[1]))["assets"]]' "$json" |
    tr -d '\r'
    rm -f "$json"
}

# Печатает ссылки на файлы со страницы expanded_assets; пусто — возвращает 1
# Prints the file links from the expanded_assets page; empty — returns 1
page_urls() {
    local tag="$1" links
    links=$(curl -sfL "https://github.com/${GH_REPOSITORY}/releases/expanded_assets/${tag}" |
        grep -o "href=\"/${GH_REPOSITORY}/releases/download/[^\"]*\"" | sort -u || true)
    if [ -z "$links" ]; then
        echo "github.com: на странице релиза ${tag} нет файлов / no files on the release page" >&2
        return 1
    fi
    echo "$links" | sed -e 's|^href="|https://github.com|' -e 's|"$||'
}

# Опрашивает раз в минуту, пока не найдётся список файлов или не выйдет время
# Polls once a minute until the file list is found or the time runs out
wait_urls() {
    local tag="$1" list="$2" minutes="$3" attempt=0
    # Пустой ответ API тоже считается неудачей: тогда список берётся со страницы
    # An empty API answer counts as a failure too: then the list is taken from the page
    while ! { { api_urls "$tag" > "$list" && [ -s "$list" ]; } || page_urls "$tag" > "$list"; }; do
        attempt=$((attempt + 1))
        if [ "$attempt" -gt "$minutes" ]; then
            echo "Релиз ${tag} на GitHub не найден за ${minutes} мин / The GitHub release was not found in ${minutes} min"
            return 3
        fi
        echo "Жду релиз ${tag} на GitHub (${attempt}/${minutes}) / Waiting for the GitHub release"
        sleep 60
    done
}

# Точка входа: тег, каталог, минуты ожидания (по умолчанию 20); каждый файл — по ссылке
# The entry point: tag, directory, minutes to wait (20 by default); every file by its link
main() {
    local tag="$1" dir="$2" minutes="${3:-20}" list url
    list="$(mktemp)"
    wait_urls "$tag" "$list" "$minutes"
    mkdir -p "$dir"
    while read -r url; do
        echo "↓ ${url##*/}"
        curl -sfL -o "${dir}/${url##*/}" "$url"
    done < "$list"
    rm -f "$list"
}

main "$@"
