#include "platform/fatal_error.hpp"

#include "gran/platform/win32/win32_window.hpp"

namespace chat
{
    // Окно без владельца (окна чата уже/ещё нет); исключение перевода строки не должно уронить вывод ошибки / A box without an owner (there is no chat window yet/anymore); a conversion exception must not break the error output
    void showFatalError(const std::string& text) noexcept
    {
        try
        {
            std::wstring wide = gran::platform::win32::toWide(text);
            MessageBoxW(nullptr, wide.c_str(), L"Gran Chat", MB_OK | MB_ICONERROR);
        }
        catch (...)
        {
            // Нехватка памяти при переводе: показать хотя бы общий заголовок / Out of memory while converting: show at least the common title
            MessageBoxW(nullptr, L"Gran Chat", L"Gran Chat", MB_OK | MB_ICONERROR);
        }
    }
}
