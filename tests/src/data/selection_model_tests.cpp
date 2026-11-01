#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/data/list_model.hpp"
#include "gran/data/selection_model.hpp"
#include <vector>

namespace
{
    void testSelectionModelExtended()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::EXTENDED);

        selection.activate(1, false, false);
        TEST_ASSERT(selection.current() == 1);
        TEST_ASSERT(selection.count() == 1);
        TEST_ASSERT(selection.isSelected(1));

        selection.activate(3, false, true);
        TEST_ASSERT(selection.current() == 3);
        TEST_ASSERT(selection.anchor() == 1);
        TEST_ASSERT(selection.count() == 3);
        TEST_ASSERT(selection.isSelected(1));
        TEST_ASSERT(selection.isSelected(2));
        TEST_ASSERT(selection.isSelected(3));

        selection.activate(0, true, false);
        TEST_ASSERT(selection.current() == 0);
        TEST_ASSERT(selection.count() == 4);
        TEST_ASSERT(selection.isSelected(0));
        TEST_ASSERT(selection.isSelected(1));
        TEST_ASSERT(selection.isSelected(2));
        TEST_ASSERT(selection.isSelected(3));
    }

    void testSelectionModelSingle()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::SINGLE);

        selection.select(1);
        TEST_ASSERT(selection.count() == 1);
        TEST_ASSERT(selection.isSelected(1));

        selection.select(3);
        TEST_ASSERT(selection.count() == 1);
        TEST_ASSERT(selection.isSelected(3));
        TEST_ASSERT(!selection.isSelected(1));
    }

    void testSelectionModelInsert()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::EXTENDED);

        selection.activate(3, false, false);
        TEST_ASSERT(selection.current() == 3);
        TEST_ASSERT(selection.count() == 1);

        model.insert(0, {{"n", "new"}});
        TEST_ASSERT(selection.current() == 4);
        TEST_ASSERT(selection.count() == 1);
        TEST_ASSERT(selection.isSelected(4));
    }

    void testSelectionModelRemove()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::EXTENDED);

        selection.activate(2, false, false);
        TEST_ASSERT(selection.current() == 2);
        TEST_ASSERT(selection.count() == 1);

        model.remove(2);
        TEST_ASSERT(selection.current() == 2);
        TEST_ASSERT(selection.count() == 0);

        selection.activate(3, false, false);
        TEST_ASSERT(selection.current() == 3);
        TEST_ASSERT(selection.count() == 1);

        model.remove(3);
        TEST_ASSERT(selection.current() == 2);
        TEST_ASSERT(selection.count() == 0);
    }

    void testSelectionModelMove()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::EXTENDED);

        selection.activate(0, false, false);
        TEST_ASSERT(selection.current() == 0);
        TEST_ASSERT(selection.count() == 1);

        model.move(0, 1, 4);
        TEST_ASSERT(selection.current() == 4);
        TEST_ASSERT(selection.count() == 1);
        TEST_ASSERT(selection.isSelected(4));
    }

    void testSelectionModelMulti()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::MULTI);

        selection.selectAll();
        TEST_ASSERT(selection.count() == 5);
        for (int i = 0; i < 5; ++i)
            TEST_ASSERT(selection.isSelected(i));

        selection.clear();
        TEST_ASSERT(selection.count() == 0);
        for (int i = 0; i < 5; ++i)
            TEST_ASSERT(!selection.isSelected(i));
    }

    void testSelectionModelNone()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::NONE);

        selection.select(1);
        TEST_ASSERT(selection.count() == 0);
        TEST_ASSERT(!selection.isSelected(1));
    }

    void testSelectionModelObserver()
    {
        gran::data::ListModel model;
        for (int i = 0; i < 5; ++i)
            model.append({{"n", std::to_string(i)}});
        gran::data::SelectionModel selection(model, gran::data::SelectionMode::EXTENDED);

        class Observer : public gran::data::SelectionObserver
        {
        public:
            void selectionChanged() override
            {
                ++count;
            }

            int count = 0;
        };

        Observer observer;
        selection.subscribe(&observer);

        selection.select(1);
        TEST_ASSERT(observer.count == 1);

        selection.select(2);
        TEST_ASSERT(observer.count == 2);

        selection.clear();
        TEST_ASSERT(observer.count == 3);
    }
}

namespace suites
{
    void registerSelectionModelTests(test::TestRunner& runner)
    {
        runner.add("selection: extended", testSelectionModelExtended);
        runner.add("selection: single", testSelectionModelSingle);
        runner.add("selection: insert", testSelectionModelInsert);
        runner.add("selection: remove", testSelectionModelRemove);
        runner.add("selection: move", testSelectionModelMove);
        runner.add("selection: multi", testSelectionModelMulti);
        runner.add("selection: none", testSelectionModelNone);
        runner.add("selection: observer", testSelectionModelObserver);
    }
}
