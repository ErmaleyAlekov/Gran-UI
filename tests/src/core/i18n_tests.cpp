#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace
{
    // Метки {N} подставляются, лишние метки пропускаются, язык переключается
    // {N} placeholders are substituted, extra placeholders are skipped, the language switches
    void testFormatAndLanguage()
    {
        TEST_ASSERT_EQ(gran::i18n::format(gran::i18n::Message::STYLE_BAD_NUMBER, {"x"}),
            std::string("ожидалось число, найдено \"x\""));
        TEST_ASSERT(gran::i18n::setLanguage("en"));
        TEST_ASSERT_EQ(gran::i18n::format(gran::i18n::Message::STYLE_BAD_NUMBER, {"x"}),
            std::string("expected a number, found \"x\""));
        TEST_ASSERT(!gran::i18n::setLanguage("xx"));
        TEST_ASSERT_EQ(std::string(gran::i18n::language()), std::string("en"));
        TEST_ASSERT(gran::i18n::setLanguage("ru"));
    }

    // Все сообщения переведены на оба языка
    // All messages are translated into both languages
    void testCatalogsComplete()
    {
        for (int id = 0; id < static_cast<int>(gran::i18n::Message::COUNT); ++id)
        {
            gran::i18n::Message message = static_cast<gran::i18n::Message>(id);
            TEST_ASSERT(gran::i18n::findTemplate(gran::i18n::RUSSIAN_CATALOG, message) != nullptr);
            TEST_ASSERT(gran::i18n::findTemplate(gran::i18n::ENGLISH_CATALOG, message) != nullptr);
        }
    }

    // what() содержит имя кода / what() contains the code name
    void testExceptionText()
    {
        gran::GranException error(gran::ErrorCode::GPU, "сбой");
        TEST_ASSERT_EQ(std::string(error.what()), std::string("[GPU] сбой"));
        TEST_ASSERT(error.getCode() == gran::ErrorCode::GPU);
    }
}

namespace suites
{
    // Регистрация набора / The suite registration
    void registerI18nTests(test::TestRunner& runner)
    {
        runner.add("i18n: format и язык", testFormatAndLanguage);
        runner.add("i18n: каталоги полны", testCatalogsComplete);
        runner.add("i18n: текст исключения", testExceptionText);
    }
}
