#include "framework/test_framework.hpp"
#include "gran/input/internal/text_keys.hpp"
#include "gran/input/internal/text_cursor.hpp"
#include "gran/platform/host.hpp"

namespace
{
    void testTextStepsMacOS()
    {
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::UP, gran::platform::MODIFIER_ALT, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::PARAGRAPH_BACK);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::DOWN, gran::platform::MODIFIER_ALT, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::PARAGRAPH_FORWARD);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::A, gran::platform::MODIFIER_META, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::PARAGRAPH_START);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::E, gran::platform::MODIFIER_META, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::PARAGRAPH_END);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::X, gran::platform::MODIFIER_META, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::NONE);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::K, gran::platform::MODIFIER_META, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::PARAGRAPH_END);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::K, 0, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::NONE);
    }

    void testTextStepsWindows()
    {
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::A, gran::platform::MODIFIER_META, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::NONE);
    }

    void testWordNavigation()
    {
        std::string text = "ab\n\ncd";
        TEST_ASSERT(gran::input::internal::wordEndAfter(text, 2) == 3);
        TEST_ASSERT(gran::input::internal::wordEndAfter(text, 3) == 4);
        TEST_ASSERT(gran::input::internal::wordEndAfter(text, 4) == 6);
        TEST_ASSERT(gran::input::internal::wordStartBefore(text, 4) == 3);
        TEST_ASSERT(gran::input::internal::wordStartBefore(text, 3) == 2);
        TEST_ASSERT(gran::input::internal::wordStartBefore(text, 2) == 0);
        TEST_ASSERT(gran::input::internal::wordNextStart(text, 0) == 2);
        TEST_ASSERT(gran::input::internal::wordNextStart(text, 2) == 3);
    }

    void testWordNavigationWithSpaces()
    {
        std::string spaced = "ab  \ncd";
        TEST_ASSERT(gran::input::internal::wordNextStart(spaced, 0) == 4);
    }

    void testWordNavigationWithIndentation()
    {
        std::string indented = "ab\n  cd";
        TEST_ASSERT(gran::input::internal::wordStartBefore(indented, 5) == 3);
    }

    void testParagraphNavigation()
    {
        std::string para = "one\ntwo";
        TEST_ASSERT(gran::input::internal::paragraphStart(para, 5, false) == 4);
        TEST_ASSERT(gran::input::internal::paragraphStart(para, 4, false) == 4);
        TEST_ASSERT(gran::input::internal::paragraphStart(para, 4, true) == 0);
        TEST_ASSERT(gran::input::internal::paragraphEnd(para, 1, false) == 3);
        TEST_ASSERT(gran::input::internal::paragraphEnd(para, 3, false) == 3);
        TEST_ASSERT(gran::input::internal::paragraphEnd(para, 3, true) == 7);
        TEST_ASSERT(gran::input::internal::paragraphEnd(para, 100, false) == 7);
        TEST_ASSERT(gran::input::internal::paragraphStart(para, 0, true) == 0);
    }
}

namespace suites
{
    void registerTextStepTests(test::TestRunner& runner)
    {
        runner.add("text steps: macos", testTextStepsMacOS);
        runner.add("text steps: windows", testTextStepsWindows);
        runner.add("text steps: word navigation", testWordNavigation);
        runner.add("text steps: word navigation with spaces", testWordNavigationWithSpaces);
        runner.add("text steps: word navigation with indentation", testWordNavigationWithIndentation);
        runner.add("text steps: paragraph navigation", testParagraphNavigation);
    }
}
