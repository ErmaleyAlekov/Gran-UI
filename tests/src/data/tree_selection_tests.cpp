#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/data/tree_model.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/data/selection_model.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/ui/style_pass.hpp"
#include <vector>

namespace
{
    void testTreeSelectionCollapseKeepsMarks()
    {
        gran::data::TreeModel tree;
        auto a = tree.append(tree.root(), gran::data::Record{{"name", "A"}});
        tree.append(a, gran::data::Record{{"name", "a1"}});
        tree.append(a, gran::data::Record{{"name", "a2"}});
        tree.append(a, gran::data::Record{{"name", "a3"}});
        tree.append(tree.root(), gran::data::Record{{"name", "B"}});

        gran::data::TreeList rows(tree);
        rows.expand(rows.rowOf(a));

        gran::data::SelectionMode mode = gran::data::SelectionMode::EXTENDED;
        gran::data::SelectionModel selection(rows, mode);

        selection.activate(1, false, false); // a1
        selection.activate(3, true, false); // a3

        rows.collapse(0); // collapse A
        TEST_ASSERT(selection.count() == 0);
        TEST_ASSERT(selection.current() == 0); // parent

        rows.expand(0); // expand A
        TEST_ASSERT(selection.isSelected(1)); // a1
        TEST_ASSERT(selection.isSelected(3)); // a3
        TEST_ASSERT(!selection.isSelected(2)); // a2
        TEST_ASSERT(selection.count() == 2);
    }

    void testTreeSelectionClearDropsHidden()
    {
        gran::data::TreeModel tree;
        auto a = tree.append(tree.root(), gran::data::Record{{"name", "A"}});
        tree.append(a, gran::data::Record{{"name", "a1"}});
        tree.append(a, gran::data::Record{{"name", "a2"}});
        tree.append(a, gran::data::Record{{"name", "a3"}});
        tree.append(tree.root(), gran::data::Record{{"name", "B"}});

        gran::data::TreeList rows(tree);
        rows.expand(rows.rowOf(a));

        gran::data::SelectionMode mode = gran::data::SelectionMode::EXTENDED;
        gran::data::SelectionModel selection(rows, mode);

        selection.activate(2, false, false); // a2
        rows.collapse(0); // collapse A
        selection.clear();
        rows.expand(0); // expand A
        TEST_ASSERT(selection.count() == 0);
    }

    void testTreeSelectionClickDropsHidden()
    {
        gran::data::TreeModel tree;
        auto a = tree.append(tree.root(), gran::data::Record{{"name", "A"}});
        tree.append(a, gran::data::Record{{"name", "a1"}});
        tree.append(a, gran::data::Record{{"name", "a2"}});
        tree.append(a, gran::data::Record{{"name", "a3"}});
        tree.append(tree.root(), gran::data::Record{{"name", "B"}});

        gran::data::TreeList rows(tree);
        rows.expand(rows.rowOf(a));

        gran::data::SelectionMode mode = gran::data::SelectionMode::EXTENDED;
        gran::data::SelectionModel selection(rows, mode);

        selection.activate(2, false, false); // a2
        rows.collapse(0); // collapse A
        selection.activate(1, false, false); // b (now 1)
        rows.expand(0); // expand A
        TEST_ASSERT(selection.isSelected(4)); // b (now 4)
        TEST_ASSERT(!selection.isSelected(2)); // a2 (2)
        TEST_ASSERT(selection.count() == 1);
    }

    void testTreeSelectionCtrlShiftArrow()
    {
        auto doc = gran::markup::loadMarkupString(R"XML(<Window><Resources>
            <Data name="rows"><Item n="0"/><Item n="1"/><Item n="2"/><Item n="3"/></Data>
            <Selection name="sel" model="rows"/></Resources>
            <Stack id="list" items="rows" selection="sel"><ItemTemplate><Text>{n}</Text></ItemTemplate></Stack></Window>)XML",
            gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        lists.adoptData(doc.data);
        lists.bind(*doc.root);
        const gran::ui::Node& list = *gran::ui::findNode(*doc.root, "list");
        gran::data::SelectionModel& selection = *lists.selection("sel");

        lists.pressItem(list, *list.children[0], false, false); // pressItem строки 0
        lists.pressItem(list, *list.children[2], true, false); // pressItem строки 2 с control=true
        lists.keyItem(list, gran::binding::ListKey::DOWN, true, true); // keyItem(list, ListKey::DOWN, true, true)

        TEST_ASSERT(selection.selected() == std::vector<std::size_t>({0, 2, 3}));
        TEST_ASSERT(selection.current() == 3);
    }

    // Подсветка копий после удаления записи над выделенной остаётся на той же записи (ручная проверка Т-9:
    // выделение подписано раньше привязки и отмечало копии по старым позициям)
    // The copy highlight after removing a record above the selected one stays on the same record (manual check T-9:
    // the selection is subscribed before the binding and marked the copies by the old positions)
    void testSelectionHighlightAfterRemoval()
    {
        auto doc = gran::markup::loadMarkupString(R"XML(<Window><Resources>
            <Data name="rows"><Item n="0"/><Item n="1"/><Item n="2"/><Item n="3"/></Data>
            <Selection name="sel" model="rows"/></Resources>
            <Stack id="list" items="rows" selection="sel"><ItemTemplate><Text>{n}</Text></ItemTemplate></Stack></Window>)XML",
            gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        lists.adoptData(doc.data);
        lists.bind(*doc.root);
        const gran::ui::Node& list = *gran::ui::findNode(*doc.root, "list");
        std::uint16_t bit = gran::style::stateBit(gran::style::State::SELECTED);

        lists.pressItem(list, *list.children[3], false, false);
        lists.model("rows").remove(1);
        TEST_ASSERT(lists.selection("sel")->selected() == std::vector<std::size_t>({2}));
        TEST_ASSERT(list.children.size() == 3);
        TEST_ASSERT((list.children[2]->stateMask & bit) != 0);
        TEST_ASSERT((list.children[1]->stateMask & bit) == 0);
    }
}

namespace suites
{
    void registerTreeSelectionTests(test::TestRunner& runner)
    {
        runner.add("tree selection: collapse keeps marks", testTreeSelectionCollapseKeepsMarks);
        runner.add("tree selection: clear drops hidden", testTreeSelectionClearDropsHidden);
        runner.add("tree selection: click drops hidden", testTreeSelectionClickDropsHidden);
        runner.add("tree selection: ctrl shift arrow", testTreeSelectionCtrlShiftArrow);
        runner.add("tree selection: highlight after removal", testSelectionHighlightAfterRemoval);
    }
}
