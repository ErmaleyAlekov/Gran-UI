#include "gran/platform/win32/internal/file_dialog_win32.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/rhi/dx12/internal/com_ref.hpp"

#include <cstdio>
#include <vector>

namespace gran::platform::win32
{
    namespace
    {
        /*
        ** Типы файлов: строки UTF-16 живут в векторе до вызова SetFileTypes (диалог копирует их).
        ** Первый тип выбран сразу.
        **
        ** The file types: the UTF-16 strings live in a vector until the SetFileTypes call (the dialog copies them).
        ** The first type is selected at once.
        */
        void setFilters(IFileDialog& dialog, const std::vector<FileFilter>& filters)
        {
            std::vector<std::wstring> texts;
            std::vector<COMDLG_FILTERSPEC> specs;

            if (filters.empty())
                return;
            texts.reserve(filters.size() * 2);
            for (const FileFilter& filter : filters)
            {
                texts.push_back(toWide(filter.name));
                texts.push_back(toWide(filter.patterns));
            }
            for (std::size_t index = 0; index < filters.size(); ++index)
                specs.push_back(COMDLG_FILTERSPEC{texts[index * 2].c_str(), texts[index * 2 + 1].c_str()});
            checkDialog(dialog.SetFileTypes(static_cast<UINT>(specs.size()), specs.data()));
            checkDialog(dialog.SetFileTypeIndex(1));
        }

        // Начальная папка; несуществующая пропускается — диалог откроется там, где решит ОС
        // The initial folder; a missing one is skipped — the dialog opens where the OS decides
        void setFolder(IFileDialog& dialog, const std::string& folder)
        {
            rhi::dx12::ComRef<IShellItem> item;

            if (folder.empty())
                return;
            if (FAILED(SHCreateItemFromParsingName(toWide(folder).c_str(), nullptr, IID_IShellItem,
                    reinterpret_cast<void**>(item.put()))))
                return;
            checkDialog(dialog.SetFolder(item.get()));
        }
    }

    // Код HRESULT шестнадцатеричным числом в тексте ошибки из каталога / The HRESULT code as a hex number in the catalog error text
    void checkDialog(HRESULT result)
    {
        char code[12] = {};

        if (SUCCEEDED(result))
            return;
        std::snprintf(code, sizeof(code), "%08lX", static_cast<unsigned long>(result));
        throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_FILE_DIALOG_FAILED, {code}));
    }

    /*
    ** Флаги поверх умолчаний диалога: только файлы файловой системы; открытие — файл должен
    ** существовать, несколько — FOS_ALLOWMULTISELECT, папка — FOS_PICKFOLDERS; сохранение —
    ** вопрос о замене существующего файла.
    **
    ** The flags over the dialog defaults: file system files only; opening — the file must
    ** exist, several — FOS_ALLOWMULTISELECT, a folder — FOS_PICKFOLDERS; saving —
    ** asking before replacing an existing file.
    */
    void applyDialogOptions(IFileDialog& dialog, FileDialogKind kind, const FileDialogOptions& options)
    {
        FILEOPENDIALOGOPTIONS flags = 0;

        checkDialog(dialog.GetOptions(&flags));
        flags |= FOS_FORCEFILESYSTEM;
        if (kind == FileDialogKind::OPEN || kind == FileDialogKind::OPEN_MULTIPLE)
            flags |= FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST;
        if (kind == FileDialogKind::OPEN_MULTIPLE)
            flags |= FOS_ALLOWMULTISELECT;
        if (kind == FileDialogKind::FOLDER)
            flags |= FOS_PICKFOLDERS;
        if (kind == FileDialogKind::SAVE)
            flags |= FOS_OVERWRITEPROMPT;
        checkDialog(dialog.SetOptions(flags));
        if (!options.title.empty())
            checkDialog(dialog.SetTitle(toWide(options.title).c_str()));
        if (!options.fileName.empty())
            checkDialog(dialog.SetFileName(toWide(options.fileName).c_str()));
        if (!options.defaultExtension.empty())
            checkDialog(dialog.SetDefaultExtension(toWide(options.defaultExtension).c_str()));
        if (kind != FileDialogKind::FOLDER)
            setFilters(dialog, options.filters);
        setFolder(dialog, options.folder);
    }

    // Строку пути выделяет оболочка — освобождается через CoTaskMemFree / The shell allocates the path string — it is freed with CoTaskMemFree
    std::string shellItemPath(IShellItem& item)
    {
        PWSTR wide = nullptr;
        std::string path;

        checkDialog(item.GetDisplayName(SIGDN_FILESYSPATH, &wide));
        try
        {
            path = toUtf8(wide);
        }
        catch (...)
        {
            CoTaskMemFree(wide);
            throw;
        }
        CoTaskMemFree(wide);
        return path;
    }
}
