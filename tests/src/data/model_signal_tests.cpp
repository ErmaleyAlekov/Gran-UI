#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/data/list_model.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/data/tree_model.hpp"

#include <memory>
#include <string>
#include <vector>

namespace
{
    using Events = std::vector<std::string>;

    // Наблюдатель списка с журналом; moved — по умолчанию (удаление + вставка) / A list observer with a log; moved is the default (a removal + an insertion)
    class ListLog : public gran::data::ListObserver
    {
    public:
        void inserted(std::size_t index, std::size_t count) override
        {
            events.push_back("ins " + std::to_string(index) + " " + std::to_string(count));
        }

        void removed(std::size_t index, std::size_t count) override
        {
            events.push_back("rem " + std::to_string(index) + " " + std::to_string(count));
        }

        void changed(std::size_t index) override
        {
            events.push_back("chg " + std::to_string(index));
        }

        Events events;
    };

    // Тот же журнал, но перемещение записывается одной строкой / The same log, but a move is written as one line
    class MoveLog : public ListLog
    {
    public:
        void moved(std::size_t from, std::size_t count, std::size_t to) override
        {
            events.push_back("mov " + std::to_string(from) + " " + std::to_string(count) + " " + std::to_string(to));
        }
    };

    // Наблюдатель дерева: только удаления (место, где были узлы) / A tree observer: only the removals (the place where the nodes were)
    class TreeLog : public gran::data::TreeObserver
    {
    public:
        void inserted(gran::data::NodeId, std::size_t, std::size_t) override {}
        void moved(gran::data::NodeId, gran::data::NodeId, std::size_t) override {}
        void changed(gran::data::NodeId) override {}

        void removed(gran::data::NodeId parent, std::size_t row, std::size_t count) override
        {
            removals.push_back(std::to_string(parent) + " " + std::to_string(row) + " " + std::to_string(count));
        }

        Events removals;
    };

    // Подгрузчик ленивого узла: один ребёнок «Б1» / A lazy node fetcher: one child "Б1"
    class OneChild : public gran::data::TreeFetcher
    {
    public:
        void fetchChildren(gran::data::TreeModel& model, gran::data::NodeId node) override
        {
            model.append(node, gran::data::Record{{"name", "Б1"}});
        }
    };

    // Пакетная вставка — одно уведомление; перемещение — moved или удаление и вставка; правка записи — changed / A batch insertion is one notification; a move is moved or a removal and an insertion; a record edit is changed
    void testListSignals()
    {
        gran::data::ListModel model;
        MoveLog moves;
        ListLog plain;

        model.subscribe(&moves);
        model.subscribe(&plain);
        model.insertAll(0, {gran::data::Record{{"n", "a"}}, gran::data::Record{{"n", "b"}}, gran::data::Record{{"n", "c"}}});
        model.move(0, 1, 2);
        TEST_ASSERT(model.at(0).at("n") == "b" && model.at(1).at("n") == "c" && model.at(2).at("n") == "a");
        model.setRecord(1, gran::data::Record{{"n", "x"}});
        TEST_ASSERT((moves.events == Events{"ins 0 3", "mov 0 1 2", "chg 1"}));
        TEST_ASSERT((plain.events == Events{"ins 0 3", "rem 0 1", "ins 2 1", "chg 1"}));
        model.remove(0, 2);
        TEST_ASSERT(moves.events.back() == "rem 0 2" && model.size() == 1);
    }

    // Узлы дерева: места, родители, удаление поддерева, перенос в своё поддерево — ошибка / Tree nodes: the places, the parents, a subtree removal, a move into its own subtree is an error
    void testTreeSignals()
    {
        gran::data::TreeModel tree;
        TreeLog log;
        gran::data::NodeId first = tree.append(tree.root(), gran::data::Record{{"name", "А"}});
        gran::data::NodeId second = tree.append(tree.root(), gran::data::Record{{"name", "Б"}});
        gran::data::NodeId grandchild = tree.append(first, gran::data::Record{{"name", "А1"}});

        tree.subscribe(&log);
        TEST_ASSERT(tree.childCount(tree.root()) == 2 && tree.row(second) == 1 && tree.parent(second) == tree.root());
        TEST_ASSERT_THROWS(tree.move(first, grandchild, 0), gran::GranException);
        TEST_ASSERT_THROWS(tree.record(999), gran::GranException);
        tree.remove(first);
        TEST_ASSERT(!tree.contains(first) && !tree.contains(grandchild) && tree.row(second) == 0);
        TEST_ASSERT((log.removals == Events{std::to_string(tree.root()) + " 0 1"}));
        tree.set(second, "name", "Б");
        TEST_ASSERT(log.removals.size() == 1);
    }

    // Плоское дерево: раскрытие вставляет строки детей, свёртка убирает, глубина верхнего уровня — 0 / A flat tree: an expansion inserts the child rows, a collapse removes them, the top level depth is 0
    void testTreeListRows()
    {
        gran::data::TreeModel tree;
        gran::data::NodeId a = tree.append(tree.root(), gran::data::Record{{"name", "А"}});
        tree.append(a, gran::data::Record{{"name", "А1"}});
        tree.append(a, gran::data::Record{{"name", "А2"}});
        tree.append(tree.root(), gran::data::Record{{"name", "Б"}});
        gran::data::TreeList rows(tree);
        ListLog log;

        rows.subscribe(&log);
        TEST_ASSERT(rows.size() == 2 && rows.depth(0) == 0 && rows.hasChildren(0) && !rows.expanded(0));
        rows.expand(0);
        TEST_ASSERT(rows.size() == 4 && rows.depth(1) == 1 && rows.at(2).at("name") == "А2" && rows.expanded(0));
        rows.collapse(0);
        TEST_ASSERT(rows.size() == 2);
        TEST_ASSERT((log.events == Events{"ins 1 2", "chg 0", "rem 1 2", "chg 0"}));
        rows.expandToDepth(1);
        TEST_ASSERT(rows.size() == 4);
    }

    // Ленивый узел: раскрытие зовёт подгрузчик, ребёнок появляется строкой, признак снимается / A lazy node: the expansion calls the fetcher, the child appears as a row, the flag is dropped
    void testLazyNode()
    {
        gran::data::TreeModel tree;
        gran::data::NodeId b = tree.append(tree.root(), gran::data::Record{{"name", "Б"}});
        gran::data::TreeList rows(tree);

        tree.setLazy(b, true);
        tree.setFetcher(std::make_shared<OneChild>());
        TEST_ASSERT(rows.hasChildren(0) && tree.hasChildren(b) && tree.childCount(b) == 0);
        rows.expand(rows.rowOf(b));
        TEST_ASSERT(!tree.lazy(b) && rows.size() == 2 && rows.at(1).at("name") == "Б1" && rows.depth(1) == 1);
    }
}

namespace suites
{
    void registerModelSignalTests(test::TestRunner& runner)
    {
        runner.add("models: list signals", testListSignals);
        runner.add("models: tree signals", testTreeSignals);
        runner.add("models: tree list rows", testTreeListRows);
        runner.add("models: lazy node", testLazyNode);
    }
}
