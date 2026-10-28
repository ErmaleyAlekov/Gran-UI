#include "gran/capi/internal/bridge.hpp"
#include <cstring>

namespace gran::capi
{
    // Ошибка аргумента переводится каталогом ядра.
    // Argument errors are translated by the core catalog.
    void require(bool condition)
    {
        if (!condition)
            throw Failure{GRAN_INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_ARGUMENT)};
    }

    // post проверяет только дескриптор; остальные операции принадлежат потоку UI.
    // post checks only the handle; all other operations belong to the UI thread.
    void checkApp(gran_app* app, bool ownerThread)
    {
        require(app != nullptr);
        if (ownerThread && app->thread != std::this_thread::get_id())
            throw Failure{GRAN_WRONG_THREAD, i18n::format(i18n::Message::CAPI_THREAD)};
    }

    // Необязательная строка имеет пустое значение.
    // An optional string has an empty value.
    const char* text(const char* value) noexcept
    {
        if (value == nullptr)
            return "";
        return value;
    }

    // Память освобождается тем же runtime через gran_string_free.
    // Memory is released by the same runtime through gran_string_free.
    char* copyString(const std::string& value)
    {
        auto result = std::make_unique<char[]>(value.size() + 1);
        std::memcpy(result.get(), value.c_str(), value.size() + 1);
        return result.release();
    }
}
