#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/data/template_text.hpp"
#include "gran/data/list_model.hpp"
#include "gran/data/internal/list_notify.hpp"
#include "gran/core/gran_exception.hpp"
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    class ListObserverStub : public gran::data::ListObserver
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

        std::vector<std::string> events;
    };

    void testExpandTemplate()
    {
        gran::data::Record record = {{"name", "Alice"}, {"age", "30"}};

        TEST_ASSERT(gran::data::expandTemplate("Hello, {name}!", record) == "Hello, Alice!");
        TEST_ASSERT(gran::data::expandTemplate("You are {age} years old.", record) == "You are 30 years old.");
        TEST_ASSERT(gran::data::expandTemplate("{{name}}", record) == "{name}");
        TEST_ASSERT(gran::data::expandTemplate("}}", record) == "}");
        TEST_ASSERT(gran::data::expandTemplate("{unknown}", record) == "");
        TEST_ASSERT(gran::data::expandTemplate("{name} {name}", record) == "Alice Alice");
        TEST_ASSERT(gran::data::expandTemplate("{name{", record) == "{name{");
        gran::data::Record braces = {{"name", "{age}"}, {"age", "30"}};
        TEST_ASSERT(gran::data::expandTemplate("{name}", braces) == "{age}");
    }

    void testHasPlaceholders()
    {
        TEST_ASSERT(gran::data::hasPlaceholders("a{b}c") == true);
        TEST_ASSERT(gran::data::hasPlaceholders("{{x}}") == false);
        TEST_ASSERT(gran::data::hasPlaceholders("abc") == false);
        TEST_ASSERT(gran::data::hasPlaceholders("a{b") == false);
    }

    // Наблюдатель, отписывающий другого при первой вставке / An observer unsubscribing another one on the first insertion
    class UnsubscribingObserver : public ListObserverStub
    {
    public:
        void inserted(std::size_t index, std::size_t count) override
        {
            ListObserverStub::inserted(index, count);
            if (model != nullptr)
                model->unsubscribe(victim);
            if (model != nullptr && resubscribe)
            {
                model->subscribe(victim);
                resubscribe = false;
                model = nullptr;
            }
        }

        gran::data::ListModel* model = nullptr;
        gran::data::ListObserver* victim = nullptr;
        bool resubscribe = false;
    };

    // Ожидаемый журнал событий / The expected event log
    bool sameEvents(const ListObserverStub& observer, const std::vector<std::string>& expected)
    {
        return observer.events == expected;
    }

    void testListModel()
    {
        gran::data::ListModel model;
        ListObserverStub observer;

        model.subscribe(&observer);
        model.append({{"name", "Alice"}});
        model.insert(0, {{"name", "Bob"}});
        TEST_ASSERT(sameEvents(observer, {"ins 0 1", "ins 0 1"}));
        TEST_ASSERT(model.size() == 2);
        TEST_ASSERT(model.at(0).at("name") == "Bob");
        model.set(1, "name", "Charlie");
        model.set(1, "name", "Charlie");
        TEST_ASSERT(sameEvents(observer, {"ins 0 1", "ins 0 1", "chg 1"}));
        TEST_ASSERT(model.at(1).at("name") == "Charlie");
        TEST_ASSERT_THROWS(model.at(5), gran::GranException);
        TEST_ASSERT_THROWS(model.remove(5), gran::GranException);
        TEST_ASSERT_THROWS(model.insert(3, {}), gran::GranException);
        model.remove(0);
        model.clear();
        model.clear();
        TEST_ASSERT(sameEvents(observer, {"ins 0 1", "ins 0 1", "chg 1", "rem 0 1", "rem 0 1"}));
        TEST_ASSERT(model.size() == 0);
    }

    void testListRangeBoundaries()
    {
        gran::data::ListModel model;
        ListObserverStub observer;
        model.append({{"id", "a"}});
        model.append({{"id", "b"}});
        model.subscribe(&observer);
        TEST_ASSERT_THROWS(model.remove(1, 2), gran::GranException);
        TEST_ASSERT_THROWS(model.move(1, 2, 0), gran::GranException);
        TEST_ASSERT_THROWS(model.move(0, 2, 1), gran::GranException);
        TEST_ASSERT(model.size() == 2 && model.at(0).at("id") == "a" && model.at(1).at("id") == "b");
        TEST_ASSERT(observer.events.empty());
        model.remove(1, 1);
        TEST_ASSERT(sameEvents(observer, {"rem 1 1"}));
    }

    void testMoveMappingBoundaries()
    {
        using gran::data::internal::moveMapping;
        const std::vector<std::size_t> identity{0, 1, 2, 3};
        const std::vector<std::size_t> forward{0, 2, 3, 1};
        const std::vector<std::size_t> backward{0, 3, 1, 2};
        TEST_ASSERT(moveMapping(4, 1, 2, 2) == forward);
        TEST_ASSERT(moveMapping(4, 2, 2, 1) == backward);
        TEST_ASSERT(moveMapping(4, 3, 2, 0) == identity);
        TEST_ASSERT(moveMapping(4, 0, 2, 3) == identity);
        TEST_ASSERT(moveMapping(4, 4, 0, 0) == identity);
    }

    void testObserverRangeBoundary()
    {
        const auto last = std::numeric_limits<std::size_t>::max();
        ListObserverStub observer;
        observer.changedRange(last, 0);
        TEST_ASSERT(observer.events.empty());
        observer.changedRange(last, 1);
        TEST_ASSERT(sameEvents(observer, {"chg " + std::to_string(last)}));
        TEST_ASSERT_THROWS(observer.changedRange(last, 2), gran::GranException);
        TEST_ASSERT(observer.events.size() == 1);
    }

    void testListModelObserverUnsubscribe()
    {
        gran::data::ListModel model;
        ListObserverStub first;
        ListObserverStub second;

        model.subscribe(&first);
        model.subscribe(&second);
        model.subscribe(&second);
        model.append({{"name", "Alice"}});
        TEST_ASSERT(sameEvents(second, {"ins 0 1"}));
        model.unsubscribe(&first);
        model.append({{"name", "Bob"}});
        TEST_ASSERT(sameEvents(first, {"ins 0 1"}));
        TEST_ASSERT(sameEvents(second, {"ins 0 1", "ins 1 1"}));
        TEST_ASSERT(!model.subscribed(&first));
    }

    void testListModelObserverNestedUnsubscribe()
    {
        gran::data::ListModel model;
        UnsubscribingObserver first;
        ListObserverStub second;

        first.model = &model;
        first.victim = &second;
        model.subscribe(&first);
        model.subscribe(&second);
        model.append({{"name", "Alice"}});
        TEST_ASSERT(sameEvents(first, {"ins 0 1"}));
        TEST_ASSERT(second.events.empty());
        TEST_ASSERT(!model.subscribed(&second));
    }

    void testListModelObserverResubscribe()
    {
        gran::data::ListModel model;
        UnsubscribingObserver first;
        ListObserverStub second;

        first.model = &model;
        first.victim = &second;
        first.resubscribe = true;
        model.subscribe(&first);
        model.subscribe(&second);
        model.append({{"name", "Alice"}});
        TEST_ASSERT(second.events.empty());
        TEST_ASSERT(model.subscribed(&second));
        model.append({{"name", "Bob"}});
        TEST_ASSERT(sameEvents(second, {"ins 1 1"}));
    }
}

namespace suites
{
    void registerDataListTests(test::TestRunner& runner)
    {
        runner.add("data: expand template", testExpandTemplate);
        runner.add("data: has placeholders", testHasPlaceholders);
        runner.add("data: list model", testListModel);
        runner.add("data: list range boundaries", testListRangeBoundaries);
        runner.add("data: move mapping boundaries", testMoveMappingBoundaries);
        runner.add("data: observer range boundary", testObserverRangeBoundary);
        runner.add("data: list model observer unsubscribe", testListModelObserverUnsubscribe);
        runner.add("data: list model observer nested unsubscribe", testListModelObserverNestedUnsubscribe);
        runner.add("data: list model observer resubscribe", testListModelObserverResubscribe);
    }
}
