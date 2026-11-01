#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/data/list_model.hpp"
#include "gran/data/sort_filter_proxy.hpp"
#include "gran/core/gran_exception.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace
{
    void testSortKeysParsing()
    {
        gran::data::SortKeys keys = gran::data::parseSortKeys("client, sum:number desc, file:natural");
        TEST_ASSERT(keys.size() == 3);
        TEST_ASSERT(keys[0].field == "client");
        TEST_ASSERT(keys[0].type == gran::data::SortType::TEXT);
        TEST_ASSERT(!keys[0].descending);
        TEST_ASSERT(keys[1].field == "sum");
        TEST_ASSERT(keys[1].type == gran::data::SortType::NUMBER);
        TEST_ASSERT(keys[1].descending);
        TEST_ASSERT(keys[2].field == "file");
        TEST_ASSERT(keys[2].type == gran::data::SortType::NATURAL);
        TEST_ASSERT(!keys[2].descending);

        TEST_ASSERT(gran::data::parseSortKeys("").empty());

        TEST_ASSERT_THROWS(gran::data::parseSortKeys("a:bad"), gran::GranException);
        TEST_ASSERT_THROWS(gran::data::parseSortKeys("a b c"), gran::GranException);
    }

    void testCompareFunctions()
    {
        TEST_ASSERT(gran::data::compareNatural("file2", "file10") < 0);
        TEST_ASSERT(gran::data::compareText("абв", "АБГ") < 0);
        TEST_ASSERT(gran::data::compareNumber("x", "5") > 0);
    }

    void testSortFilterProxy()
    {
        gran::data::ListModel source;
        source.append({{"sum", "5"}});
        source.append({{"sum", "30"}});
        source.append({{"sum", "10"}});

        gran::data::SortFilterProxy proxy(source);
        proxy.setSort(gran::data::parseSortKeys("sum:number desc"));

        TEST_ASSERT(proxy.size() == 3);
        TEST_ASSERT(proxy.at(0).at("sum") == "30");
        TEST_ASSERT(proxy.at(1).at("sum") == "10");
        TEST_ASSERT(proxy.at(2).at("sum") == "5");
        TEST_ASSERT(proxy.sourceIndex(0) == 1);
        TEST_ASSERT(proxy.proxyIndex(1) == 0);

        source.set(2, "sum", "50");
        TEST_ASSERT(proxy.at(0).at("sum") == "50");
        TEST_ASSERT(proxy.at(1).at("sum") == "30");
        TEST_ASSERT(proxy.at(2).at("sum") == "5");

        source.insert(1, {{"sum", "20"}});
        TEST_ASSERT(proxy.at(2).at("sum") == "20" && proxy.at(3).at("sum") == "5");

        class DoneFilter : public gran::data::RecordFilter
        {
        public:
            bool accepts(const gran::data::ListSource& source, std::size_t index) const override
            {
                auto found = source.at(index).find("done");
                return found != source.at(index).end() && found->second == "true";
            }
        };

        proxy.setFilter(std::make_shared<DoneFilter>());
        TEST_ASSERT(proxy.size() == 0);

        source.set(0, "done", "true");
        TEST_ASSERT(proxy.size() == 1);
        TEST_ASSERT(proxy.at(0).at("sum") == "5");

        proxy.set(0, "note", "x");
        TEST_ASSERT(source.at(0).at("note") == "x");
    }
}

namespace suites
{
    void registerSortFilterTests(test::TestRunner& runner)
    {
        runner.add("sort: parse sort keys", testSortKeysParsing);
        runner.add("sort: compare functions", testCompareFunctions);
        runner.add("sort: sort filter proxy", testSortFilterProxy);
    }
}
