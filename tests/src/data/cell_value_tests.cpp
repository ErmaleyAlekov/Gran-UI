#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/data/cell_value.hpp"
#include "gran/data/record_tracker.hpp"
#include "gran/data/list_model.hpp"
#include <string>
#include <vector>

namespace
{
    void testNormalizeValue()
    {
        std::string text;

        text = "12";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "12");

        text = " -3,5 ";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "-3.5");

        text = "+7";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "7");

        text = "1e3";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "1e3");

        text = ".5";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == ".5");

        text = "abc";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "abc");

        text = "1.2.3";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "1.2.3");

        text = "1e";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "1e");

        text = "--1";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "--1");

        text = "inf";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "inf");

        text = "";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "");

        text = "   ";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::NUMBER, text) && text == "");

        text = "2024-02-29";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::DATE, text) && text == "2024-02-29");

        text = "2023-02-29";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::DATE, text) && text == "2023-02-29");

        text = "5.3.2024";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::DATE, text) && text == "2024-03-05");

        text = "2024-13-01";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::DATE, text) && text == "2024-13-01");

        text = "24-01-01";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::DATE, text) && text == "24-01-01");

        text = "Yes";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::BOOLEAN, text) && text == "true");

        text = "0";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::BOOLEAN, text) && text == "false");

        text = "maybe";
        TEST_ASSERT(!gran::data::normalizeValue(gran::data::ColumnType::BOOLEAN, text) && text == "maybe");

        text = "  a ";
        TEST_ASSERT(gran::data::normalizeValue(gran::data::ColumnType::TEXT, text) && text == "  a ");
    }

    void testValueTrue()
    {
        TEST_ASSERT(gran::data::valueTrue("TRUE"));
        TEST_ASSERT(gran::data::valueTrue(" 1 "));
        TEST_ASSERT(!gran::data::valueTrue("no"));
    }

    void testRecordTracker()
    {
        gran::data::ListModel model;
        for (std::size_t i = 0; i < 5; ++i)
        {
            model.append({{"n", std::to_string(i)}});
        }

        gran::data::RecordTracker tracker(model, 2);

        model.insert(0, {{"n", "0"}});
        TEST_ASSERT(tracker.index() == 3);

        model.remove(0);
        TEST_ASSERT(tracker.index() == 2);

        model.move(0, 1, 3);
        TEST_ASSERT(tracker.index() == 1);
        TEST_ASSERT(model.at(tracker.index()).at("n") == "2");

        model.remove(tracker.index());
        TEST_ASSERT(!tracker.alive());

        gran::data::RecordTracker tracker2(model, 10);
        TEST_ASSERT(!tracker2.alive());
    }
}

namespace suites
{
    void registerCellValueTests(test::TestRunner& runner)
    {
        runner.add("cell value: normalize value", testNormalizeValue);
        runner.add("cell value: value true", testValueTrue);
        runner.add("record tracker: basic functionality", testRecordTracker);
    }
}
