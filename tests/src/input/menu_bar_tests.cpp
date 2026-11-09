#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "support/layout_helpers.hpp"
#include <string>

namespace
{
    class ActionSinkStub : public gran::input::ActionSink
    {
    public:
        void action(const std::string& name, gran::ui::Node&) override
        {
            lastAction = name;
            actionCount++;
        }

        std::string lastAction;
        int actionCount = 0;
    };

    class TextLocatorStub : public gran::input::TextLocator
    {
    public:
        std::size_t offsetAt(const gran::ui::Node&, gran::Vec2) override
        {
            return 0;
        }

        std::size_t lineEdge(const gran::ui::Node&, std::size_t offset, bool) override
        {
            return offset;
        }

        std::size_t lineMove(const gran::ui::Node&, std::size_t offset, int, float&) override
        {
            return offset;
        }

        int pageLines(const gran::ui::Node&) override
        {
            return 1;
        }
    };

    class WindowStub : public gran::platform::Window
    {
    public:
        bool pollEvent(gran::platform::Event&) override
        {
            return false;
        }

        void waitEvents(double) override
        {
        }

        void* nativeHandle() const override
        {
            return nullptr;
        }

        float scale() const override
        {
            return 1.0f;
        }

        gran::Size pixelSize() const override
        {
            return {0, 0};
        }

        void setCursor(gran::platform::CursorShape) override
        {
        }

        void show() override
        {
        }

        void setLiveResize(std::function<void()>) override
        {
        }

        std::string clipboardText() override
        {
            return clipboardText_;
        }

        void setClipboardText(const std::string& text) override
        {
            clipboardText_ = text;
        }

        void setAccessTree(gran::a11y::AccessTree*) override
        {
        }

        void accessFocusChanged() override
        {
        }

        void accessRefresh() override
        {
        }

        void wake() noexcept override
        {
        }

        std::string clipboardText_;
    };

    const char* MARKUP = R"(
        <Window>
            <Stack>
                <MenuBar id="bar">
                    <MenuItem id="file" text="_Файл">
                        <Popup id="fileMenu">
                            <MenuItem id="open" text="_Открыть" onClick="open"/>
                            <MenuItem id="under" text="a__b" onClick="under"/>
                        </Popup>
                    </MenuItem>
                    <MenuItem id="edit" text="_Правка">
                        <Popup id="editMenu">
                            <MenuItem id="copy" text="_Копировать" onClick="copy"/>
                        </Popup>
                    </MenuItem>
                    <MenuItem id="view" text="_Вид">
                        <Popup id="viewMenu">
                            <MenuItem id="zoom" text="_Масштаб" onClick="zoom"/>
                        </Popup>
                    </MenuItem>
                </MenuBar>
                <Input id="field" width="200"/>
                <ComboBox id="combo">
                    <MenuItem text="snake_case"/>
                </ComboBox>
            </Stack>
        </Window>
    )";

    void testMnemonics()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        auto* file = gran::ui::findNode(*doc.root, "file");
        auto* open = gran::ui::findNode(*doc.root, "open");
        auto* under = gran::ui::findNode(*doc.root, "under");
        auto* combo = gran::ui::findNode(*doc.root, "combo");
        auto* comboItem = combo->children[0]->children[0].get();

        TEST_ASSERT(file->content.text == u8"Файл");
        TEST_ASSERT(file->content.mnemonic == U'ф');
        TEST_ASSERT(file->content.mnemonicAt == 0);

        TEST_ASSERT(open->content.text == u8"Открыть");
        TEST_ASSERT(open->content.mnemonic == U'о');

        TEST_ASSERT(under->content.text == u8"a_b");
        TEST_ASSERT(under->content.mnemonic == 0);

        TEST_ASSERT(comboItem->content.text == u8"snake_case");
    }

    void testAltEntersBar()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ALT;
        event.modifiers = gran::platform::MODIFIER_ALT;
        router.handle(event);
        TEST_ASSERT(router.focused() == nullptr);
        event.kind = gran::platform::EventKind::KEY_UP;
        event.modifiers = 0;
        router.handle(event);

        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "file"));
        TEST_ASSERT(router.mnemonicsShown() == true);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::RIGHT;
        router.handle(event);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "edit"));

        event.key = gran::platform::Key::DOWN;
        router.handle(event);
        TEST_ASSERT(gran::ui::findNode(*doc.root, "editMenu")->visible);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "copy"));

        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);
        TEST_ASSERT(!gran::ui::findNode(*doc.root, "editMenu")->visible);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "edit"));

        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);
        TEST_ASSERT(router.focused() == nullptr);
        TEST_ASSERT(router.mnemonicsShown() == false);

        event.key = gran::platform::Key::F10;
        event.modifiers = 0;
        router.handle(event);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "file"));

        event.key = gran::platform::Key::F10;
        event.modifiers = 0;
        router.handle(event);
        TEST_ASSERT(router.focused() == nullptr);
    }

    void testAltLetter()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        router.focus(gran::ui::findNode(*doc.root, "field"));

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::CHARACTER;
        event.character = U'Ф';
        event.modifiers = gran::platform::MODIFIER_ALT;
        router.handle(event);
        TEST_ASSERT(gran::ui::findNode(*doc.root, "fileMenu")->visible);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "open"));
        TEST_ASSERT(gran::ui::findNode(*doc.root, "field")->content.text.empty());

        event.character = U'о';
        event.modifiers = 0;
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "open");
        TEST_ASSERT(!gran::ui::findNode(*doc.root, "fileMenu")->visible);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "field"));

        event.character = U'z';
        event.modifiers = gran::platform::MODIFIER_ALT;
        router.handle(event);
        TEST_ASSERT(gran::ui::findNode(*doc.root, "field")->content.text.empty());
    }

    void testAltCancelled()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ALT;
        event.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_ALT;
        router.handle(event);

        event.kind = gran::platform::EventKind::KEY_UP;
        router.handle(event);
        TEST_ASSERT(router.focused() == nullptr);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ALT;
        event.modifiers = gran::platform::MODIFIER_ALT;
        router.handle(event);

        event.key = gran::platform::Key::A;
        event.modifiers = gran::platform::MODIFIER_ALT;
        router.handle(event);

        event.kind = gran::platform::EventKind::KEY_UP;
        event.key = gran::platform::Key::A;
        router.handle(event);

        event.kind = gran::platform::EventKind::KEY_UP;
        event.key = gran::platform::Key::ALT;
        router.handle(event);
        TEST_ASSERT(router.focused() == nullptr);
    }

    void testSideKeysAndOpenState()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::CHARACTER;
        event.character = U'п';
        event.modifiers = gran::platform::MODIFIER_ALT;
        router.handle(event);
        TEST_ASSERT(gran::ui::findNode(*doc.root, "editMenu")->visible);
        TEST_ASSERT((gran::ui::findNode(*doc.root, "edit")->stateMask & gran::style::stateBit(gran::style::State::OPEN)) != 0);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::RIGHT;
        router.handle(event);
        TEST_ASSERT(!gran::ui::findNode(*doc.root, "editMenu")->visible);
        TEST_ASSERT(gran::ui::findNode(*doc.root, "viewMenu")->visible);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*doc.root, "zoom"));
        TEST_ASSERT((gran::ui::findNode(*doc.root, "edit")->stateMask & gran::style::stateBit(gran::style::State::OPEN)) == 0);
        TEST_ASSERT((gran::ui::findNode(*doc.root, "view")->stateMask & gran::style::stateBit(gran::style::State::OPEN)) != 0);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::LEFT;
        router.handle(event);
        TEST_ASSERT(gran::ui::findNode(*doc.root, "editMenu")->visible);

        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, measurer);

        auto* file = gran::ui::findNode(*doc.root, "file");
        auto* fileMenu = gran::ui::findNode(*doc.root, "fileMenu");
        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.position = {file->layout.frame.x + file->layout.frame.width / 2, file->layout.frame.y + file->layout.frame.height / 2};
        router.handle(event);
        TEST_ASSERT(fileMenu->visible);
        TEST_ASSERT(!gran::ui::findNode(*doc.root, "editMenu")->visible);

        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        event.button = gran::platform::MouseButton::LEFT;
        router.handle(event);
        TEST_ASSERT(!fileMenu->visible);
    }
}

namespace suites
{
    void registerMenuBarTests(test::TestRunner& runner)
    {
        runner.add("menu bar: mnemonics", testMnemonics);
        runner.add("menu bar: alt enters bar", testAltEntersBar);
        runner.add("menu bar: alt letter", testAltLetter);
        runner.add("menu bar: alt cancelled", testAltCancelled);
        runner.add("menu bar: side keys and open state", testSideKeysAndOpenState);
    }
}
