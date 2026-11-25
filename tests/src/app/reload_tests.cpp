#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/data/column_filter.hpp"
#include "gran/data/sort_filter_proxy.hpp"
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace
{
    class ReloadObserver final : public gran::data::ListObserver
    {
    public:
        void inserted(std::size_t, std::size_t) override
        {
            ++insertions;
            if (failInsertion)
                throw std::runtime_error("reload observer insertion failed");
        }
        void changed(std::size_t) override {}
        void removed(std::size_t, std::size_t) override
        {
            if (failRemoval)
                throw std::runtime_error("reload observer failed");
        }

        bool failRemoval = true;
        bool failInsertion = false;
        unsigned int insertions = 0;
    };

    class RestoredRowsObserver final : public gran::data::ListObserver
    {
    public:
        void inserted(std::size_t, std::size_t) override
        {
            sawOldRows = model->size() == 1 && model->at(0).at("value") == "old";
        }
        void removed(std::size_t, std::size_t) override {}
        void changed(std::size_t) override {}

        gran::data::ListModel* model = nullptr;
        bool sawOldRows = false;
    };

    class ReentrantReloadObserver final : public gran::data::ListObserver
    {
    public:
        void inserted(std::size_t, std::size_t) override {}
        void changed(std::size_t) override {}
        void removed(std::size_t, std::size_t) override
        {
            sawOldModel = &app->model("rows") == model;
            try { app->loadString("<Window><Text id=\"nested\"/></Window>"); }
            catch (const gran::GranException&) { rejectedLoad = true; }
            try { app->model("provisional"); }
            catch (const gran::GranException&) { rejectedModel = true; }
            throw std::runtime_error("reload observer failed");
        }

        gran::Application* app = nullptr;
        gran::data::ListModel* model = nullptr;
        bool sawOldModel = false;
        bool rejectedLoad = false;
        bool rejectedModel = false;
    };

    // Ошибка первого подписчика при восстановлении не прерывает доставку следующему.
    // The first observer throwing during recovery does not interrupt delivery to the next one.
    void testReloadDeliversRestoredRows()
    {
        ReloadObserver failing;
        RestoredRowsObserver following;
        gran::Application app;
        app.loadString(R"(<Window><Resources><Data name="rows"><Item value="old"/></Data></Resources><Text/></Window>)");
        following.model = &app.model("rows");
        failing.failInsertion = true;
        following.model->subscribe(&failing);
        following.model->subscribe(&following);
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Resources><Data name="rows"/></Resources><Text/></Window>)"),
            std::runtime_error);
        TEST_ASSERT(failing.insertions == 1 && following.sawOldRows);
    }

    // Наблюдатель может читать существующую модель, вложенная загрузка и новая модель отклоняются.
    // The observer can read an existing model, while a nested load and a new model are rejected.
    void testReloadRejectsReentrantChanges()
    {
        ReentrantReloadObserver observer;
        gran::Application app;
        app.loadString(R"(<Window><Resources><Data name="rows"><Item value="old"/></Data></Resources><Text id="old"/></Window>)");
        observer.app = &app;
        observer.model = &app.model("rows");
        observer.model->subscribe(&observer);
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Resources><Data name="rows"/></Resources><Text id="new"/></Window>)"),
            std::runtime_error);
        TEST_ASSERT(observer.sawOldModel && observer.rejectedLoad && observer.rejectedModel);
        TEST_ASSERT(app.find("old") != nullptr && app.find("nested") == nullptr && app.find("new") == nullptr);
    }

    // Ошибка смены базового фильтра оставляет фильтр столбца и прежнюю выборку.
    // A failure while changing the base filter preserves the column filter and the previous rows.
    void testReloadRestoresColumnFilter()
    {
        ReloadObserver observer;
        gran::Application app;
        auto& model = app.model("rows");
        model.append({{"name", "Ann"}, {"active", "yes"}});
        model.append({{"name", "Bob"}, {"active", "yes"}});
        model.append({{"name", "Anna"}, {"active", "no"}});
        app.loadString(R"(<Window><Resources><SortFilter name="view" source="rows" filter="@active = 'yes'"/>)"
            R"(</Resources><Text id="old"/></Window>)");
        auto& proxy = app.proxy("view");
        auto base = proxy.filter();
        auto columns = std::make_shared<gran::data::ColumnFilter>(base, nullptr);
        columns->setConditions({{"name", "Ann"}});
        proxy.setFilter(columns);
        TEST_ASSERT(base && proxy.size() == 1 && proxy.at(0).at("name") == "Ann");
        proxy.subscribe(&observer);
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Resources><SortFilter name="view" source="rows" filter="@active = 'no'"/>)"
            R"(</Resources><Text id="new"/></Window>)"), std::runtime_error);
        TEST_ASSERT(app.find("old") != nullptr && app.find("new") == nullptr);
        TEST_ASSERT(&app.proxy("view") == &proxy && proxy.filter() == columns);
        TEST_ASSERT(columns->baseRecords() == base && !columns->empty());
        TEST_ASSERT(proxy.size() == 1 && proxy.at(0).at("name") == "Ann");
    }

    // При подготовке нового документа нельзя унаследовать удалённую глобальную команду старого окна.
    // Preparing the new document must not inherit a global command removed from the old window.
    void testReloadDoesNotBindRemovedGlobalCommand()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="action" scope="global" text="Old"/>)"
            R"(</Resources><Button id="old" command="action"/></Window>)");
        TEST_ASSERT(app.find("old") != nullptr && app.find("old")->content.text == "Old");
        app.loadString(R"(<Window><Button id="new" command="action" text="New"/></Window>)");
        TEST_ASSERT(app.find("new") != nullptr && app.find("new")->content.text == "New");
    }

    void testReloadRebindsProxyChain()
    {
        gran::Application app;
        auto& oldSource = app.model("oldRows");
        auto& newSource = app.model("newRows");
        oldSource.append({{"value", "old"}});
        newSource.append({{"value", "new"}});
        auto& view = app.createProxy("view", "oldRows");
        auto& chain = app.createProxy("chain", "view");
        auto& selection = app.createSelection("pick", "chain");
        selection.activate(0, false, false);
        app.loadString(R"(<Window><Resources><SortFilter name="view" source="newRows"/></Resources><Text/></Window>)");

        TEST_ASSERT(&app.proxy("view") == &view && &view.source() == &newSource);
        TEST_ASSERT(&app.proxy("chain") == &chain && &chain.source() == &view);
        TEST_ASSERT(chain.size() == 1 && chain.at(0).at("value") == "new");
        TEST_ASSERT(selection.count() == 0 && selection.current() == gran::data::NO_ROW);
        oldSource.append({{"value", "ignored"}});
        TEST_ASSERT(chain.size() == 1);
        newSource.append({{"value", "observed"}});
        TEST_ASSERT(chain.size() == 2 && chain.at(1).at("value") == "observed");
    }

    void testReloadRebindsTreeWithoutTransferringSelection()
    {
        gran::Application app;
        auto& oldTree = app.tree("oldTree");
        auto& newTree = app.tree("newTree");
        const auto oldParent = oldTree.append(oldTree.root(), {{"value", "old parent"}});
        const auto oldChild = oldTree.append(oldParent, {{"value", "old child"}});
        const auto newParent = newTree.append(newTree.root(), {{"value", "new parent"}});
        const auto newChild = newTree.append(newParent, {{"value", "new child"}});
        TEST_ASSERT(oldParent == newParent && oldChild == newChild);
        auto& view = app.createTreeList("view", "oldTree");
        auto& selection = app.createSelection("pick", "view");
        view.expand(0);
        selection.activate(1, false, false);
        view.collapse(0);
        TEST_ASSERT(selection.count() == 0);
        app.loadString(R"(<Window><Resources><TreeList name="view" tree="newTree" expanded="1"/></Resources><Text/></Window>)");

        TEST_ASSERT(&app.treeList("view") == &view && &view.tree() == &newTree);
        TEST_ASSERT(view.expanded(0) && view.size() == 2 && view.at(1).at("value") == "new child");
        TEST_ASSERT(selection.count() == 0 && selection.current() == gran::data::NO_ROW);
    }

    void testReloadRestoresProxySourceAfterObserverFailure()
    {
        ReloadObserver observer;
        gran::Application app;
        app.loadString(R"(<Window><Resources><Data name="oldRows"><Item value="old"/></Data>)"
            R"(<SortFilter name="view" source="oldRows"/></Resources><Text id="old"/></Window>)");
        auto& oldSource = app.model("oldRows");
        auto& view = app.proxy("view");
        view.subscribe(&observer);
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Resources><Data name="newRows"><Item value="new"/></Data>)"
            R"(<SortFilter name="view" source="newRows"/></Resources><Text id="new"/></Window>)"), std::runtime_error);

        TEST_ASSERT(app.find("old") != nullptr && app.find("new") == nullptr);
        TEST_ASSERT(&app.proxy("view") == &view && &view.source() == &oldSource);
        TEST_ASSERT(view.size() == 1 && view.at(0).at("value") == "old");
        view.unsubscribe(&observer);
        oldSource.append({{"value", "observed"}});
        TEST_ASSERT(view.size() == 2 && view.at(1).at("value") == "observed");
    }

    class AppendOnSourceRemoval final : public gran::data::ListObserver
    {
    public:
        explicit AppendOnSourceRemoval(gran::data::ListModel& next) : source(next) {}
        void inserted(std::size_t, std::size_t count) override { insertions += count; }
        void changed(std::size_t) override {}
        void removed(std::size_t, std::size_t) override { source.append({{"value", "during removal"}}); }

        gran::data::ListModel& source;
        std::size_t insertions = 0;
    };

    void testReloadNotifiesOnceForRowsAddedDuringRebind()
    {
        gran::Application app;
        app.model("oldRows").append({{"value", "old"}});
        auto& source = app.model("newRows");
        source.append({{"value", "new"}});
        auto& view = app.createProxy("view", "oldRows");
        AppendOnSourceRemoval observer(source);
        view.subscribe(&observer);
        app.loadString(R"(<Window><Resources><SortFilter name="view" source="newRows"/></Resources><Text/></Window>)");
        view.unsubscribe(&observer);

        TEST_ASSERT(view.size() == 2 && observer.insertions == 2);
        TEST_ASSERT(view.at(1).at("value") == "during removal");
    }

    // При отказе подписчика прежние данные и узлы доступны, следующая загрузка работает.
    // If an observer throws, the previous data and nodes stay available and the next load works.
    void testReloadRestoresDataAfterObserverFailure()
    {
        constexpr auto original = R"(<Window><Resources><Data name="rows"><Item value="old"/></Data>)"
            R"(</Resources><Text id="old">Old</Text></Window>)";
        constexpr auto replacement = R"(<Window><Resources><Data name="rows"><Item value="new"/></Data>)"
            R"(</Resources><Text id="new">New</Text></Window>)";
        ReloadObserver observer;
        gran::Application app;
        app.loadString(original);
        auto* const node = app.find("old");
        TEST_ASSERT(node != nullptr);
        auto* const root = node->parent;
        auto& model = app.model("rows");
        model.subscribe(&observer);
        TEST_ASSERT_THROWS(app.loadString(replacement), std::runtime_error);
        TEST_ASSERT(app.find("old") == node && node->parent == root);
        TEST_ASSERT(app.find("new") == nullptr);
        TEST_ASSERT(&app.model("rows") == &model);
        TEST_ASSERT(model.size() == 1 && model.at(0).at("value") == "old");
        TEST_ASSERT(observer.insertions == 1);
        observer.failRemoval = false;
        app.loadString(replacement);
        TEST_ASSERT(app.find("old") == nullptr && app.find("new") != nullptr);
        TEST_ASSERT(&app.model("rows") == &model);
        TEST_ASSERT(model.size() == 1 && model.at(0).at("value") == "new");
    }

    void testLoadStringReplaceTree()
    {
        gran::Application app;
        app.loadString(R"(<Window><Text id="old">Old</Text></Window>)");
        auto* oldNode = app.find("old");
        TEST_ASSERT(oldNode != nullptr);
        app.focus(oldNode);
        app.loadString(R"(<Window><Text id="new">New</Text></Window>)");
        auto* newNode = app.find("new");
        TEST_ASSERT(newNode != nullptr);
        TEST_ASSERT(app.find("old") == nullptr);
        TEST_ASSERT_THROWS(app.loadString("<Window>"), gran::MarkupException);
        TEST_ASSERT(app.find("new") == newNode);
        TEST_ASSERT_THROWS(app.loadString("<Window><Resources><Font family=\"Missing\" source=\"missing-reload.ttf\"/></Resources><Text/></Window>"), gran::GranException);
        TEST_ASSERT(app.find("new") == newNode);
    }

    void testReloadWithoutLoadFile()
    {
        gran::Application app;
        TEST_ASSERT_THROWS(app.reload(), gran::GranException);
    }

    void testReloadIfChanged()
    {
        const std::string tempFilePath = "scratchpad/reload_unit.xml";
        std::filesystem::create_directories("scratchpad");
        std::ofstream tempFile(tempFilePath);
        tempFile << R"(<Window><Text id="initial">Initial</Text></Window>)";
        tempFile.close();

        gran::Application app;
        app.loadFile(tempFilePath);
        TEST_ASSERT(app.find("initial") != nullptr);

        TEST_ASSERT(!app.reloadIfChanged());

        std::ofstream tempFile2(tempFilePath);
        tempFile2 << R"(<Window><Text id="changed">Changed content longer</Text></Window>)";
        tempFile2.close();

        TEST_ASSERT(app.reloadIfChanged());
        TEST_ASSERT(app.find("changed") != nullptr);
        TEST_ASSERT(app.find("initial") == nullptr);

        TEST_ASSERT(!app.reloadIfChanged());

        std::filesystem::remove(tempFilePath);
    }

    void testReloadInvalidFile()
    {
        const std::string tempFilePath = "scratchpad/reload_unit.xml";
        std::filesystem::create_directories("scratchpad");
        std::ofstream tempFile(tempFilePath);
        tempFile << R"(<Window><Text id="invalid">Invalid</Text></Window>)";
        tempFile.close();

        gran::Application app;
        app.loadFile(tempFilePath);
        TEST_ASSERT(app.find("invalid") != nullptr);
        std::ofstream broken(tempFilePath);
        broken << "<Window>";
        broken.close();
        TEST_ASSERT_THROWS(app.reload(), gran::MarkupException);
        TEST_ASSERT(app.find("invalid") != nullptr);

        std::ofstream tempFile2(tempFilePath);
        tempFile2 << R"(<Window><Text id="valid">Valid</Text></Window>)";
        tempFile2.close();

        app.reload();
        TEST_ASSERT(app.find("valid") != nullptr);
        TEST_ASSERT(app.find("invalid") == nullptr);

        app.loadString(R"(<Window><Text id="final">Final</Text></Window>)");
        TEST_ASSERT(app.find("final") != nullptr);
        TEST_ASSERT(app.find("valid") == nullptr);
        TEST_ASSERT_THROWS(app.reload(), gran::GranException);

        std::filesystem::remove(tempFilePath);
    }
}

namespace suites
{
    void registerReloadTests(test::TestRunner& runner)
    {
        runner.add("loadString: replace tree", testLoadStringReplaceTree);
        runner.add("reload: without loadFile", testReloadWithoutLoadFile);
        runner.add("reloadIfChanged: basic functionality", testReloadIfChanged);
        runner.add("reload: invalid file", testReloadInvalidFile);
        runner.add("reload: restores data after observer failure", testReloadRestoresDataAfterObserverFailure);
        runner.add("reload: delivers restored rows after observer failure", testReloadDeliversRestoredRows);
        runner.add("reload: rejects reentrant changes", testReloadRejectsReentrantChanges);
        runner.add("reload: restores the column filter", testReloadRestoresColumnFilter);
        runner.add("reload: does not bind a removed global command", testReloadDoesNotBindRemovedGlobalCommand);
        runner.add("reload: rebinds a proxy chain", testReloadRebindsProxyChain);
        runner.add("reload: rebinds a tree without transferring selection", testReloadRebindsTreeWithoutTransferringSelection);
        runner.add("reload: restores proxy source after observer failure", testReloadRestoresProxySourceAfterObserverFailure);
        runner.add("reload: notifies once for rows added during rebind", testReloadNotifiesOnceForRowsAddedDuringRebind);
    }
}
