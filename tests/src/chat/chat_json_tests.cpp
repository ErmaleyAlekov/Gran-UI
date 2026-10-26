#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "common/json_value.hpp"
#include "common/chat_errors.hpp"
#include <string>

namespace
{
    void testJsonTypesAndMembers()
    {
        auto json = chat::parseJson(R"({"a":[1,true,null,"x"],"b":{}})");
        TEST_ASSERT(json.type == chat::JsonType::OBJECT);
        TEST_ASSERT(json.members.size() == 2);

        const auto* memberA = chat::jsonMember(json, "a");
        TEST_ASSERT(memberA != nullptr);
        TEST_ASSERT(memberA->type == chat::JsonType::ARRAY);
        TEST_ASSERT(memberA->items.size() == 4);
        TEST_ASSERT(memberA->items[0].type == chat::JsonType::NUMBER);
        TEST_ASSERT(memberA->items[0].number == 1.0);
        TEST_ASSERT(memberA->items[1].type == chat::JsonType::BOOLEAN);
        TEST_ASSERT(memberA->items[1].boolean == true);
        TEST_ASSERT(memberA->items[2].type == chat::JsonType::NULL_VALUE);
        TEST_ASSERT(memberA->items[3].type == chat::JsonType::STRING);
        TEST_ASSERT(memberA->items[3].text == "x");

        const auto* memberB = chat::jsonMember(json, "b");
        TEST_ASSERT(memberB != nullptr);
        TEST_ASSERT(memberB->type == chat::JsonType::OBJECT);
        TEST_ASSERT(memberB->members.empty());
    }

    void testJsonStringEscaping()
    {
        std::string out;
        chat::writeJsonString(out, "a\"b\n");
        TEST_ASSERT(out == "\"a\\\"b\\n\"");

        auto parsed = chat::parseJson(out);
        TEST_ASSERT(parsed.type == chat::JsonType::STRING);
        TEST_ASSERT(parsed.text == "a\"b\n");

        TEST_ASSERT(chat::parseJson(R"("\n\t\"\\\/")").text == "\n\t\"\\/");
        TEST_ASSERT(chat::parseJson(R"("Aп")").text == "A\xD0\xBF");
        TEST_ASSERT(chat::parseJson(R"("😀")").text == "\xF0\x9F\x98\x80");
    }

    void testJsonNumbers()
    {
        auto json = chat::parseJson(R"([-0.5, 1e3, 0, 12])");
        TEST_ASSERT(json.type == chat::JsonType::ARRAY);
        TEST_ASSERT(json.items.size() == 4);
        TEST_ASSERT_NEAR(json.items[0].number, -0.5, 1e-9);
        TEST_ASSERT_NEAR(json.items[1].number, 1000.0, 1e-9);
        TEST_ASSERT_NEAR(json.items[2].number, 0.0, 1e-9);
        TEST_ASSERT_NEAR(json.items[3].number, 12.0, 1e-9);
    }

    void testJsonErrors()
    {
        TEST_ASSERT_THROWS(chat::parseJson(""), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson(R"({"a":1,})"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson(R"([1 2])"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson("01"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson(".5"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson("abc"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson(R"("\uDC00")"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson(R"("abc)"), chat::JsonException);
        TEST_ASSERT_THROWS(chat::parseJson("true x"), chat::JsonException);

        std::string deepArray;
        for (int i = 0; i < chat::JSON_MAX_DEPTH + 1; ++i) {
            deepArray += "[";
        }
        for (int i = 0; i < chat::JSON_MAX_DEPTH + 1; ++i) {
            deepArray += "]";
        }
        TEST_ASSERT_THROWS(chat::parseJson(deepArray), chat::JsonException);
    }
}

namespace suites
{
    void registerChatJsonTests(test::TestRunner& runner)
    {
        runner.add("chat: Json types and members", testJsonTypesAndMembers);
        runner.add("chat: Json string escaping", testJsonStringEscaping);
        runner.add("chat: Json numbers", testJsonNumbers);
        runner.add("chat: Json errors", testJsonErrors);
    }
}
