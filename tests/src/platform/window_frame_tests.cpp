#include <framework/test_framework.hpp>
#include <suites/test_suites.hpp>
#ifdef _WIN32
#include <gran/platform/win32/win32_window.hpp>
#include <windowsx.h>

namespace {

LONG_PTR hitAt(HWND hwnd, int clientX, int clientY) {
    POINT pt = { clientX, clientY };
    ClientToScreen(hwnd, &pt);
    return SendMessage(hwnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
}

void testSystem() {
    gran::platform::WindowDesc desc;
    desc.width = 400.0f;
    desc.height = 300.0f;
    desc.titleBar = gran::platform::TitleBar::SYSTEM;

    auto window = gran::platform::createWindow(desc);
    TEST_ASSERT(window != nullptr);

    HWND hwnd = static_cast<HWND>(window->nativeHandle());
    TEST_ASSERT(hwnd != nullptr);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;
    int windowHeight = windowRect.bottom - windowRect.top;

    TEST_ASSERT_NEAR(static_cast<double>(clientWidth), desc.width * window->scale(), 1);
    TEST_ASSERT_NEAR(static_cast<double>(clientHeight), desc.height * window->scale(), 1);
    TEST_ASSERT(windowHeight > clientHeight);

    LONG_PTR hit = hitAt(hwnd, clientWidth / 2, static_cast<int>(-12 * window->scale()));
    TEST_ASSERT(hit == HTCAPTION);
    TEST_ASSERT(hitAt(hwnd, clientWidth / 2, clientHeight / 2) == HTCLIENT);
}

void testCustom() {
    gran::platform::WindowDesc desc;
    desc.width = 400.0f;
    desc.height = 300.0f;
    desc.titleBar = gran::platform::TitleBar::CUSTOM;

    auto window = gran::platform::createWindow(desc);
    TEST_ASSERT(window != nullptr);

    HWND hwnd = static_cast<HWND>(window->nativeHandle());
    TEST_ASSERT(hwnd != nullptr);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    TEST_ASSERT_NEAR(static_cast<double>(clientWidth), desc.width * window->scale(), 1);
    TEST_ASSERT_NEAR(static_cast<double>(clientHeight), desc.height * window->scale(), 1);
    TEST_ASSERT(windowWidth == clientWidth);
    TEST_ASSERT(windowHeight == clientHeight);

    struct HitTestFunctor {
        gran::platform::WindowRole operator()(gran::Vec2 pos) const {
            if (pos.x < 200.0f) {
                return gran::platform::WindowRole::DRAG;
            } else {
                return gran::platform::WindowRole::MAXIMIZE;
            }
        }
    };

    window->setFrameHitTest(HitTestFunctor());

    LONG_PTR hit = hitAt(hwnd, 0, 0);
    TEST_ASSERT(hit == HTTOPLEFT);
    hit = hitAt(hwnd, clientWidth / 2, 0);
    TEST_ASSERT(hit == HTTOP);
    hit = hitAt(hwnd, clientWidth - 1, 0);
    TEST_ASSERT(hit == HTTOPRIGHT);
    hit = hitAt(hwnd, 0, clientHeight - 1);
    TEST_ASSERT(hit == HTBOTTOMLEFT);
    hit = hitAt(hwnd, clientWidth - 1, clientHeight - 1);
    TEST_ASSERT(hit == HTBOTTOMRIGHT);
    hit = hitAt(hwnd, clientWidth / 2, clientHeight - 1);
    TEST_ASSERT(hit == HTBOTTOM);
    hit = hitAt(hwnd, 0, clientHeight / 2);
    TEST_ASSERT(hit == HTLEFT);
    hit = hitAt(hwnd, clientWidth - 1, clientHeight / 2);
    TEST_ASSERT(hit == HTRIGHT);

    hit = hitAt(hwnd, clientWidth / 4, clientHeight / 2);
    TEST_ASSERT(hit == HTCAPTION);
    hit = hitAt(hwnd, 3 * clientWidth / 4, clientHeight / 2);
    TEST_ASSERT(hit == HTMAXBUTTON);
}

void testFixed() {
    gran::platform::WindowDesc desc;
    desc.width = 400.0f;
    desc.height = 300.0f;
    desc.titleBar = gran::platform::TitleBar::CUSTOM;
    desc.resizable = false;
    desc.minWidth = 120;

    auto window = gran::platform::createWindow(desc);
    TEST_ASSERT(window != nullptr);

    HWND hwnd = static_cast<HWND>(window->nativeHandle());
    TEST_ASSERT(hwnd != nullptr);

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    TEST_ASSERT((style & WS_THICKFRAME) == 0);
    TEST_ASSERT((style & WS_MAXIMIZEBOX) == 0);

    window->maximize();
    TEST_ASSERT(!window->isMaximized());
    TEST_ASSERT(hitAt(hwnd, 0, 0) == HTCLIENT);
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    TEST_ASSERT(windowWidth == static_cast<int>(desc.width * window->scale()));
    TEST_ASSERT(windowHeight == static_cast<int>(desc.height * window->scale()));

    MINMAXINFO mmi{};
    mmi.ptMinTrackSize.y = 37;
    SendMessage(hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&mmi));
    TEST_ASSERT(mmi.ptMinTrackSize.x == static_cast<int>(desc.minWidth * window->scale()));
    TEST_ASSERT(mmi.ptMinTrackSize.y == 37);

    gran::platform::Event event;
    while (window->pollEvent(event)) {}
    window->close();
    bool closed = false;
    while (window->pollEvent(event))
        closed = closed || event.kind == gran::platform::EventKind::CLOSE;
    TEST_ASSERT(closed);
    TEST_ASSERT(IsWindow(hwnd));
}

void testDPI() {
    gran::platform::WindowDesc desc;
    desc.width = 400.0f;
    desc.height = 300.0f;
    desc.titleBar = gran::platform::TitleBar::CUSTOM;

    auto window = gran::platform::createWindow(desc);
    TEST_ASSERT(window != nullptr);

    HWND hwnd = static_cast<HWND>(window->nativeHandle());
    TEST_ASSERT(hwnd != nullptr);

    RECT suggestedRect = {0, 0, 600, 450};
    SendMessage(hwnd, WM_DPICHANGED, MAKELONG(144, 144), reinterpret_cast<LPARAM>(&suggestedRect));

    TEST_ASSERT(window->scale() == 1.5f);
    TEST_ASSERT(window->pixelSize().width == 600);
    TEST_ASSERT(window->pixelSize().height == 450);
}

} // namespace

#endif // _WIN32

namespace suites {

void registerPlatformFrameTests(test::TestRunner& runner) {
#ifndef _WIN32
    (void)runner;
#endif
#ifdef _WIN32
    runner.add("WindowFrameTests_CreateWindowWidth400Height300_SYSTEM", testSystem);
    runner.add("WindowFrameTests_CreateWindowWidth400Height300_CUSTOM", testCustom);
    runner.add("WindowFrameTests_CustomWindowNonResizable", testFixed);
    runner.add("WindowFrameTests_DPI", testDPI);
#endif // _WIN32
}

} // namespace suites
