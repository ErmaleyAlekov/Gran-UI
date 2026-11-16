#!/usr/bin/env bash
# ============================================================================
#  Релиз на Gitverse через публичный API (https://api.gitverse.ru)
#
#  Использование: GV_API_TOKEN=… gitverse_release.sh <тег> <файл описания> [файлы…]
#  Находит релиз по тегу или создаёт его (название «Грань UI X.Y.Z», описание из
#  файла) и прикрепляет файлы. Файл с тем же именем сначала удаляется, поэтому
#  повторный запуск заменяет файлы, а не падает. Тег уже должен быть на Gitverse.
#  Токен — «Настройки → Токены» на gitverse.ru с правом «Репозитории → Запись»;
#  в CI/CD он лежит в секрете GV_API_TOKEN (префикс GITVERSE_ в секретах запрещён).
#  Нужны curl и python3 (разбор и сборка JSON).
# ============================================================================
#  A Gitverse release through the public API (https://api.gitverse.ru)
#
#  Usage: GV_API_TOKEN=… gitverse_release.sh <tag> <notes file> [files…]
#  Finds the release by tag or creates it (title "Грань UI X.Y.Z", notes from
#  the file) and attaches the files. A file with the same name is deleted first, so
#  a re-run replaces the files instead of failing. The tag must already be on Gitverse.
#  The token — "Settings → Tokens" on gitverse.ru with the "Repositories → Write" right;
#  in CI/CD it is kept in the GV_API_TOKEN secret (the GITVERSE_ prefix is forbidden for secrets).
#  curl and python3 are needed (parsing and building JSON).
# ============================================================================
set -euo pipefail

GV_API_URL="${GV_API_URL:-https://api.gitverse.ru}"
GV_REPOSITORY="${GV_REPOSITORY:-ermaleyalekov/Gran-UI}"
GV_ACCEPT='application/vnd.gitverse.object+json;version=1'
# Интерпретатор Python (в Windows обычно python, а не python3)
# The Python interpreter (on Windows usually python, not python3)
PYTHON="${PYTHON:-python3}"

# Запрос к API: метод, путь, файл ответа, остальные аргументы — curl; печатает HTTP-код
# An API request: method, path, response file, the remaining arguments go to curl; prints the HTTP code
gv_api() {
    local method="$1" path="$2" out="$3"
    shift 3
    curl -s -X "$method" -o "$out" -w '%{http_code}' \
        -H "Authorization: Bearer ${GV_API_TOKEN}" -H "Accept: ${GV_ACCEPT}" \
        "$@" "${GV_API_URL}/repos/${GV_REPOSITORY}${path}"
}

# Печатает id релиза по тегу; релиза нет — создаёт его из файла описания
# Prints the release id by tag; no release — creates it from the notes file
release_id() {
    local tag="$1" notes="$2" out code body
    out="$(mktemp)"
    code=$(gv_api GET "/releases/tags/${tag}" "$out")
    if [ "$code" != "200" ]; then
        body=$("$PYTHON" -c 'import json,sys; print(json.dumps({"tag_name": sys.argv[1], "name": "Грань UI " + sys.argv[1].lstrip("v"), "body": open(sys.argv[2], encoding="utf-8").read()}))' "$tag" "$notes")
        code=$(gv_api POST "/releases" "$out" -H 'Content-Type: application/json' --data "$body")
        if [ "$code" != "201" ]; then
            echo "Релиз не создан, HTTP ${code} / The release was not created: $(cat "$out")" >&2
            return 1
        fi
    fi
    "$PYTHON" -c 'import json,sys; print(json.load(open(sys.argv[1]))["id"])' "$out" | tr -d '\r'
    rm -f "$out"
}

# Удаляет у релиза файл с тем же именем, если он уже прикреплён
# Deletes a release file with the same name if it is already attached
drop_asset() {
    local id="$1" name="$2" out asset
    out="$(mktemp)"
    gv_api GET "/releases/${id}/assets" "$out" > /dev/null
    asset=$("$PYTHON" -c 'import json,sys; d=json.load(open(sys.argv[1])); d=d if isinstance(d, list) else d.get("assets", []); print(next((str(a["id"]) for a in d if a.get("name") == sys.argv[2]), ""))' "$out" "$name" 2> /dev/null | tr -d '\r' || true)
    if [ -n "$asset" ]; then
        gv_api DELETE "/releases/${id}/assets/${asset}" "$out" > /dev/null
    fi
    rm -f "$out"
}

# Прикрепляет один файл к релизу (multipart, поле attachment, имя — в ?name=)
# Attaches one file to the release (multipart, the attachment field, the name in ?name=)
upload_asset() {
    local id="$1" file="$2" name code out
    # Gitverse принимает только расширения из своего списка (zip, tgz, whl, jar, …), .nupkg
    # в нём нет: такой файл кладётся в zip — file.ext.zip
    # Gitverse accepts only the extensions from its list (zip, tgz, whl, jar, …), .nupkg
    # is not there: such a file is put into a zip — file.ext.zip
    case "$file" in
        *.zip|*.tgz|*.gz|*.7z|*.whl|*.jar|*.md|*.txt|*.pdf) ;;
        *) zip -qj "${file}.zip" "$file" && file="${file}.zip" ;;
    esac
    name="$(basename "$file")"
    drop_asset "$id" "$name"
    out="$(mktemp)"
    code=$(gv_api POST "/releases/${id}/assets?name=${name}" "$out" -F "attachment=@${file}")
    if [ "$code" != "201" ]; then
        echo "Файл ${name} не загружен, HTTP ${code} / The file was not uploaded: $(head -c 300 "$out")" >&2
        rm -f "$out"
        return 1
    fi
    rm -f "$out"
    echo "↑ ${name}"
}

# Точка входа: тег, файл описания, файлы для прикрепления
# The entry point: tag, notes file, files to attach
main() {
    local tag="$1" notes="$2" id file
    shift 2
    if [ -z "${GV_API_TOKEN:-}" ]; then
        echo "Нет GV_API_TOKEN / GV_API_TOKEN is not set" >&2
        return 1
    fi
    id=$(release_id "$tag" "$notes")
    for file in "$@"; do
        upload_asset "$id" "$file"
    done
    echo "Релиз ${tag} на Gitverse: https://gitverse.ru/${GV_REPOSITORY}/releases (id ${id})"
}

main "$@"
