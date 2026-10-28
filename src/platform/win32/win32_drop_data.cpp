#include "gran/platform/win32/internal/drop_target.hpp"

#include <shellapi.h>

namespace gran::platform::win32
{
    namespace
    {
        // Формат «список файлов» в глобальной памяти / The "file list" format in global memory
        FORMATETC fileListFormat() noexcept
        {
            FORMATETC format{};

            format.cfFormat = CF_HDROP;
            format.dwAspect = DVASPECT_CONTENT;
            format.lindex = -1;
            format.tymed = TYMED_HGLOBAL;
            return format;
        }

        // Пути из заблокированного HDROP; исключения (память, UTF-8) ловит вызывающий / The paths from a locked HDROP; the caller catches exceptions (memory, UTF-8)
        void collectPaths(HDROP drop, std::vector<std::string>& paths)
        {
            UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);

            for (UINT index = 0; index < count; ++index)
            {
                UINT length = DragQueryFileW(drop, index, nullptr, 0);
                std::wstring path(length + 1, L'\0');

                DragQueryFileW(drop, index, &path[0], length + 1);
                path.resize(length);
                paths.push_back(toUtf8(path.c_str()));
            }
        }
    }

    // Источник предлагает CF_HDROP / The source offers CF_HDROP
    bool hasFiles(IDataObject* data) noexcept
    {
        FORMATETC format = fileListFormat();

        return data != nullptr && data->QueryGetData(&format) == S_OK;
    }

    /*
    ** Данные CF_HDROP: память блокируется на время чтения и отдаётся обратно (ReleaseStgMedium)
    ** при любом исходе. Пустой список — тоже неудача: бросать нечего.
    **
    ** The CF_HDROP data: the memory is locked while reading and given back (ReleaseStgMedium)
    ** whatever happens. An empty list is a failure too: there is nothing to drop.
    */
    bool readDroppedFiles(IDataObject* data, std::vector<std::string>& paths) noexcept
    {
        FORMATETC format = fileListFormat();
        STGMEDIUM medium{};
        bool read = false;

        if (data == nullptr || FAILED(data->GetData(&format, &medium)))
            return false;
        HDROP drop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
        if (drop != nullptr)
        {
            try
            {
                collectPaths(drop, paths);
                read = !paths.empty();
            }
            catch (...)
            {
                read = false;
            }
            GlobalUnlock(medium.hGlobal);
        }
        ReleaseStgMedium(&medium);
        return read;
    }
}
