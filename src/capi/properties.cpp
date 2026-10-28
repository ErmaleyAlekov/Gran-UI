#include "gran/capi/internal/bridge.hpp"

// Выходная строка принадлежит вызывающему, а не дереву UI.
// The output string belongs to the caller, not to the UI tree.
gran_status gran_app_read(gran_app* app, const char* target, const char* property, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(target != nullptr && property != nullptr && result != nullptr);
    *result = gran::capi::copyString(gran::app::readProperty(gran::app::requireNode(app->application, target), property));
    GRAN_CAPI_END
}

// Один переход через ABI для любого числа операций; ядро поднимает dirty к корню.
// A single ABI crossing for any number of operations; the core propagates dirty flags to the root.
gran_status gran_app_apply(gran_app* app, const gran_update* updates, size_t count)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(count == 0 || updates != nullptr);
    for (size_t index = 0; index < count; ++index)
    {
        const gran_update& update = updates[index];
        gran::capi::require(update.target != nullptr && update.property != nullptr && update.value != nullptr);
        gran::app::writeProperty(app->application, gran::app::requireNode(app->application, update.target), update.property, update.value);
    }
    GRAN_CAPI_END
}

// Общий диспетчер команд не дублируется в Python, Java и C#.
// The shared command dispatcher is not duplicated in Python, Java or C#.
gran_status gran_app_command(gran_app* app, const char* command, const char* target, const char* argument)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(command != nullptr);
    gran::app::executeCommand(app->application, command, gran::capi::text(target), gran::capi::text(argument));
    GRAN_CAPI_END
}
