#ifndef GRAN_GRAN_VERSION_H
#define GRAN_GRAN_VERSION_H

/*
 * Версия движка Gran UI по правилам семантического версионирования (https://semver.org/lang/ru/):
 * МАЖОРНАЯ.МИНОРНАЯ.ПАТЧ[-предвыпуск]. Это единственное место, где записан номер версии:
 * CMake читает числа отсюда (project VERSION), тест сверяет строку и число, теги git (vX.Y.Z)
 * и CHANGELOG.md обязаны совпадать с ними. Правила совместимости — docs/versioning.md.
 * На ветке main номер — следующая минорная версия с суффиксом "-dev"; на ветке release/X.Y —
 * номер последнего или готовящегося патч-выпуска без суффикса.
 *
 * Gran UI engine version under Semantic Versioning (https://semver.org/):
 * MAJOR.MINOR.PATCH[-prerelease]. This is the only place where the version number is written:
 * CMake reads the numbers from here (project VERSION), a test checks the string against the number,
 * git tags (vX.Y.Z) and CHANGELOG.md must match them. Compatibility rules are in docs/versioning.md.
 * On the main branch the number is the next minor version with the "-dev" suffix; on a release/X.Y
 * branch it is the number of the latest or upcoming patch release without a suffix.
 */

/* Мажорная версия: растёт при несовместимых изменениях (до 1.0 — см. docs/versioning.md).
 * Major version: grows on incompatible changes (before 1.0 — see docs/versioning.md). */
#define GRAN_VERSION_MAJOR 0

/* Минорная версия: новые возможности без поломки совместимости.
 * Minor version: new features without breaking compatibility. */
#define GRAN_VERSION_MINOR 2

/* Патч-версия: только исправления, выпускается с ветки release/X.Y.
 * Patch version: fixes only, released from the release/X.Y branch. */
#define GRAN_VERSION_PATCH 0

/* Суффикс предвыпуска с дефисом ("-dev", "-rc.1") или пустая строка у выпуска.
 * Pre-release suffix with a hyphen ("-dev", "-rc.1") or an empty string for a release. */
#define GRAN_VERSION_PRERELEASE "-dev"

/* Версия двоичного интерфейса Си (gran.h). Растёт только при несовместимом изменении ABI:
 * удаление функции, смена сигнатуры, раскладки структуры или значения перечисления.
 * Добавление новых функций ABI не меняет.
 * Version of the C binary interface (gran.h). It grows only on an incompatible ABI change:
 * removing a function, changing a signature, a structure layout or an enumeration value.
 * Adding new functions does not change the ABI. */
#define GRAN_ABI_VERSION 1

/* Вспомогательные макросы превращения числа в строковый литерал (два шага, чтобы раскрыть аргумент).
 * Helper macros that turn a number into a string literal (two steps so the argument is expanded). */
#define GRAN_VERSION_STRINGIFY_(value) #value
#define GRAN_VERSION_STRINGIFY(value) GRAN_VERSION_STRINGIFY_(value)

/* Полная строка версии, например "0.1.0" или "0.2.0-dev".
 * The full version string, for example "0.1.0" or "0.2.0-dev". */
#define GRAN_VERSION_STRING                                                     \
    GRAN_VERSION_STRINGIFY(GRAN_VERSION_MAJOR) "."                              \
    GRAN_VERSION_STRINGIFY(GRAN_VERSION_MINOR) "."                              \
    GRAN_VERSION_STRINGIFY(GRAN_VERSION_PATCH) GRAN_VERSION_PRERELEASE

/* Сборка номера в одно число для сравнений: 1.2.3 → 1002003 (по три десятичных разряда на часть).
 * Packing a version into one number for comparisons: 1.2.3 → 1002003 (three decimal digits per part). */
#define GRAN_VERSION_ENCODE(major, minor, patch)                                \
    ((major) * 1000000UL + (minor) * 1000UL + (patch))

/* Версия этих заголовков одним числом.
 * The version of these headers as one number. */
#define GRAN_VERSION_NUMBER                                                     \
    GRAN_VERSION_ENCODE(GRAN_VERSION_MAJOR, GRAN_VERSION_MINOR, GRAN_VERSION_PATCH)

/* Проверка при компиляции: «заголовки не старше указанной версии».
 * Пример: #if GRAN_VERSION_AT_LEAST(0, 2, 0) ... #endif
 * Compile-time check: "the headers are not older than the given version".
 * Example: #if GRAN_VERSION_AT_LEAST(0, 2, 0) ... #endif */
#define GRAN_VERSION_AT_LEAST(major, minor, patch)                              \
    (GRAN_VERSION_NUMBER >= GRAN_VERSION_ENCODE(major, minor, patch))

#endif /* GRAN_GRAN_VERSION_H */
