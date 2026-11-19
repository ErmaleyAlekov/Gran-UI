#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/data/column_filter.hpp"
#include "gran/data/list_model.hpp"
#include "gran/data/sort_filter_proxy.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/data/tree_model.hpp"
#include <string>
#include <vector>

namespace
{
    void testColumnFilterParse()
    {
        TEST_ASSERT(gran::data::parseCondition({"name", "СБОР", false}).op == gran::data::FilterOperator::CONTAINS);
        TEST_ASSERT(gran::data::parseCondition({"sum", ">10", true}).op == gran::data::FilterOperator::GREATER);
        TEST_ASSERT(gran::data::parseCondition({"sum", ">=14", true}).op == gran::data::FilterOperator::GREATER_EQUAL);
        TEST_ASSERT(gran::data::parseCondition({"sum", "<=5", true}).op == gran::data::FilterOperator::LESS_EQUAL);
        TEST_ASSERT(gran::data::parseCondition({"sum", "=0", true}).op == gran::data::FilterOperator::EQUAL);
        TEST_ASSERT(gran::data::parseCondition({"sum", "<>3", true}).op == gran::data::FilterOperator::NOT_EQUAL);
        TEST_ASSERT(gran::data::parseCondition({"sum", " > 10 ", true}).op == gran::data::FilterOperator::GREATER);
        TEST_ASSERT(gran::data::parseCondition({"sum", ">10", false}).op == gran::data::FilterOperator::CONTAINS);
        TEST_ASSERT(gran::data::parseCondition({"name", "> abc", true}).op == gran::data::FilterOperator::CONTAINS);
    }

    void testColumnFilterConditionMatches()
    {
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"name", "СБОР", false}), "Цех сборки"));
        TEST_ASSERT(!gran::data::conditionMatches(gran::data::parseCondition({"name", "СБОР", false}), "Склад"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", ">10", true}), "14"));
        TEST_ASSERT(!gran::data::conditionMatches(gran::data::parseCondition({"sum", ">10", true}), "7"));
        TEST_ASSERT(!gran::data::conditionMatches(gran::data::parseCondition({"sum", ">10", true}), ""));
        TEST_ASSERT(!gran::data::conditionMatches(gran::data::parseCondition({"sum", ">10", true}), "abc"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", ">=14", true}), "14"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", "<=5", true}), "5"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", "=0", true}), "0"));
        TEST_ASSERT(!gran::data::conditionMatches(gran::data::parseCondition({"sum", "<>3", true}), "3"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", "<>3", true}), "4"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", " > 10 ", true}), "14"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"sum", ">10", false}), "a>10b"));
        TEST_ASSERT(!gran::data::conditionMatches(gran::data::parseCondition({"name", "> abc", true}), "Участок 1"));
        TEST_ASSERT(gran::data::conditionMatches(gran::data::parseCondition({"name", "> abc", true}), "x > ABC y"));
    }

    void testColumnFilterOverSortFilterProxy()
    {
        gran::data::ListModel source;
        source.append({{"name", "Анна"}, {"sum", "5"}});
        source.append({{"name", "Борис"}, {"sum", "30"}});
        source.append({{"name", "Вера"}, {"sum", "10"}});
        source.append({{"name", "Глеб"}, {"sum", "40"}});

        gran::data::SortFilterProxy proxy(source);
        auto filter = std::make_shared<gran::data::ColumnFilter>(nullptr, nullptr);
        filter->setConditions({{"name", "Р", false}, {"sum", ">=10", true}});
        proxy.setFilter(filter);

        TEST_ASSERT(proxy.size() == 2);
        TEST_ASSERT(proxy.at(0).at("name") == "Борис");
        TEST_ASSERT(proxy.at(1).at("name") == "Вера");

        filter->setConditions({});
        TEST_ASSERT(filter->empty());
    }

    void testColumnFilterWithBaseFilter()
    {
        class RejectFirst : public gran::data::RecordFilter
        {
        public:
            bool accepts(const gran::data::ListSource& source, std::size_t index) const override
            {
                return index != 0;
            }
        };

        gran::data::ListModel source;
        source.append({{"name", "Анна"}, {"sum", "5"}});
        source.append({{"name", "Борис"}, {"sum", "30"}});
        source.append({{"name", "Вера"}, {"sum", "10"}});
        source.append({{"name", "Глеб"}, {"sum", "40"}});

        gran::data::SortFilterProxy proxy(source);
        auto baseFilter = std::make_shared<RejectFirst>();
        auto filter = std::make_shared<gran::data::ColumnFilter>(baseFilter, nullptr);
        filter->setConditions({{"sum", ">0", true}});
        proxy.setFilter(filter);

        TEST_ASSERT(proxy.size() == 3);
        TEST_ASSERT(proxy.at(0).at("name") == "Борис");
        TEST_ASSERT(proxy.at(1).at("name") == "Вера");
        TEST_ASSERT(proxy.at(2).at("name") == "Глеб");

        TEST_ASSERT(filter->baseRecords() == baseFilter);
    }

    void testColumnFilterOverTreeList()
    {
        gran::data::TreeModel tree;
        auto root = tree.root();
        gran::data::NodeId production = tree.append(root, {{"name", "Производство"}});
        gran::data::NodeId assembly = tree.append(production, {{"name", "Цех сборки"}});
        tree.append(assembly, {{"name", "Участок 1"}});
        tree.append(assembly, {{"name", "Участок 2"}});
        tree.append(root, {{"name", "Склад"}});

        gran::data::TreeList rows(tree);
        auto filter = std::make_shared<gran::data::ColumnFilter>(nullptr, nullptr);
        filter->setConditions({{"name", "участок", false}});
        rows.setFilter(filter);

        TEST_ASSERT(rows.size() == 4);
        TEST_ASSERT(tree.record(rows.node(0)).at("name") == "Производство");
    }
}

namespace suites
{
    void registerColumnFilterTests(test::TestRunner& runner)
    {
        runner.add("column filter: parse", testColumnFilterParse);
        runner.add("column filter: condition matches", testColumnFilterConditionMatches);
        runner.add("column filter: over sort filter proxy", testColumnFilterOverSortFilterProxy);
        runner.add("column filter: with base filter", testColumnFilterWithBaseFilter);
        runner.add("column filter: over tree list", testColumnFilterOverTreeList);
    }
}
