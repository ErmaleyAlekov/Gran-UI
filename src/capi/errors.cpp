#include "gran/capi/internal/bridge.hpp"
#include <cstdio>

namespace
{
    // Буфер не выделяет память при обработке bad_alloc; ошибки независимы между потоками.
    // The buffer allocates no memory while handling bad_alloc; errors are thread local.
    thread_local char errorText[4096] = {};
}

namespace gran::capi
{
    // Вызывается только внутри catch; последний текст сохраняется до следующей ошибки.
    // Called only inside catch; the latest text remains until the next error.
    gran_status capture() noexcept
    {
        try { throw; }
        catch (const Failure& error)
        {
            std::snprintf(errorText, sizeof(errorText), "%s", error.message.c_str());
            return error.status;
        }
        catch (const GranException& error)
        {
            std::snprintf(errorText, sizeof(errorText), "%s", error.what());
            if (error.getCode() == ErrorCode::INVALID_ARGUMENT) return GRAN_INVALID_ARGUMENT;
            return GRAN_ERROR;
        }
        catch (const std::exception& error)
        {
            std::snprintf(errorText, sizeof(errorText), "%s", error.what());
            return GRAN_ERROR;
        }
        catch (...)
        {
            // Текст берётся прямо из каталога, без выделений во время сбоя.
            // Read the catalog directly without allocating while handling a failure.
            const char* fallback = i18n::findTemplate(i18n::currentCatalog(), i18n::Message::CAPI_UNKNOWN);
            if (fallback != nullptr) std::snprintf(errorText, sizeof(errorText), "%s", fallback);
            return GRAN_ERROR;
        }
    }
}

// ABI и ошибки доступны без приложения.
// ABI and errors are available without an application.
uint32_t gran_abi_version(void) { return GRAN_ABI_VERSION; }
const char* gran_last_error(void) { return errorText; }
void gran_string_free(char* value) { delete[] value; }
