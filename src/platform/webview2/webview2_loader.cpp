#include "gran/platform/webview2/internal/webview2_host.hpp"

#include <shlobj.h>
#include <algorithm>

#include "gran/utils/file_system.hpp"

namespace gran::platform::webview2
{
    namespace
    {
        // Одна ссылка COM до выхода потока окна / One COM reference until the window thread exits
        struct Apartment
        {
            bool entered = false;

            ~Apartment()
            {
                if (entered)
                    CoUninitialize();
            }
        };

        bool enterApartment() noexcept
        {
            thread_local Apartment apartment;
            if (!apartment.entered)
                apartment.entered = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            return apartment.entered;
        }

        // Получить полный путь даже при длине больше MAX_PATH / Read the full path even beyond MAX_PATH
        std::string programStem()
        {
            std::wstring path(MAX_PATH, L'\0');
            for (;;)
            {
                const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
                if (length == 0)
                    return {};
                if (length < path.size())
                    return fs::fileStem(fs::fromWidePath(path.data(), length));
                if (path.size() >= 32768)
                    return {};
                path.resize(std::min<std::size_t>(32768, path.size() * 2));
            }
        }

        // Загрузчик рядом с программой или в System32 / The loader next to the program or in System32
        CreateEnvironmentFunction loadFactory() noexcept
        {
            HMODULE module = LoadLibraryExW(L"WebView2Loader.dll", nullptr,
                LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);

            if (module == nullptr)
                return nullptr;
            return reinterpret_cast<CreateEnvironmentFunction>(
                reinterpret_cast<void*>(GetProcAddress(module, "CreateCoreWebView2EnvironmentWithOptions")));
        }
    }

    // Один раз на процесс; модуль загрузчика не выгружается (среда браузера живёт до конца) / Once per process; the loader module is not unloaded (the browser environment lives until the end)
    CreateEnvironmentFunction environmentFactory() noexcept
    {
        static const CreateEnvironmentFunction FACTORY = loadFactory();

        if (FACTORY == nullptr || !enterApartment())
            return nullptr;
        return FACTORY;
    }

    // Рядом с программой писать нельзя (Program Files), поэтому — локальные данные пользователя
    // Writing next to the program is not allowed (Program Files), therefore — the user local data
    std::wstring browserDataFolder()
    {
        wchar_t* local = nullptr;
        const std::string stem = programStem();
        std::string folder;

        if (stem.empty())
            return {};
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local)))
        {
            CoTaskMemFree(local);
            return std::wstring();
        }
        folder = fs::fromWidePath(local, wcslen(local));
        CoTaskMemFree(local);
        folder = fs::joinPath(fs::joinPath(folder, "GranUI"), stem);
        return fs::toWidePath(fs::joinPath(folder, "WebView2"));
    }
}
