#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/capi/gran.h"
#include <cstring>
#include <string>

namespace
{
    // Строка версии собирается из чисел заголовка: "МАЖОР.МИНОР.ПАТЧ" + суффикс предвыпуска.
    // The version string is built from the header numbers: "MAJOR.MINOR.PATCH" + the pre-release suffix.
    void testVersionStringMatchesNumbers()
    {
        std::string expected = std::to_string(GRAN_VERSION_MAJOR) + "." + std::to_string(GRAN_VERSION_MINOR)
            + "." + std::to_string(GRAN_VERSION_PATCH) + GRAN_VERSION_PRERELEASE;
        TEST_ASSERT(expected == GRAN_VERSION_STRING);
        // Суффикс — пустой или начинается с дефиса (правило SemVer).
        // The suffix is empty or starts with a hyphen (the SemVer rule).
        const char* suffix = GRAN_VERSION_PRERELEASE;
        TEST_ASSERT(suffix[0] == '\0' || suffix[0] == '-');
    }

    // Библиотека сообщает ту же версию и ABI, что и заголовки, с которыми собран тест.
    // The library reports the same version and ABI as the headers the test was built with.
    void testLibraryReportsHeaderVersion()
    {
        TEST_ASSERT(gran_version_number() == GRAN_VERSION_NUMBER);
        TEST_ASSERT(std::strcmp(gran_version_string(), GRAN_VERSION_STRING) == 0);
        TEST_ASSERT(gran_abi_version() == GRAN_ABI_VERSION);
    }

    // Число версии упорядочено так же, как сами версии, и AT_LEAST сравнивает его правильно.
    // The version number is ordered like the versions themselves and AT_LEAST compares it correctly.
    void testVersionOrdering()
    {
        TEST_ASSERT(GRAN_VERSION_ENCODE(1, 2, 3) == 1002003UL);
        TEST_ASSERT(GRAN_VERSION_ENCODE(0, 10, 0) > GRAN_VERSION_ENCODE(0, 9, 99));
        TEST_ASSERT(GRAN_VERSION_ENCODE(1, 0, 0) > GRAN_VERSION_ENCODE(0, 999, 999));
        TEST_ASSERT(GRAN_VERSION_AT_LEAST(GRAN_VERSION_MAJOR, GRAN_VERSION_MINOR, GRAN_VERSION_PATCH));
        TEST_ASSERT(!GRAN_VERSION_AT_LEAST(GRAN_VERSION_MAJOR + 1, 0, 0));
    }
}

namespace suites
{
    // Регистрация тестов номера версии.
    // Registration of the version number tests.
    void registerVersionTests(test::TestRunner& runner)
    {
        runner.add("version string matches numbers", testVersionStringMatchesNumbers);
        runner.add("library reports header version", testLibraryReportsHeaderVersion);
        runner.add("version ordering", testVersionOrdering);
    }
}
