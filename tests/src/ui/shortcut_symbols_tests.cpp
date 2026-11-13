#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/ui/command.hpp"
#include "gran/platform/window.hpp"
#include "gran/platform/host.hpp"

namespace
{
    void testShortcutSymbols()
    {
        gran::ui::Shortcut shortcut;
        shortcut.key = gran::platform::Key::S;
        shortcut.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT;
        TEST_ASSERT(gran::ui::shortcutSymbols(shortcut) == u8"\u21E7\u2318S");

        shortcut.key = gran::platform::Key::A;
        shortcut.modifiers = gran::platform::MODIFIER_META | gran::platform::MODIFIER_ALT | gran::platform::MODIFIER_SHIFT | gran::platform::MODIFIER_CONTROL;
        TEST_ASSERT(gran::ui::shortcutSymbols(shortcut) == u8"\u2303\u2325\u21E7\u2318A");

        shortcut.key = gran::platform::Key::DELETE_KEY;
        shortcut.modifiers = 0;
        TEST_ASSERT(gran::ui::shortcutSymbols(shortcut) == u8"\u2326");

        shortcut.key = gran::platform::Key::LEFT;
        shortcut.modifiers = gran::platform::MODIFIER_ALT;
        TEST_ASSERT(gran::ui::shortcutSymbols(shortcut) == u8"\u2325\u2190");

        shortcut.key = gran::platform::Key::F5;
        shortcut.modifiers = gran::platform::MODIFIER_CONTROL;
        TEST_ASSERT(gran::ui::shortcutSymbols(shortcut) == u8"\u2318F5");
    }

    void testShortcutTextStyles()
    {
        gran::ui::Shortcut shortcut;
        shortcut.key = gran::platform::Key::S;
        shortcut.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::SYMBOLS) == gran::ui::shortcutSymbols(shortcut));
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Ctrl+Shift+S");

        shortcut.key = gran::platform::Key::DELETE_KEY;
        shortcut.modifiers = 0;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::SYMBOLS) == gran::ui::shortcutSymbols(shortcut));
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Del");

        shortcut.key = gran::platform::Key::F5;
        shortcut.modifiers = gran::platform::MODIFIER_CONTROL;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::SYMBOLS) == gran::ui::shortcutSymbols(shortcut));
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Ctrl+F5");
    }

    void testShortcutTextDefaultStyle()
    {
        gran::ui::Shortcut shortcut;
        shortcut.key = gran::platform::Key::S;
        shortcut.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT;

        if (gran::platform::hostOs() == gran::platform::HostOs::MACOS)
            TEST_ASSERT(gran::ui::shortcutText(shortcut) == u8"\u21E7\u2318S");
        else
            TEST_ASSERT(gran::ui::shortcutText(shortcut) == "Ctrl+Shift+S");
#if defined(_WIN32)
        TEST_ASSERT(gran::platform::hostOs() == gran::platform::HostOs::WINDOWS);
#elif defined(__APPLE__)
        TEST_ASSERT(gran::platform::hostOs() == gran::platform::HostOs::MACOS);
#else
        TEST_ASSERT(gran::platform::hostOs() == gran::platform::HostOs::LINUX);
#endif
    }
}

namespace suites
{
    void registerShortcutSymbolsTests(test::TestRunner& runner)
    {
        runner.add("shortcut symbols: shortcutSymbols", testShortcutSymbols);
        runner.add("shortcut symbols: shortcutTextStyles", testShortcutTextStyles);
        runner.add("shortcut symbols: shortcutTextDefaultStyle", testShortcutTextDefaultStyle);
    }
}
