#include <framework/test_framework.hpp>
#include <suites/test_suites.hpp>
#ifdef _WIN32
#include <gran/platform/win32/win32_window.hpp>
#include <windowsx.h>

namespace {

void testWin32AltLetterAndKeyMenu() {
    gran::platform::WindowDesc desc;
    desc.width = 400.0f;
    desc.height = 300.0f;
    desc.titleBar = gran::platform::TitleBar::SYSTEM;

    auto window = gran::platform::createWindow(desc);
    TEST_ASSERT(window != nullptr);

    HWND hwnd = static_cast<HWND>(window->nativeHandle());
    TEST_ASSERT(hwnd != nullptr);

    // Отправляем WM_SYSCHAR с кодом символа 'ф' (0x0444)
    LRESULT r = SendMessage(hwnd, WM_SYSCHAR, 0x0444, 0);
    TEST_ASSERT(r == 0);

    gran::platform::Event e;
    bool found = false;
    while (window->pollEvent(e)) {
        if (e.kind == gran::platform::EventKind::CHARACTER && e.character == 0x0444 && (e.modifiers & gran::platform::MODIFIER_ALT) != 0) {
            found = true;
            break;
        }
    }
    TEST_ASSERT(found);

    // Отправляем WM_SYSCOMMAND с SC_KEYMENU и параметром 'f'
    r = SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 'f');
    TEST_ASSERT(r == 0);
}

} // namespace

#endif // _WIN32

namespace suites {

void registerWin32KeyMenuTests(test::TestRunner& runner) {
#ifndef _WIN32
    (void)runner;
#endif
#ifdef _WIN32
    runner.add("win32: alt letter and key menu", testWin32AltLetterAndKeyMenu);
#endif // _WIN32
}

} // namespace suites
