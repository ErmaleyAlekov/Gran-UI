#ifndef GRAN_GRAN_EXPORT_H
#define GRAN_GRAN_EXPORT_H

/*
 * Макросы видимости символов и пометки стабильности публичного интерфейса.
 * Заголовок совместим с C и C++, исполняемого кода не содержит.
 *
 * ГАРАНТИИ НА ВЕТКАХ release/X.Y (полный текст — docs/versioning.md):
 *  1. Двоичная совместимость Си-интерфейса: программа или обёртка, собранная с gran_c X.Y.0,
 *     работает с любым gran_c X.Y.Z без пересборки. Функции, их сигнатуры, раскладка структур,
 *     значения перечислений и кодов gran_status не меняются; GRAN_ABI_VERSION не меняется.
 *  2. Совместимость исходников C++: объявления в include/gran (кроме папок internal/) не удаляются
 *     и не переименовываются; поведение меняется только там, где оно было ошибкой.
 *     Двоичной совместимости C++ нет: libgran — статическая библиотека, программа пересобирается.
 *  3. Совместимость разметки: документ, проходящий schemas/gran-ui.xsd версии X.Y.0, проходит её
 *     в X.Y.Z; формат файлов настроек и ресурсов (gran-settings.xsd, gran-resources.xsd) не меняется.
 *  4. Идентификаторы сообщений каталога и коды ошибок не удаляются и не меняют смысл.
 *  5. Минимальные требования (ОС, компиляторы, glibc) не повышаются.
 *  6. В патч-выпуск попадают только исправления ошибок, уязвимостей, сборки и документации.
 * Что гарантиями НЕ покрыто: всё в папках internal/, объявления с GRAN_EXPERIMENTAL, точное
 * попиксельное совпадение кадра между выпусками, сгенерированные файлы и примеры.
 *
 * Symbol visibility macros and stability markers of the public interface.
 * The header is C and C++ compatible and contains no executable code.
 *
 * GUARANTEES ON release/X.Y BRANCHES (full text in docs/versioning.en.md):
 *  1. Binary compatibility of the C interface: a program or a wrapper built against gran_c X.Y.0
 *     works with any gran_c X.Y.Z without a rebuild. Functions, their signatures, structure layouts,
 *     enumeration values and gran_status codes do not change; GRAN_ABI_VERSION does not change.
 *  2. C++ source compatibility: declarations in include/gran (except internal/ folders) are not
 *     removed or renamed; behaviour changes only where it was a bug.
 *     There is no C++ binary compatibility: libgran is a static library, the program is rebuilt.
 *  3. Markup compatibility: a document that passes schemas/gran-ui.xsd of X.Y.0 passes it
 *     in X.Y.Z; the settings and resources file formats (gran-settings.xsd, gran-resources.xsd) do not change.
 *  4. Message catalog identifiers and error codes are not removed and keep their meaning.
 *  5. Minimum requirements (OS, compilers, glibc) are not raised.
 *  6. A patch release contains only fixes of bugs, vulnerabilities, the build and documentation.
 * NOT covered by the guarantees: everything in internal/ folders, declarations marked GRAN_EXPERIMENTAL,
 * exact pixel equality of a frame between releases, generated files and examples.
 */

#include "gran/gran_version.h"

/* GRAN_API — видимость функции Си-интерфейса.
 * При сборке gran_c.dll задаётся GRAN_CAPI_BUILD (экспорт), программа с DLL задаёт GRAN_CAPI_SHARED
 * (импорт); на GCC/Clang символ делается видимым при -fvisibility=hidden; статическая сборка — пусто.
 * GRAN_API — visibility of a C interface function.
 * Building gran_c.dll defines GRAN_CAPI_BUILD (export), a program using the DLL defines GRAN_CAPI_SHARED
 * (import); on GCC/Clang the symbol is made visible under -fvisibility=hidden; a static build — empty. */
#if defined(_WIN32) && defined(GRAN_CAPI_BUILD)
# define GRAN_API __declspec(dllexport)
#elif defined(_WIN32) && defined(GRAN_CAPI_SHARED)
# define GRAN_API __declspec(dllimport)
#elif defined(__GNUC__)
# define GRAN_API __attribute__((visibility("default")))
#else
# define GRAN_API
#endif

/* GRAN_DEPRECATED("замена") — объявление устарело: компилятор предупредит при использовании.
 * Ставится перед объявлением. Устаревшее живёт минимум одну минорную версию (после 1.0 — до
 * следующей мажорной) и удаляется только на main, никогда на ветке release/X.Y.
 * GRAN_DEPRECATED("replacement") — the declaration is deprecated: the compiler warns on use.
 * It goes before the declaration. A deprecated item lives at least one minor version (after 1.0 —
 * until the next major one) and is removed only on main, never on a release/X.Y branch. */
#if defined(__cplusplus) && __cplusplus >= 201402L
# define GRAN_DEPRECATED(message) [[deprecated(message)]]
#elif defined(_MSC_VER)
# define GRAN_DEPRECATED(message) __declspec(deprecated(message))
#elif defined(__GNUC__)
# define GRAN_DEPRECATED(message) __attribute__((deprecated(message)))
#else
# define GRAN_DEPRECATED(message)
#endif

/* GRAN_EXPERIMENTAL — пометка для читателя и инструментов: объявление новое и может измениться
 * в любой минорной версии; гарантии веток release/X.Y на него не распространяются.
 * Кода не порождает.
 * GRAN_EXPERIMENTAL — a marker for readers and tools: the declaration is new and may change
 * in any minor version; the release/X.Y branch guarantees do not apply to it.
 * It generates no code. */
#define GRAN_EXPERIMENTAL

#endif /* GRAN_GRAN_EXPORT_H */
