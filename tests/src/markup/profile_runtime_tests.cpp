#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"

#include <stdexcept>

namespace
{
    struct RuntimeEditor
    {
        gran::ui::Node* tab;
        gran::ui::Node* page;
    };

    RuntimeEditor appendEditor(gran::Application& app, int& clicks)
    {
        using gran::ui::ElementKind;
        auto tab = std::make_unique<gran::ui::Node>();
        tab->kind = ElementKind::BUTTON;
        tab->id = "tab-1";
        tab->content.text = "Draft";
        auto* tabNode = &app.append(*app.find("editorTabs"), std::move(tab));
        app.addEventListener(*tabNode, "click", [&clicks](auto&, auto&) { ++clicks; });
        auto page = std::make_unique<gran::ui::Node>();
        page->kind = ElementKind::TEXT_AREA;
        page->id = "document-1";
        auto* pageNode = &app.append(*app.find("editorPages"), std::move(page));
        app.setStyle(*pageNode, "EditorPage");
        return {tabNode, pageNode};
    }

    void switchProfiles(gran::Application& app, gran::AppDriver& driver, const RuntimeEditor& editor)
    {
        const auto history = editor.page->content.history;
        const auto caret = editor.page->content.caret;
        const auto anchor = editor.page->content.anchor;
        TEST_ASSERT(history && history->steps.canUndo());
        auto verifyDraft = [&] {
            TEST_ASSERT(app.find("tab-1") == editor.tab);
            TEST_ASSERT(app.find("document-1") == editor.page);
            TEST_ASSERT(editor.page->content.text == "Draft text");
            TEST_ASSERT(editor.page->content.caret == caret && editor.page->content.anchor == anchor);
            TEST_ASSERT(editor.page->content.history == history);
            TEST_ASSERT(editor.page->rule && editor.page->rule->name == "EditorPage");
        };
        app.importProfile("larger", R"(<Overlay version="1"><Token name="size" value="14"/></Overlay>)");
        driver.settle();
        verifyDraft();
        TEST_ASSERT_NEAR(editor.page->computed.fontSize, 14.0f, 0.001f);
        app.importProfile("moved", R"(<Overlay version="1"><Move id="editor" parent="right"/></Overlay>)");
        driver.settle();
        verifyDraft();
        TEST_ASSERT(app.find("editor")->parent == app.find("right"));
    }

    void testRuntimeEditorProfiles()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Theme><Token name="size" type="size" value="12"/></Theme>
          <Style name="EditorPage" fontSize="$size"/></Resources><Stack>
          <Stack id="left"><Stack id="editor" customizable="move">
          <Stack id="editorTabs"/><Stack id="editorPages"/></Stack></Stack>
          <Stack id="right" customizable="insert"/></Stack></Window>)");
        int clicks = 0;
        const auto editor = appendEditor(app, clicks);
        gran::AppDriver driver(app);
        driver.start();
        app.focus(editor.page);
        driver.type("Draft text");
        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(editor.page->content.caret < editor.page->content.text.size());
        switchProfiles(app, driver, editor);
        driver.press(gran::platform::Key::Z, gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(editor.page->content.text != "Draft text");
        driver.press(gran::platform::Key::Z, gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT(editor.page->content.text == "Draft text");
        app.dispatchEvent(*editor.tab, "click");
        TEST_ASSERT(clicks == 1);
    }

    void testProfileFocusObserverFailure()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Theme><Token name="ink" type="color" value="#000000"/>
            </Theme></Resources><Stack id="host"><Input onFocus="observe"/><Text color="$ink">Text</Text></Stack></Window>)");
        auto* host = app.find("host");
        auto child = std::make_unique<gran::ui::Node>();
        child->kind = gran::ui::ElementKind::TEXT;
        child->id = "runtime";
        auto* runtime = &app.append(*host, std::move(child));
        gran::AppDriver driver(app);
        driver.start();
        int focusEvents = 0;
        app.onEvent("observe", [&](auto&, auto&) {
            if (++focusEvents == 2) throw std::runtime_error("focus observer failed");
        });
        app.focus(host->children.front().get());
        const std::string overlay = R"(<Overlay version="1"><Token name="ink" value="#333333"/></Overlay>)";
        app.importProfile("changed", overlay);
        driver.settle();
        TEST_ASSERT(focusEvents == 2 && app.find("runtime") == runtime);
        TEST_ASSERT(app.settings().getString("gran.activeProfile") == "changed");
        TEST_ASSERT(app.exportProfile("changed") == overlay && app.reloadError().empty());
    }

    void testProfileKeepsRuntimeTabHeaderFocus()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Stack><Tabs id="tabs"><Tab id="staticPage" text="Static" customizable="move"/>
            <Tab text="Remain"/></Tabs><Tabs id="other" customizable="insert"><Tab text="Other"/></Tabs></Stack></Window>)");
        auto* tabs = app.find("tabs");
        for (const char* id : {"first", "second"})
        {
            auto tab = std::make_unique<gran::ui::Node>();
            tab->kind = gran::ui::ElementKind::TAB;
            tab->id = tab->content.option = tab->content.text = id;
            app.append(*tabs, std::move(tab));
        }
        gran::ui::syncTabs(*tabs);
        app.selectTab(*tabs, "second");
        gran::AppDriver driver(app);
        driver.start();
        app.focus(tabs->children.front()->children.at(2).get());
        app.importProfile("moved", R"(<Overlay version="1"><Move id="staticPage" parent="other"/></Overlay>)");
        driver.settle();
        tabs = app.find("tabs");
        TEST_ASSERT(app.find("staticPage")->parent == app.find("other"));
        TEST_ASSERT(tabs->content.option == "second" && !app.find("first")->visible && app.find("second")->visible);
        const auto focused = gran::style::stateBit(gran::style::State::FOCUSED);
        const auto& headers = tabs->children.front()->children;
        TEST_ASSERT(headers.size() == 3 && headers.at(1)->content.option == "first");
        TEST_ASSERT((headers.at(1)->stateMask & focused) != 0);
        TEST_ASSERT((headers.front()->stateMask & focused) == 0 && (headers.back()->stateMask & focused) == 0);
    }

    void testProfileMovesFocusedStaticTabHeader()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Stack>
            <Tabs id="tabs"><Tab id="staticPage" value="shared" text="Static" customizable="move"/><Tab text="Remain"/></Tabs>
            <Tabs id="other" customizable="insert"><Tab value="other" text="Other"/></Tabs>
            <Tabs id="unrelated"><Tab value="shared" text="Unrelated"/></Tabs>
            </Stack></Window>)");
        auto* tabs = app.find("tabs");
        auto tab = std::make_unique<gran::ui::Node>();
        tab->kind = gran::ui::ElementKind::TAB;
        tab->id = tab->content.option = tab->content.text = "dynamic";
        auto* runtime = &app.append(*tabs, std::move(tab));
        gran::ui::syncTabs(*tabs);
        app.selectTab(*tabs, "dynamic");
        gran::AppDriver driver(app);
        driver.start();
        app.focus(tabs->children.front()->children.front().get());
        app.importProfile("moved", R"(<Overlay version="1"><Move id="staticPage" parent="other"/></Overlay>)");
        driver.settle();
        auto* page = app.find("staticPage");
        const auto mask = gran::style::stateBit(gran::style::State::FOCUSED);
        auto& targetHeader = *app.find("other")->children.front()->children.back();
        const auto& unrelatedHeader = *app.find("unrelated")->children.front()->children.front();
        TEST_ASSERT(gran::ui::pageOf(targetHeader) == page && (targetHeader.stateMask & mask) != 0);
        TEST_ASSERT((unrelatedHeader.stateMask & mask) == 0);
        TEST_ASSERT(app.find("dynamic") == runtime && runtime->visible && app.find("tabs")->content.option == "dynamic");
    }
}

namespace suites
{
    void registerProfileRuntimeTests(test::TestRunner& runner)
    {
        runner.add("customization: profiles preserve open editor tabs", testRuntimeEditorProfiles);
        runner.add("customization: installed profile survives a throwing focus observer", testProfileFocusObserverFailure);
        runner.add("customization: profile restores the focused runtime tab header", testProfileKeepsRuntimeTabHeaderFocus);
        runner.add("customization: profile follows a moved static tab header", testProfileMovesFocusedStaticTabHeader);
    }
}
