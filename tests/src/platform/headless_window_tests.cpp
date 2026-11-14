#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/rhi/null/null_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include <thread>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
    void testHeadlessWindowPushEvents()
    {
        gran::platform::HeadlessWindow window;
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        window.pushEvent(event);
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        window.pushEvent(event);
        event.kind = gran::platform::EventKind::KEY_DOWN;
        window.pushEvent(event);

        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::MOUSE_MOVE);
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::MOUSE_DOWN);
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::KEY_DOWN);
        TEST_ASSERT(!window.pollEvent(event));
    }

    void testHeadlessWindowWaitEventsEmptyQueue()
    {
        gran::platform::HeadlessWindow window;
        window.waitEvents(0.01);
    }

    void testHeadlessWindowWake()
    {
        gran::platform::HeadlessWindow window;
        std::thread t([&window]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            window.wake();
        });

        window.waitEvents(-1.0);
        t.join();
    }

    void testHeadlessWindowClose()
    {
        gran::platform::HeadlessWindow window;
        window.close();

        gran::platform::Event event;
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::CLOSE);
        TEST_ASSERT(!window.pollEvent(event));
    }

    void testHeadlessWindowResizeAndScale()
    {
        gran::platform::WindowDesc desc;
        desc.width = 200.0f;
        desc.height = 100.0f;
        gran::platform::HeadlessWindow window(desc);
        window.changeScale(1.5f);

        gran::platform::Event event;
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::DPI_CHANGED && event.scale == 1.5f);
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::RESIZE && event.width == 300 && event.height == 150);
        TEST_ASSERT(!window.pollEvent(event));

        TEST_ASSERT(window.pixelSize().width == 300.0f && window.pixelSize().height == 150.0f);
    }

    void testHeadlessWindowClipboard()
    {
        gran::platform::HeadlessWindow window1;
        gran::platform::HeadlessWindow window2;

        window1.setClipboardText("x");
        TEST_ASSERT(window2.clipboardText() == "x");
    }

    void testHeadlessWindowDropFiles()
    {
        gran::platform::HeadlessWindow window;
        window.dropFiles({10, 20}, {"a.txt", "b.txt"});

        gran::platform::Event event;
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::DRAG_MOVE && event.position.x == 10);
        TEST_ASSERT(window.pollEvent(event) && event.kind == gran::platform::EventKind::DROP && event.position.x == 10);

        std::vector<std::string> paths = window.takeDroppedFiles();
        TEST_ASSERT(paths.size() == 2 && paths[0] == "a.txt" && paths[1] == "b.txt");
        TEST_ASSERT(window.takeDroppedFiles().empty());
    }

    void testHeadlessWindowStateChanges()
    {
        gran::platform::HeadlessWindow window;
        window.maximize();
        TEST_ASSERT(window.isMaximized());

        window.restore();
        TEST_ASSERT(!window.isMaximized());

        window.minimize();
        TEST_ASSERT(window.state().minimized);

        window.activate();
        TEST_ASSERT(!window.state().minimized && window.state().activations == 1);

        window.setCursor(gran::platform::CursorShape::HAND);
        TEST_ASSERT(window.state().cursor == gran::platform::CursorShape::HAND);
    }

    void testNullBackendTextureManagement()
    {
        gran::rhi::NullBackend backend;
        TEST_ASSERT(backend.createTexture({4, 4, gran::rhi::TextureFormat::RGBA8, "t"}) == 0);
        TEST_ASSERT(backend.createTexture({4, 4, gran::rhi::TextureFormat::RGBA8, "t"}) == 1);

        backend.releaseTexture(0);
        TEST_ASSERT(backend.createTexture({4, 4, gran::rhi::TextureFormat::RGBA8, "t"}) == 0);

        std::vector<std::uint8_t> pixels(64);
        TEST_ASSERT_THROWS(backend.updateTexture(1, {2, 2, 4, 4}, pixels.data(), 16), gran::GranException);
        backend.updateTexture(1, {0, 0, 4, 4}, pixels.data(), 16);
        TEST_ASSERT_THROWS(backend.updateTexture(1, {0, 0, 4, 4}, pixels.data(), 8), gran::GranException);

        TEST_ASSERT_THROWS(backend.releaseTexture(7), gran::GranException);
    }

    void testNullBackendRender()
    {
        gran::rhi::NullBackend backend;
        gran::render::GpuInstance instances[2] = {};
        gran::rhi::FrameSegment segment{1, 5, false, 0.0f};
        gran::rhi::FrameData frame;
        frame.instanceCount = 2;
        frame.instances = instances;
        TEST_ASSERT(backend.render(frame) == gran::rhi::FrameStatus::PRESENTED);
        TEST_ASSERT(backend.stats().frames == 1 && backend.stats().lastInstances == 2);

        frame.segments = &segment;
        frame.segmentCount = 1;
        TEST_ASSERT_THROWS(backend.render(frame), gran::GranException);
    }
}

namespace suites
{
    void registerHeadlessWindowTests(test::TestRunner& runner)
    {
        runner.add("headless: pushEvents", testHeadlessWindowPushEvents);
        runner.add("headless: waitEventsEmptyQueue", testHeadlessWindowWaitEventsEmptyQueue);
        runner.add("headless: wake", testHeadlessWindowWake);
        runner.add("headless: close", testHeadlessWindowClose);
        runner.add("headless: resizeAndScale", testHeadlessWindowResizeAndScale);
        runner.add("headless: clipboard", testHeadlessWindowClipboard);
        runner.add("headless: dropFiles", testHeadlessWindowDropFiles);
        runner.add("headless: stateChanges", testHeadlessWindowStateChanges);
        runner.add("headless: nullBackendTextureManagement", testNullBackendTextureManagement);
        runner.add("headless: nullBackendRender", testNullBackendRender);
    }
}
