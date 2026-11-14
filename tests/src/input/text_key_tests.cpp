#include "framework/test_framework.hpp"
#include "gran/input/internal/text_keys.hpp"
#include "gran/input/internal/text_cursor.hpp"
#include "gran/platform/host.hpp"

namespace
{
    void testTextKeysCaretMoveWindows()
    {
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, 0, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::CHAR_BACK);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::WORD_BACK);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::RIGHT, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::WORD_NEXT);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::RIGHT, gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::WORD_NEXT);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::HOME, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::TEXT_START);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, gran::platform::MODIFIER_ALT, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::NONE);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::PAGE_UP, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::NONE);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, gran::platform::MODIFIER_META, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::NONE);
    }

    void testTextKeysCaretMoveLinux()
    {
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::RIGHT, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::LINUX) == gran::input::internal::TextMove::WORD_END);
    }

    void testTextKeysCaretMoveMacOS()
    {
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, gran::platform::MODIFIER_ALT, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::WORD_BACK);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::RIGHT, gran::platform::MODIFIER_ALT, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::WORD_END);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::LINE_START);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::RIGHT, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::LINE_END);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::UP, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::TEXT_START);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::DOWN, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::TEXT_END);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::HOME, 0, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::TEXT_START);
        TEST_ASSERT(gran::input::internal::caretMove(gran::platform::Key::LEFT, gran::platform::MODIFIER_META, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::NONE);
    }

    void testTextKeysEraseMoveWindows()
    {
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::BACKSPACE, 0, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::CHAR_BACK);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::BACKSPACE, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::WORD_BACK);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::DELETE_KEY, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::WORD_NEXT);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::BACKSPACE, gran::platform::MODIFIER_ALT, gran::platform::HostOs::WINDOWS) == gran::input::internal::TextMove::NONE);
    }

    void testTextKeysEraseMoveMacOS()
    {
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::BACKSPACE, gran::platform::MODIFIER_ALT, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::WORD_BACK);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::BACKSPACE, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::LINE_START);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::DELETE_KEY, gran::platform::MODIFIER_CONTROL, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::LINE_END);
        TEST_ASSERT(gran::input::internal::eraseMove(gran::platform::Key::A, 0, gran::platform::HostOs::MACOS) == gran::input::internal::TextMove::NONE);
    }

    void testTextKeysWordNavigation()
    {
        std::string text = "one  two, three";
        TEST_ASSERT(gran::input::internal::wordStartBefore(text, 7) == 5);
        TEST_ASSERT(gran::input::internal::wordStartBefore(text, 5) == 0);
        TEST_ASSERT(gran::input::internal::wordEndAfter(text, 3) == 8);
        TEST_ASSERT(gran::input::internal::wordNextStart(text, 0) == 5);
        TEST_ASSERT(gran::input::internal::wordNextStart(text, 5) == 8);
        TEST_ASSERT(gran::input::internal::wordStartBefore(text, 0) == 0);
        TEST_ASSERT(gran::input::internal::wordEndAfter(text, text.size()) == text.size());
    }
}

namespace suites
{
    void registerTextKeyTests(test::TestRunner& runner)
    {
        runner.add("text keys: caret move windows", testTextKeysCaretMoveWindows);
        runner.add("text keys: caret move linux", testTextKeysCaretMoveLinux);
        runner.add("text keys: caret move macos", testTextKeysCaretMoveMacOS);
        runner.add("text keys: erase move windows", testTextKeysEraseMoveWindows);
        runner.add("text keys: erase move macos", testTextKeysEraseMoveMacOS);
        runner.add("text keys: word navigation", testTextKeysWordNavigation);
    }
}
