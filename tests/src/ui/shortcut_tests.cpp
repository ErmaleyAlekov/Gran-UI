#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/ui/command.hpp"
#include "gran/platform/window.hpp"

namespace
{
    void testParseShortcut()
    {
        gran::ui::Shortcut result;
        TEST_ASSERT(gran::ui::parseShortcut("Mod+S", result));
        TEST_ASSERT(result.key == gran::platform::Key::S);
        TEST_ASSERT(result.modifiers == gran::platform::MODIFIER_CONTROL);

        TEST_ASSERT(gran::ui::parseShortcut("ctrl+shift+z", result));
        TEST_ASSERT(result.key == gran::platform::Key::Z);
        TEST_ASSERT(result.modifiers == (gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT));

        TEST_ASSERT(gran::ui::parseShortcut("F5", result));
        TEST_ASSERT(result.key == gran::platform::Key::F5);
        TEST_ASSERT(result.modifiers == 0);

        TEST_ASSERT(gran::ui::parseShortcut("Alt+Enter", result));
        TEST_ASSERT(result.key == gran::platform::Key::ENTER);
        TEST_ASSERT(result.modifiers == gran::platform::MODIFIER_ALT);

        TEST_ASSERT(gran::ui::parseShortcut("Del", result));
        TEST_ASSERT(result.key == gran::platform::Key::DELETE_KEY);
        TEST_ASSERT(result.modifiers == 0);

        TEST_ASSERT(gran::ui::parseShortcut("Esc", result));
        TEST_ASSERT(result.key == gran::platform::Key::ESCAPE);
        TEST_ASSERT(result.modifiers == 0);

        TEST_ASSERT(!gran::ui::parseShortcut("Mod+Foo", result));
        TEST_ASSERT(!gran::ui::parseShortcut("Ctrl+Ctrl+S", result));
        TEST_ASSERT(!gran::ui::parseShortcut("", result));
        TEST_ASSERT(!gran::ui::parseShortcut("Shift+", result));
    }

    void testParseShortcuts()
    {
        std::vector<gran::ui::Shortcut> result;
        TEST_ASSERT(gran::ui::parseShortcuts("Mod+Y Mod+Shift+Z", result));
        TEST_ASSERT(result.size() == 2);
        TEST_ASSERT(result[0].key == gran::platform::Key::Y);
        TEST_ASSERT(result[0].modifiers == gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(result[1].key == gran::platform::Key::Z);
        TEST_ASSERT(result[1].modifiers == (gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT));

        result.clear();
        TEST_ASSERT(gran::ui::parseShortcuts("", result));
        TEST_ASSERT(result.empty());

        TEST_ASSERT(!gran::ui::parseShortcuts("Mod+S Bad+X", result));
    }

    void testShortcutText()
    {
        gran::ui::Shortcut shortcut;
        shortcut.key = gran::platform::Key::S;
        shortcut.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Ctrl+Shift+S");

        shortcut.key = gran::platform::Key::DELETE_KEY;
        shortcut.modifiers = 0;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Del");

        shortcut.key = gran::platform::Key::F5;
        shortcut.modifiers = gran::platform::MODIFIER_ALT;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Alt+F5");

        shortcut.key = gran::platform::Key::LEFT;
        shortcut.modifiers = 0;
        TEST_ASSERT(gran::ui::shortcutText(shortcut, gran::ui::ShortcutStyle::TEXT) == "Left");
    }

    void testSameShortcut()
    {
        gran::ui::Shortcut s1, s2;
        s1.key = gran::platform::Key::S;
        s1.modifiers = gran::platform::MODIFIER_CONTROL;
        s2.key = gran::platform::Key::S;
        s2.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT;
        TEST_ASSERT(!gran::ui::sameShortcut(s1, s2));

        s2.modifiers = gran::platform::MODIFIER_CONTROL;
        TEST_ASSERT(gran::ui::sameShortcut(s1, s2));
    }
}

namespace suites
{
    void registerShortcutTests(test::TestRunner& runner)
    {
        runner.add("shortcut: parseShortcut", testParseShortcut);
        runner.add("shortcut: parseShortcuts", testParseShortcuts);
        runner.add("shortcut: shortcutText", testShortcutText);
        runner.add("shortcut: sameShortcut", testSameShortcut);
    }
}
