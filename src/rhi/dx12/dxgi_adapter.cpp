#include "gran/rhi/dx12/internal/dx12_internal.hpp"

#include <cstdio>

namespace gran::rhi::dx12
{
    namespace
    {
        // Имя из UTF-16 в UTF-8 / The name from UTF-16 into UTF-8
        std::string toUtf8(const wchar_t* text)
        {
            int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);

            if (size <= 0)
                return std::string();
            std::string result(static_cast<std::size_t>(size), '\0');
            WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], size, nullptr, nullptr);
            result.resize(result.size() - 1);
            return result;
        }

        // Версия драйвера a.b.c.d из 64-битного числа CheckInterfaceSupport / The driver version a.b.c.d from the 64-bit CheckInterfaceSupport number
        std::string driverVersion(IDXGIAdapter* adapter)
        {
            LARGE_INTEGER version{};
            char text[48];

            if (FAILED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version)))
                return std::string();
            std::snprintf(text, sizeof(text), "%u.%u.%u.%u", static_cast<unsigned>(HIWORD(version.HighPart)),
                static_cast<unsigned>(LOWORD(version.HighPart)), static_cast<unsigned>(HIWORD(version.LowPart)),
                static_cast<unsigned>(LOWORD(version.LowPart)));
            return text;
        }
    }

    /*
    ** Сведения для журнала, а не для работы: ошибки запросов не бросают исключений, поле просто
    ** остаётся пустым. Признак программного адаптера — из DXGI_ADAPTER_DESC1 (WARP).
    **
    ** The information is for the log, not for work: query errors throw no exceptions, the field simply
    ** stays empty. The software adapter flag comes from DXGI_ADAPTER_DESC1 (WARP).
    */
    AdapterInfo describeDxgiAdapter(IDXGIAdapter* adapter, const char* api)
    {
        AdapterInfo info;
        DXGI_ADAPTER_DESC1 desc{};
        ComRef<IDXGIAdapter1> adapter1;

        info.api = api;
        if (adapter == nullptr)
            return info;
        if (SUCCEEDED(adapter->QueryInterface(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(adapter1.put())))
            && SUCCEEDED(adapter1->GetDesc1(&desc)))
        {
            info.name = toUtf8(desc.Description);
            info.software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        }
        info.driver = driverVersion(adapter);
        return info;
    }
}
