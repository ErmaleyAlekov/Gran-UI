#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/components.hpp"
#include "gran/markup/internal/components.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    // Библиотека для всех тестов: стиль библиотеки, Card со слотами, Pair с вложенной Card, Loop сама в себе
    // The library for all tests: a library style, Card with slots, Pair with a nested Card, Loop inside itself
    const char* const LIBRARY = R"(<Library namespace="urn:test" xmlns:t="urn:test">
  <Style name="Pad" padding="7"/>
  <Component name="Card">
    <Param name="title" required="true"/>
    <Param name="note"/>
    <Param name="tone" type="color" default="#ff0000"/>
    <Param name="wide" type="boolean" default="false"/>
    <Param name="onOpen" type="action"/>
    <Style name="Frame" spacing="3" basedOn="Pad"/>
    <Stack id="frame" style="Frame" background="{$tone}">
      <Text id="caption">{$title}</Text>
      <Text id="note" visible="{$wide}">[{$note}]</Text>
      <Input id="field" labeledBy="caption" placeholder="{concat('&gt;', $title)}"/>
      <Button id="open" text="Open" onClick="{$onOpen}"/>
      <Stack id="actions" orientation="horizontal"><Slot name="actions"/></Stack>
      <Slot><Text id="empty">empty</Text></Slot>
    </Stack>
  </Component>
  <Component name="Pair">
    <Param name="first" required="true"/>
    <Stack><t:Card title="{$first}"><Text id="inner">in</Text></t:Card><Slot/></Stack>
  </Component>
  <Component name="Loop">
    <Stack><t:Loop/></Stack>
  </Component>
  <Component name="Relay">
    <Param name="title"/>
    <Stack><Input id="field" value="{$title}"/>
      <t:Card id="card" title="Inner"><Slot><Text id="fallback">{$field.text}</Text></Slot></t:Card>
    </Stack>
  </Component>
  <Component name="Numeric">
    <Param name="amount" type="number"/>
    <Text>{1 + $amount}</Text>
  </Component>
</Library>)";

    // Файл библиотеки рядом с тестами (относительно каталога запуска) / The library file next to the tests (relative to the run directory)
    void writeLibrary()
    {
        std::filesystem::create_directories("scratchpad/components");
        std::ofstream file("scratchpad/components/lib.xml", std::ios::binary | std::ios::trunc);
        file << LIBRARY;
    }

    // Окно с библиотекой и содержимым body / A window with the library and the body content
    std::string window(const std::string& body, const std::string& resources = std::string())
    {
        return R"(<Window xmlns:xi="http://www.w3.org/2001/XInclude" xmlns:t="urn:test"><Resources>)" + resources
            + R"(<xi:include href="scratchpad/components/lib.xml"/></Resources>)" + body + "</Window>";
    }

    // Текст ошибки загрузки (пусто — загрузилось) / The loading error text (empty — loaded)
    std::string loadError(const std::string& markup)
    {
        try
        {
            gran::Application app;
            app.loadString(markup);
        }
        catch (const gran::MarkupException& error)
        {
            return error.getMessage();
        }
        return std::string();
    }

    void testComponentExpands()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<t:Card id="c1" title="Hello" grow="2"/>)"));
        gran::ui::Node* root = app.find("c1");
        TEST_ASSERT(root != nullptr);
        TEST_ASSERT(root->kind == gran::ui::ElementKind::STACK);
        TEST_ASSERT(*app.markupAttribute(*root, "title") == "Hello");
        TEST_ASSERT(root->rule != nullptr && root->rule->basedOn != nullptr);
        TEST_ASSERT(app.find("c1.caption")->content.text == "Hello");
        TEST_ASSERT(app.find("c1.field")->content.labeledBy == "c1.caption");
        TEST_ASSERT(app.find("c1.open")->content.onClick.empty());
    }

    void testComponentDefaultsAndSlots()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<Stack><t:Card id="a" title="A"/><t:Card id="b" title="B" note="N" onOpen="go">
            <Button id="act" slot="actions" text="X"/><Text id="body">body</Text></t:Card></Stack>)"));
        TEST_ASSERT(app.find("a.note")->content.text == "[]");
        TEST_ASSERT(app.find("a.empty") != nullptr);
        TEST_ASSERT(app.find("b.note")->content.text == "[N]");
        TEST_ASSERT(app.find("b.empty") == nullptr);
        TEST_ASSERT(app.find("act")->parent == app.find("b.actions"));
        TEST_ASSERT(app.find("body")->parent == app.find("b"));
        TEST_ASSERT(app.find("b.open")->content.onClick == "go");
    }

    // Вложенный компонент: параметр уходит дальше, у экземпляра без id — своя приставка / A nested component: the parameter goes further, an instance without an id has its own prefix
    void testComponentNested()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<t:Pair id="p" first="L"><Text id="tail">t</Text></t:Pair>)"));
        TEST_ASSERT(app.find("p") != nullptr);
        TEST_ASSERT(app.find("p.inner") != nullptr);
        TEST_ASSERT(app.find("tail")->parent == app.find("p"));
        const gran::ui::Node* card = app.find("p.inner")->parent;
        TEST_ASSERT(card->id.rfind("p.Card-", 0) == 0);
        TEST_ASSERT(app.find(card->id + ".caption")->content.text == "L");
    }

    // Пересланный слот сохраняет ссылки автора, даже если получатель использует такое же локальное имя.
    // A forwarded slot preserves the author's references even if the receiver uses the same local name.
    void testComponentForwardedSlot()
    {
        writeLibrary();
        gran::Application app;

        app.loadString(window(R"(<Stack><Input id="field" value="Window"/>
            <t:Relay id="a" title="Outer"><Text id="assigned">{$field.text}</Text></t:Relay>
            <t:Relay id="b" title="Other"/><t:Relay id="c"/><t:Relay id="d" title=""/></Stack>)"));
        TEST_ASSERT(app.find("assigned")->parent == app.find("a.card"));
        TEST_ASSERT(app.find("assigned")->content.text == "Window");
        TEST_ASSERT(app.find("b.fallback")->parent == app.find("b.card"));
        TEST_ASSERT(app.find("b.fallback")->content.text == "Other");
        TEST_ASSERT(!app.markupAttribute(*app.find("c.field"), "value").has_value());
        TEST_ASSERT(app.markupAttribute(*app.find("d.field"), "value") == "");
    }

    // Числовой параметр принимает знак плюс, но значение по умолчанию не может быть выражением без скобок.
    // A number parameter accepts a leading plus, but its default cannot be an expression without braces.
    void testComponentNumberParameter()
    {
        writeLibrary();
        gran::Application app;

        app.loadString(window(R"(<t:Numeric id="number" amount="+2.5"/>)"));
        TEST_ASSERT(app.find("number")->content.text == "3.5");
        std::ofstream("scratchpad/components/lib.xml", std::ios::binary | std::ios::trunc)
            << R"XML(<Library namespace="urn:test"><Component name="Numeric">
                <Param name="amount" type="number" default="1 + 2"/>
                <Text>{1 + $amount}</Text></Component></Library>)XML";
        TEST_ASSERT_THROWS(app.loadString(window(R"(<t:Numeric/>)")), gran::MarkupException);
    }

    // Два объявления одного слота не должны создавать две копии переданного узла.
    // Two declarations of one slot must not create two copies of the supplied node.
    void testComponentDuplicateSlots()
    {
        std::filesystem::create_directories("scratchpad/components");
        std::ofstream("scratchpad/components/lib.xml", std::ios::binary | std::ios::trunc)
            << R"(<Library namespace="urn:test"><Component name="Duplicate"><Stack><Slot/><Slot/>)"
            << R"(</Stack></Component></Library>)";
        TEST_ASSERT(loadError(window(R"(<t:Duplicate/>)")).find("Duplicate") != std::string::npos);
    }

    // Цепочка разных компонентов не ограничивается проверкой циклов в исходной разметке.
    // A chain of distinct components is not bounded by cycle detection in the source markup.
    void testComponentExpansionLimits()
    {
        std::filesystem::create_directories("scratchpad/components");
        std::ofstream file("scratchpad/components/lib.xml", std::ios::binary | std::ios::trunc);

        file << R"(<Library namespace="urn:test" xmlns:t="urn:test">)";
        for (int depth = 0; depth < 65; ++depth)
            file << "<Component name=\"Part" << depth << "\"><t:Part" << depth + 1 << "/></Component>";
        file << R"(<Component name="Part65"><Stack/></Component></Library>)";
        file.close();
        TEST_ASSERT(gran::markup::loadMarkupString(window("<t:Part2/>"), {}).root != nullptr);
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(window("<t:Part1/>"), {}), gran::MarkupException);
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(window("<t:Part63/>"), {"", 2}), gran::MarkupException);
    }

    // Ошибка построения тела должна вернуть контекст к области вызывающего компонента.
    // An error while building the body must restore the calling component's scope.
    void testComponentRestoresScopeOnError()
    {
        namespace markup = gran::markup;
        namespace internal = gran::markup::internal;
        writeLibrary();
        markup::UiDocument document = markup::loadMarkupString(window(R"(<t:Card title="T"/>)"), {});
        const markup::LoadOptions options{"", 1};
        const internal::ComponentScope scope;
        internal::BuildContext context{options, document, document.tokens, 1};
        bool nodeLimitReached = false;

        context.scope = &scope;
        context.expansionDepth = 3;
        try
        {
            internal::expandComponent(*document.root->domElement, context);
        }
        catch (const gran::MarkupException& error)
        {
            nodeLimitReached = error.getCode() == gran::ErrorCode::LIMIT_EXCEEDED && context.nodeCount == 2;
        }
        TEST_ASSERT(nodeLimitReached);
        TEST_ASSERT(context.scope == &scope);
        TEST_ASSERT(context.expansionDepth == 3);
    }

    void testComponentErrors()
    {
        writeLibrary();
        TEST_ASSERT(!loadError(window(R"(<t:Card/>)")).empty());
        TEST_ASSERT(loadError(window(R"(<t:Card title="x"><Text slot="nope"/></t:Card>)")).find("nope") != std::string::npos);
        TEST_ASSERT(loadError(window(R"(<t:Nope/>)")).find("Nope") != std::string::npos);
        TEST_ASSERT(loadError(window(R"(<t:Loop/>)")).find("Loop") != std::string::npos);
        TEST_ASSERT(!loadError(window(R"(<Stack><Slot/></Stack>)")).empty());
        TEST_ASSERT(!loadError(window(R"(<u:X xmlns:u="urn:other"/>)")).empty());
        TEST_ASSERT(!loadError(window(R"(<Button onClick="{$x}"/>)")).empty());
        TEST_ASSERT(!loadError(window(R"(<t:Card title="x" wide="maybe"/>)")).empty());
        TEST_ASSERT(!loadError(window(R"(<t:Card title="x" unknown="1"/>)")).empty());
    }

    // Битая библиотека: ошибка называет файл / A broken library: the error names the file
    void testComponentLibraryErrors()
    {
        std::filesystem::create_directories("scratchpad/components");
        std::ofstream("scratchpad/components/bad.xml", std::ios::binary | std::ios::trunc)
            << R"(<Library namespace="urn:bad"><Component name="C"><Param name="width"/><Stack/></Component></Library>)";
        std::string reserved = loadError(R"(<Window xmlns:xi="http://www.w3.org/2001/XInclude"><Resources>
            <xi:include href="scratchpad/components/bad.xml"/></Resources><Stack/></Window>)");
        TEST_ASSERT(reserved.find("bad.xml") != std::string::npos);
        TEST_ASSERT(reserved.find("width") != std::string::npos);
        gran::Application app;
        TEST_ASSERT_THROWS(app.loadString(R"(<Window xmlns:xi="http://www.w3.org/2001/XInclude"><Resources>
            <xi:include href="scratchpad/components/missing.xml"/></Resources><Stack/></Window>)"), gran::GranException);
    }

    void testComponentSchemaText()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<t:Card title="x"/>)"));
        std::string schema = app.componentSchema("urn:test");
        TEST_ASSERT(schema.find("targetNamespace=\"urn:test\"") != std::string::npos);
        TEST_ASSERT(schema.find("name=\"Card\"") != std::string::npos);
        TEST_ASSERT(schema.find("name=\"title\" type=\"xs:string\" use=\"required\"") != std::string::npos);
        TEST_ASSERT(app.componentSchema("urn:none").empty());
    }

    // Живая правка экземпляра перестраивает развёртку; сохраняется экземпляр, а не развёртка / A live edit of an instance rebuilds the expansion; the instance is saved, not the expansion
    void testComponentLiveEdit()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<Stack id="page"><t:Card id="c1" title="Old"/></Stack>)"));
        gran::ui::Node* root = app.find("c1");
        app.setMarkupAttribute(*root, "title", "New");
        TEST_ASSERT(app.find("c1") == root);
        TEST_ASSERT(app.find("c1.caption")->content.text == "New");
        TEST_ASSERT_THROWS(app.removeMarkupAttribute(*root, "title"), gran::MarkupException);
        TEST_ASSERT(app.find("c1.caption")->content.text == "New");
        TEST_ASSERT_THROWS(app.setMarkupAttribute(*app.find("c1.caption"), "fontSize", "20"), gran::GranException);
        std::string saved = app.saveMarkup();
        TEST_ASSERT(saved.find("<t:Card id=\"c1\" title=\"New\"/>") != std::string::npos);
        TEST_ASSERT(saved.find("xi:include") != std::string::npos);
        TEST_ASSERT(saved.find("Frame") == std::string::npos);
    }

    // Выражение экземпляра остаётся живой привязкой / The instance expression stays a live binding
    void testComponentBindings()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<t:Card id="c1" title="{$name}" wide="{$show}"/>)",
            R"(<Variable name="name" value="A"/><Variable name="show" type="boolean" value="false"/>)"));
        TEST_ASSERT(app.find("c1.caption")->content.text == "A");
        TEST_ASSERT(app.find("c1.field")->content.placeholder == ">A");
        TEST_ASSERT(!app.find("c1.note")->visible);
        app.setVariable("name", "B");
        app.setVariableBoolean("show", true);
        app.refreshBindings();
        TEST_ASSERT(app.find("c1.caption")->content.text == "B");
        TEST_ASSERT(app.find("c1.field")->content.placeholder == ">B");
        TEST_ASSERT(app.find("c1.note")->visible);
    }

    // Фрагмент с экземпляром проверяется той же схемой библиотек / A fragment with an instance is checked by the same library schema
    void testComponentFragment()
    {
        writeLibrary();
        gran::Application app;
        app.loadString(window(R"(<Stack id="page"/>)"));
        app.appendMarkup(*app.find("page"), R"(<t:Card xmlns:t="urn:test" id="f" title="F"/>)");
        TEST_ASSERT(app.find("f.caption")->content.text == "F");
        TEST_ASSERT_THROWS(app.appendMarkup(*app.find("page"), R"(<t:Card xmlns:t="urn:test"/>)"), gran::MarkupException);
    }

    // Файл с текстом / A file with a text
    void writeFile(const std::string& path, const std::string& text)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << text;
    }

    // Частичная перезагрузка сохраняет состояние; правка файла библиотеки тоже перезагружает окно
    // A partial reload keeps the state; an edit of the library file reloads the window too
    void testComponentReload()
    {
        const std::string library = "scratchpad/components/reload_lib.xml";
        const std::string page = "scratchpad/components/reload_page.xml";
        const std::string head = R"(<Window xmlns:xi="http://www.w3.org/2001/XInclude" xmlns:r="urn:reload"><Resources>)"
            R"(<xi:include href="reload_lib.xml"/></Resources><Stack id="page"><Input id="field"/>)";
        std::filesystem::create_directories("scratchpad/components");
        writeFile(library, R"(<Library namespace="urn:reload"><Component name="Tag"><Param name="title"/>)"
            R"(<Stack><Text id="caption">{$title}</Text></Stack></Component></Library>)");
        writeFile(page, head + R"(<r:Tag id="c1" title="A"/></Stack></Window>)");
        gran::Application app;
        app.loadFile(page);
        app.find("field")->content.text = "typed";
        writeFile(page, head + R"(<r:Tag id="c1" title="Bb"/></Stack></Window>)");
        TEST_ASSERT(app.reloadIfChanged());
        TEST_ASSERT(app.find("c1.caption")->content.text == "Bb");
        TEST_ASSERT(app.find("field")->content.text == "typed");
        writeFile(library, R"(<Library namespace="urn:reload"><Component name="Tag"><Param name="title"/>)"
            R"(<Stack><Text id="caption">{$title}!</Text></Stack></Component></Library>)");
        TEST_ASSERT(app.reloadIfChanged());
        TEST_ASSERT(app.find("c1.caption")->content.text == "Bb!");
    }
}

namespace suites
{
    void registerComponentTests(test::TestRunner& runner)
    {
        runner.add("ComponentTests_Expands", testComponentExpands);
        runner.add("ComponentTests_DefaultsAndSlots", testComponentDefaultsAndSlots);
        runner.add("ComponentTests_Nested", testComponentNested);
        runner.add("ComponentTests_ForwardedSlot", testComponentForwardedSlot);
        runner.add("ComponentTests_NumberParameter", testComponentNumberParameter);
        runner.add("ComponentTests_DuplicateSlots", testComponentDuplicateSlots);
        runner.add("ComponentTests_ExpansionLimits", testComponentExpansionLimits);
        runner.add("ComponentTests_RestoresScopeOnError", testComponentRestoresScopeOnError);
        runner.add("ComponentTests_Errors", testComponentErrors);
        runner.add("ComponentTests_LibraryErrors", testComponentLibraryErrors);
        runner.add("ComponentTests_SchemaText", testComponentSchemaText);
        runner.add("ComponentTests_LiveEdit", testComponentLiveEdit);
        runner.add("ComponentTests_Bindings", testComponentBindings);
        runner.add("ComponentTests_Fragment", testComponentFragment);
        runner.add("ComponentTests_Reload", testComponentReload);
    }
}
