#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/capi/gran.h"
#include "gran/capi/internal/bridge.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/app/commands.hpp"
#include <thread>
#include <cstring>

namespace
{
    void testNullArguments()
    {
        gran_app* app = nullptr;
        gran_status status = gran_app_create(nullptr, nullptr);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);
        TEST_ASSERT(app == nullptr);
        gran_options options{};
        options.size = 1;
        TEST_ASSERT(gran_app_create(&options, &app) == GRAN_INVALID_ARGUMENT);
        TEST_ASSERT(app == nullptr);
        TEST_ASSERT(gran_abi_version() == 1);

        status = gran_app_destroy(app);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_load(app, "<Window></Window>", 0);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_run(app, nullptr);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_on_action(app, "action", nullptr, nullptr);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_on_message(app, "message", nullptr, nullptr);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_post(app, nullptr, nullptr);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        gran_timer timer = {1.0, 0, nullptr, nullptr};
        uint64_t timerId = 0;
        status = gran_app_timer(app, &timer, &timerId);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_clear_timer(app, timerId);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        char* result = nullptr;
        status = gran_app_read(app, "target", "property", &result);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);
        TEST_ASSERT(result == nullptr);

        gran_update update = {"target", "property", "value"};
        status = gran_app_apply(app, &update, 1);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_app_command(app, "command", "target", "argument");
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        size_t count = 0;
        status = gran_model_count(app, "model", &count);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_model_read(app, "model", 0, "field", &result);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);
        TEST_ASSERT(result == nullptr);

        gran_record record = {0, nullptr, 0};
        status = gran_model_insert(app, "model", &record);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_model_set(app, "model", &record);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_model_remove(app, "model", 0);
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);

        status = gran_model_clear(app, "model");
        TEST_ASSERT(status == GRAN_INVALID_ARGUMENT);
    }

    void testUTF8TextRoundtrip()
    {
        gran_app* app = nullptr;
        TEST_ASSERT(gran_app_create(nullptr, &app) == GRAN_OK);
        TEST_ASSERT(app != nullptr);

        const char* xml = R"(<Window><Stack><Text id="text">Привет, мир!</Text><CheckBox id="check"/><Slider id="slider" min="0" max="10"/></Stack></Window>)";
        TEST_ASSERT(gran_app_load(app, xml, 0) == GRAN_OK);

        char* result = nullptr;
        gran_app_read(app, "text", "text", &result);
        TEST_ASSERT(result != nullptr);
        TEST_ASSERT_EQ(result, "Привет, мир!");
        gran_string_free(result);
        gran_update updates[] = {{"text", "text", "🌲 & <текст>"}, {"check", "checked", "true"}, {"slider", "value", "50"}};
        TEST_ASSERT(gran_app_apply(app, updates, 3) == GRAN_OK);
        TEST_ASSERT(app->application.find("check")->content.checked);
        TEST_ASSERT(app->application.find("slider")->content.value == 10);
        TEST_ASSERT(gran_app_load(app, "<invalid/>", 0) != GRAN_OK);
        TEST_ASSERT(gran_app_read(app, "text", "text", &result) == GRAN_OK);
        TEST_ASSERT_EQ(result, "🌲 & <текст>");
        gran_string_free(result);

        gran_app_destroy(app);
    }

    void testModelOperations()
    {
        gran_app* app = nullptr;
        TEST_ASSERT(gran_app_create(nullptr, &app) == GRAN_OK);
        TEST_ASSERT(app != nullptr);

        const char* xml = R"(<Window><Stack id="root"><Stack id="list" items="rows"><ItemTemplate><Text>{name}</Text></ItemTemplate></Stack></Stack></Window>)";
        TEST_ASSERT(gran_app_load(app, xml, 0) == GRAN_OK);

        gran_record record = {0, nullptr, 0};
        gran_field fields[] = {{"name", "Item 1"}};
        record.fields = fields;
        record.count = 1;

        TEST_ASSERT(gran_model_insert(app, "rows", &record) == GRAN_OK);
        TEST_ASSERT(gran_model_insert(app, "rows", &record) == GRAN_OK);
        TEST_ASSERT(app->application.find("list")->children.size() == 2);
        fields[0].value = "<&🌲>";
        TEST_ASSERT(gran_model_set(app, "rows", &record) == GRAN_OK);
        TEST_ASSERT_EQ(app->application.find("list")->children[0]->content.text, "<&🌲>");

        size_t count = 0;
        TEST_ASSERT(gran_model_count(app, "rows", &count) == GRAN_OK);
        TEST_ASSERT(count == 2);

        char* result = nullptr;
        TEST_ASSERT(gran_model_read(app, "rows", 0, "name", &result) == GRAN_OK);
        TEST_ASSERT(result != nullptr);
        TEST_ASSERT_EQ(result, "<&🌲>");
        gran_string_free(result);

        TEST_ASSERT(gran_model_remove(app, "rows", 0) == GRAN_OK);
        TEST_ASSERT(gran_model_count(app, "rows", &count) == GRAN_OK);
        TEST_ASSERT(count == 1);

        TEST_ASSERT(gran_model_clear(app, "rows") == GRAN_OK);
        TEST_ASSERT(gran_model_count(app, "rows", &count) == GRAN_OK);
        TEST_ASSERT(count == 0);
        TEST_ASSERT(app->application.find("list")->children.empty());
        TEST_ASSERT(gran_app_command(app, "appendMarkup", "root", "<Text id=\"added\">ok</Text>") == GRAN_OK);
        gran::Application::State state;
        state.owner = &app->application;
        gran::app::executeBuiltin(state, "gran:remove:added", *app->application.find("root"));
        TEST_ASSERT(gran_app_read(app, "added", "text", &result) == GRAN_INVALID_ARGUMENT);

        gran_app_destroy(app);
    }

    void testInvalidIdProperty()
    {
        gran_app* app = nullptr;
        TEST_ASSERT(gran_app_create(nullptr, &app) == GRAN_OK);
        TEST_ASSERT(app != nullptr);

        const char* xml = R"(<Window><Text id="text">Hello</Text></Window>)";
        TEST_ASSERT(gran_app_load(app, xml, 0) == GRAN_OK);

        char* result = nullptr;
        gran_status status = gran_app_read(app, "invalid", "text", &result);
        TEST_ASSERT(status != GRAN_OK);
        TEST_ASSERT(result == nullptr);

        status = gran_app_read(app, "text", "invalid", &result);
        TEST_ASSERT(status != GRAN_OK);
        TEST_ASSERT(result == nullptr);

        gran_status threadStatus = GRAN_OK;
        gran_status postStatus = GRAN_ERROR;
        std::string threadError;
        char* output = nullptr;
        // Захват нужен только для передачи результата проверки между потоками.
        // The capture is only needed to transfer the check result between threads.
        std::thread worker([&] {
            threadStatus = gran_app_read(app, "text", "text", &output);
            threadError = gran_last_error();
            postStatus = gran_app_post(app, [](void*) {}, nullptr);
        });
        worker.join();
        TEST_ASSERT(threadStatus == GRAN_WRONG_THREAD);
        TEST_ASSERT(output == nullptr && !threadError.empty());
        TEST_ASSERT(postStatus == GRAN_OK);
        TEST_ASSERT(gran_app_load(app, "<Window><Button text=\"x\" onClick=\"gran:close\"/></Window>", 0) == GRAN_OK);
        TEST_ASSERT(gran_app_load(app, "<Window><Button text=\"x\" onClick=\"gran:noSuchCommand\"/></Window>", 0) != GRAN_OK);
        gran_app_destroy(app);
    }
}

namespace suites
{
    void registerCapiTests(test::TestRunner& runner)
    {
        runner.add("null arguments", testNullArguments);
        runner.add("UTF8 text roundtrip", testUTF8TextRoundtrip);
        runner.add("model operations", testModelOperations);
        runner.add("invalid id/property", testInvalidIdProperty);
    }
}
