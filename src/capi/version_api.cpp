#include "gran/capi/gran.h"

/*
 * Версия собранной библиотеки для Си-интерфейса и обёрток. Значения берутся из gran_version.h
 * в момент сборки библиотеки, поэтому программа может сравнить их со своими GRAN_VERSION_*
 * и узнать, с какой версией движка она реально работает.
 *
 * The version of the built library for the C interface and wrappers. The values come from
 * gran_version.h when the library is built, so a program can compare them with its own GRAN_VERSION_*
 * and find out which engine version it actually runs with.
 */

// Число версии библиотеки: 1.2.3 → 1002003.
// The library version number: 1.2.3 → 1002003.
uint32_t gran_version_number(void)
{
    return static_cast<uint32_t>(GRAN_VERSION_NUMBER);
}

// Строка версии библиотеки — статический литерал, живёт всё время работы программы.
// The library version string — a static literal that lives for the whole program run.
const char* gran_version_string(void)
{
    return GRAN_VERSION_STRING;
}
