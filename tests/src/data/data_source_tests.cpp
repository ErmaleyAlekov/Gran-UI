#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/data/data_source.hpp"
#include "gran/data/json_reader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    // Файл во временном каталоге тестов / A file in the test temporary directory
    std::string writeFile(const std::string& name, const std::string& text)
    {
        std::filesystem::create_directories("scratchpad/models");
        std::string path = "scratchpad/models/" + name;
        std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
        return path;
    }

    // Лексемы и строки: escape-последовательности, суррогатная пара, числа текстом / Tokens and strings: escapes, a surrogate pair, numbers as text
    void testJsonReaderTokens()
    {
        std::istringstream input("{\"a\": [\"\\u0416\\uD83D\\uDE00\\n\", -1.5e3, true, null], \"b\": {}}");
        gran::data::JsonReader reader(input, "inline");

        TEST_ASSERT(reader.next() == gran::data::JsonToken::BEGIN_OBJECT);
        TEST_ASSERT(reader.next() == gran::data::JsonToken::NAME && reader.value() == "a");
        TEST_ASSERT(reader.next() == gran::data::JsonToken::BEGIN_ARRAY);
        TEST_ASSERT(reader.next() == gran::data::JsonToken::STRING && reader.value() == "Ж😀\n");
        TEST_ASSERT(reader.next() == gran::data::JsonToken::NUMBER && reader.value() == "-1.5e3");
        TEST_ASSERT(reader.next() == gran::data::JsonToken::BOOLEAN && reader.value() == "true");
        TEST_ASSERT(reader.next() == gran::data::JsonToken::NUL && reader.value().empty());
        TEST_ASSERT(reader.next() == gran::data::JsonToken::END_ARRAY);
        TEST_ASSERT(reader.next() == gran::data::JsonToken::NAME && reader.value() == "b");
        reader.skip(reader.next());
        TEST_ASSERT(reader.next() == gran::data::JsonToken::END_OBJECT);
        TEST_ASSERT(reader.next() == gran::data::JsonToken::END);
    }

    // Ошибка знает строку и столбец; висячая запятая и одинокий суррогат — ошибки / An error knows the line and the column; a trailing comma and a lone surrogate are errors
    void testJsonReaderErrors()
    {
        std::istringstream comma("{\"a\": [1,\n ]}");
        gran::data::JsonReader reader(comma, "bad.json");
        bool failed = false;

        try
        {
            while (reader.next() != gran::data::JsonToken::END)
                continue;
        }
        catch (const gran::DataException& error)
        {
            failed = error.getLine() == 2 && error.getColumn() == 2 && error.getSource() == "bad.json"
                && error.getCode() == gran::ErrorCode::DATA;
        }
        TEST_ASSERT(failed);
        std::istringstream surrogate("\"\\uDE00\"");
        gran::data::JsonReader lone(surrogate, "s.json");
        TEST_ASSERT_THROWS(lone.next(), gran::DataException);
        std::istringstream tail("[1] 2");
        gran::data::JsonReader extra(tail, "t.json");
        extra.skip(extra.next());
        TEST_ASSERT_THROWS(extra.next(), gran::DataException);
    }

    // JSON-список: путь select, поля Field по развёрнутым именам, порции по 2 / A JSON list: the select path, Field fields by unfolded names, portions of 2
    void testJsonListSource()
    {
        gran::data::SourceSpec spec;
        spec.path = writeFile("orders.json", "{\"meta\": {\"x\": [1, 2]}, \"data\": {\"items\": ["
            "{\"id\": 1, \"client\": {\"name\": \"Анна\", \"city\": \"Тверь\"}, \"tags\": [\"a\"]},"
            "{\"id\": 2, \"client\": {\"name\": \"Борис\"}}, {\"id\": 3}, {\"id\": 4}, {\"id\": 5}]}}");
        spec.select = "data.items";
        spec.fetch = 2;
        std::shared_ptr<gran::data::ListFetcher> fetcher = gran::data::openListSource(spec);
        std::vector<gran::data::Record> first = fetcher->next();

        TEST_ASSERT(first.size() == 2 && fetcher->hasMore());
        TEST_ASSERT(first[0].at("client.name") == "Анна" && first[0].at("client.city") == "Тверь");
        TEST_ASSERT(first[0].count("tags") == 0 && first[1].at("id") == "2");
        TEST_ASSERT(fetcher->next().size() == 2);
        TEST_ASSERT(fetcher->next().size() == 1 && !fetcher->hasMore());
        spec.fields = {{"name", "client.name"}, {"number", "id"}};
        spec.fetch = 0;
        std::vector<gran::data::Record> all = gran::data::openListSource(spec)->next();
        TEST_ASSERT(all.size() == 5 && all[0].size() == 2 && all[0].at("name") == "Анна" && all[4].at("name").empty());
    }

    // XML: select по шагам, поля XPath относительно записи, поля по умолчанию / XML: select by steps, XPath fields relative to the record, default fields
    void testXmlListSource()
    {
        gran::data::SourceSpec spec;
        spec.path = writeFile("books.xml", "<?xml version=\"1.0\"?><catalog><shelf><book id=\"1\"><title>Война</title>"
            "<line qty=\"2\"/><line qty=\"3\"/></book><book id=\"2\"><title>Мир</title></book></shelf>"
            "<book id=\"3\"><title>Вне полки</title></book></catalog>");
        spec.select = "/catalog/shelf/book";
        std::vector<gran::data::Record> plain = gran::data::openListSource(spec)->next();

        TEST_ASSERT(plain.size() == 2 && plain[0].at("id") == "1" && plain[0].at("title") == "Война");
        spec.select = "book";
        spec.fields = {{"name", "title"}, {"total", "sum(line/@qty)"}};
        std::vector<gran::data::Record> all = gran::data::openListSource(spec)->next();
        TEST_ASSERT(all.size() == 3 && all[0].at("total") == "5" && all[2].at("name") == "Вне полки");
    }

    // Ленивый XML: рабочий поток отдаёт порции по 2; уничтожение посреди файла не зависает / Lazy XML: the worker thread gives portions of 2; destroying in the middle of the file does not hang
    void testXmlLazyFetch()
    {
        std::string text = "<list>";
        gran::data::SourceSpec spec;

        for (int index = 0; index < 5; ++index)
            text += "<row n=\"" + std::to_string(index) + "\"/>";
        spec.path = writeFile("rows.xml", text + "</list>");
        spec.select = "/list/row";
        spec.fetch = 2;
        gran::data::ListModel model;
        gran::data::loadList(model, spec);
        TEST_ASSERT(model.size() == 2 && model.canFetchMore());
        model.fetchMore();
        model.fetchMore();
        TEST_ASSERT(model.size() == 5 && model.at(4).at("n") == "4");
        model.fetchMore();
        TEST_ASSERT(!model.canFetchMore());
        std::shared_ptr<gran::data::ListFetcher> partial = gran::data::openListSource(spec);
        TEST_ASSERT(partial->next().size() == 2);
        partial.reset();
    }

    // Деревья из XML (вложенные элементы) и JSON (член children) / Trees from XML (nested elements) and JSON (the children member)
    void testTreeSources()
    {
        gran::data::SourceSpec xml;
        xml.path = writeFile("org.xml", "<org><dept name=\"A\"><dept name=\"A1\"/><dept name=\"A2\"><dept name=\"A21\"/>"
            "</dept></dept><dept name=\"B\"/></org>");
        xml.select = "/org/dept";
        xml.children = "dept";
        std::vector<gran::data::TreeItem> nodes = gran::data::readTreeSource(xml);
        TEST_ASSERT(nodes.size() == 2 && nodes[0].children.size() == 2 && nodes[0].children[1].children[0].record.at("name") == "A21");
        gran::data::SourceSpec json;
        json.path = writeFile("org.json", "[{\"name\": \"A\", \"kids\": [{\"name\": \"A1\"}]}, {\"name\": \"B\"}]");
        json.children = "kids";
        nodes = gran::data::readTreeSource(json);
        TEST_ASSERT(nodes.size() == 2 && nodes[0].children.size() == 1 && nodes[0].record.count("kids") == 0);
        gran::data::TreeModel tree;
        gran::data::fillTree(tree, tree.root(), nodes);
        TEST_ASSERT(tree.childCount(tree.root()) == 2 && tree.record(tree.child(tree.child(tree.root(), 0), 0)).at("name") == "A1");
    }

    // Ошибки источников: нет файла, неверный путь, испорченный XML, нет пути в JSON / Source errors: no file, a wrong path, a damaged XML, no path in JSON
    void testSourceErrors()
    {
        gran::data::SourceSpec spec;
        spec.path = "scratchpad/models/missing.json";
        TEST_ASSERT_THROWS(gran::data::openListSource(spec), gran::GranException);
        spec.path = writeFile("bad.xml", "<list>\n<row></list>");
        spec.select = "/list/row";
        bool positioned = false;
        try
        {
            gran::data::openListSource(spec)->next();
        }
        catch (const gran::DataException& error)
        {
            positioned = error.getLine() == 2 && error.getMessage().find("bad.xml") != std::string::npos;
        }
        TEST_ASSERT(positioned);
        spec.select = "/list/row[1]";
        TEST_ASSERT_THROWS(gran::data::openListSource(spec), gran::GranException);
        spec.select = "/list/row";
        spec.fields = {{"x", "count("}};
        TEST_ASSERT_THROWS(gran::data::openListSource(spec), gran::GranException);
        gran::data::SourceSpec json;
        json.path = writeFile("plain.json", "{\"a\": 1}");
        json.select = "items";
        TEST_ASSERT_THROWS(gran::data::openListSource(json), gran::DataException);
    }
}

namespace suites
{
    void registerDataSourceTests(test::TestRunner& runner)
    {
        runner.add("sources: json reader tokens", testJsonReaderTokens);
        runner.add("sources: json reader errors", testJsonReaderErrors);
        runner.add("sources: json list", testJsonListSource);
        runner.add("sources: xml list", testXmlListSource);
        runner.add("sources: xml lazy fetch", testXmlLazyFetch);
        runner.add("sources: trees", testTreeSources);
        runner.add("sources: errors", testSourceErrors);
    }
}
