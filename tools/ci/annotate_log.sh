#!/usr/bin/env bash
# ============================================================================
#  Журнал упавшей задачи CI → аннотации GitHub (::error). Журналы задач GitHub отдаёт только вошедшим
#  пользователям, а аннотации проверок читаются анонимно через API (check-runs/<id>/annotations) — так причину
#  падения видно и без токена. Две аннотации: строки ошибок (компилятор, компоновщик, упавшие тесты, итог)
#  и хвост журнала.
#  Запуск: annotate_log.sh <журнал> <заголовок> (в заголовке без запятых и двоеточий — это разделители команды).
# ============================================================================
#  A failed CI job log → GitHub annotations (::error). GitHub gives job logs only to signed-in users, while check
#  annotations are read anonymously through the API (check-runs/<id>/annotations) — so the failure reason is visible
#  without a token. Two annotations: the error lines (the compiler, the linker, the failed tests, the summary) and
#  the log tail.
#  Usage: annotate_log.sh <log> <title> (no commas and colons in the title — they are the command separators).
# ============================================================================
log="$1"
title="$2"

# Текст → одна строка команды: %, CR и LF кодируются, как требует GitHub
# A text → one command line: %, CR and LF are encoded as GitHub requires
emit() {
    local text="$1"
    text="${text//'%'/'%25'}"
    text="${text//$'\r'/'%0D'}"
    text="${text//$'\n'/'%0A'}"
    echo "::error title=${title}::${text}"
}

if [ ! -f "$log" ]; then
    emit "no log file: $log"
    exit 0
fi
# После «[FAIL] имя» тестовый каркас печатает строку с причиной — берём и её (-A1). Команды компилятора и ar
# пропускаются: в путях исходников бывают «errors» и «exception» (src/capi/errors/ «Берёсты»), и такие строки
# вытесняли настоящую ошибку за предел аннотации (Windows MinGW, M6). Строки хвоста обрезаются до 300 знаков:
# одна команда ar длиной в десятки килобайт занимала весь хвост.
# After "[FAIL] name" the test framework prints a line with the reason — it is taken too (-A1). The compiler and ar
# commands are skipped: source paths may contain "errors" and "exception" (Beresta's src/capi/errors/), and such lines
# pushed the real error out of the annotation limit (Windows MinGW, M6). The tail lines are cut to 300 characters:
# a single ar command tens of kilobytes long took the whole tail.
commands='^[^ ]*(g\+\+|clang\+\+|gcc|clang|ar)(\.exe)? '
emit "$(grep -a -v -E "$commands" "$log" | grep -a -A1 -E 'error:|error [A-Z]+[0-9]+|Error [0-9]|FAIL|undefined reference|Undefined symbols|ld: |Итого|упало|exception|Assertion|runtime error|Sanitizer' | head -n 80)"
emit "$(tail -n 60 "$log" | cut -c1-300)"
