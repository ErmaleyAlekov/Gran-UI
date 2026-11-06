#include "gran/log/internal/log_state.hpp"

#include <cstdio>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace gran::log::internal
{
#if defined(_WIN32)
    namespace
    {
        /*
        ** UTF-8 → UTF-16 для WriteConsoleW. Неверные байты заменяются знаком U+FFFD, а не обрывают строку.
        **
        ** UTF-8 → UTF-16 for WriteConsoleW. Invalid bytes are replaced with U+FFFD and do not cut the line.
        */
        std::wstring toUtf16(std::string_view text)
        {
            int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
            std::wstring wide;

            if (count <= 0)
                return wide;
            wide.resize(static_cast<std::size_t>(count));
            MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), count);
            return wide;
        }
    }
#endif

    /*
    ** Консоль Windows показывает байты в своей кодовой странице (обычно 866), и UTF-8 через std::cerr
    ** превращался в «кракозябры» (найдено в ручной проверке Т-2, 2026-11-06). Поэтому в консоль текст идёт
    ** через WriteConsoleW в UTF-16 — верно при любой кодовой странице. Если stderr перенаправлен в файл или
    ** канал, байты UTF-8 пишутся как есть: так их ждут читающие программы. На Linux и macOS терминалы в UTF-8.
    **
    ** The Windows console shows bytes in its code page (usually 866), and UTF-8 through std::cerr turned into
    ** garbage (found in the manual check T-2, 2026-11-06). Therefore the text goes to the console through
    ** WriteConsoleW in UTF-16 — correct with any code page. If stderr is redirected to a file or a pipe, the
    ** UTF-8 bytes are written as is: that is what the reading programs expect. On Linux and macOS terminals are UTF-8.
    */
    void writeStderr(std::string_view line) noexcept
    {
#if defined(_WIN32)
        HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode = 0;

        if (handle != nullptr && handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode) != 0)
        {
            try
            {
                std::wstring wide = toUtf16(line);
                DWORD written = 0;

                // Ошибку вывода в консоль некуда сообщить: журнал в файле уже есть / A console output error has nowhere to go: the file log exists anyway
                WriteConsoleW(handle, wide.data(), static_cast<DWORD>(wide.size()), &written, nullptr);
                return;
            }
            catch (...)
            {
                // Нет памяти на перевод — пишем байты как есть / No memory for the conversion — write the bytes as is
            }
        }
#endif
        std::fwrite(line.data(), 1, line.size(), stderr);
        std::fflush(stderr);
    }
}
