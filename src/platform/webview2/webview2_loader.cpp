#include "gran/platform/webview2/internal/webview2_host.hpp"

#include <shlobj.h>

#include <filesystem>

namespace gran::platform::webview2
{
    namespace
    {
        /*
        ** COM в однопоточном апартаменте — требование WebView2 к потоку окна. Обратный CoUninitialize
        ** не вызывается: браузер освобождает свои объекты асинхронно до конца процесса.
        ** RPC_E_CHANGED_MODE (поток уже в многопоточном апартаменте) — браузер недоступен.
        **
        ** COM in a single-threaded apartment is a WebView2 requirement for the window thread. The matching
        ** CoUninitialize is not called: the browser releases its objects asynchronously until the process ends.
        ** RPC_E_CHANGED_MODE (the thread is already in a multithreaded apartment) — the browser is unavailable.
        */
        bool enterApartment() noexcept
        {
            HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

            return SUCCEEDED(result);
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
        wchar_t program[MAX_PATH] = {};
        std::filesystem::path folder;

        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local)))
        {
            CoTaskMemFree(local);
            return std::wstring();
        }
        folder = local;
        CoTaskMemFree(local);
        GetModuleFileNameW(nullptr, program, MAX_PATH);
        folder /= L"GranUI";
        folder /= std::filesystem::path(program).stem();
        folder /= L"WebView2";
        return folder.wstring();
    }
}
