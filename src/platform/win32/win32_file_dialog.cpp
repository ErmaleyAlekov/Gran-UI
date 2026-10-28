#include "gran/platform/win32/internal/file_dialog_win32.hpp"
#include "gran/rhi/dx12/internal/com_ref.hpp"

namespace gran::platform::win32
{
    namespace
    {
        /*
        ** Диалоги оболочки работают только в однопоточном апартаменте COM. Поток окна уже в нём
        ** (так же входят доступность и WebView2); повторный вход — S_FALSE. Многопоточный апартамент
        ** (RPC_E_CHANGED_MODE) — ошибка. CoUninitialize не вызывается: апартамент живёт до конца потока.
        **
        ** The shell dialogs work only in a single-threaded COM apartment. The window thread is already in it
        ** (accessibility and WebView2 enter it the same way); a repeated entry gives S_FALSE. A multithreaded apartment
        ** (RPC_E_CHANGED_MODE) is an error. CoUninitialize is not called: the apartment lives until the thread ends.
        */
        void enterApartment()
        {
            HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

            if (result != S_FALSE)
                checkDialog(result);
        }

        // Объект диалога: сохранение — FileSaveDialog, остальное — FileOpenDialog / The dialog object: saving — FileSaveDialog, the rest — FileOpenDialog
        rhi::dx12::ComRef<IFileDialog> createDialog(FileDialogKind kind)
        {
            rhi::dx12::ComRef<IFileDialog> dialog;

            if (kind == FileDialogKind::SAVE)
                checkDialog(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog,
                    reinterpret_cast<void**>(dialog.put())));
            else
                checkDialog(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog,
                    reinterpret_cast<void**>(dialog.put())));
            return dialog;
        }

        // Несколько файлов — через IFileOpenDialog::GetResults / Several files — through IFileOpenDialog::GetResults
        std::vector<std::string> multipleResults(IFileDialog& dialog)
        {
            rhi::dx12::ComRef<IFileOpenDialog> open;
            rhi::dx12::ComRef<IShellItemArray> items;
            std::vector<std::string> paths;
            DWORD count = 0;

            checkDialog(dialog.QueryInterface(IID_IFileOpenDialog, reinterpret_cast<void**>(open.put())));
            checkDialog(open->GetResults(items.put()));
            checkDialog(items->GetCount(&count));
            for (DWORD index = 0; index < count; ++index)
            {
                rhi::dx12::ComRef<IShellItem> item;

                checkDialog(items->GetItemAt(index, item.put()));
                paths.push_back(shellItemPath(*item.get()));
            }
            return paths;
        }

        // Выбор пользователя: один путь или несколько / The user choice: one path or several
        std::vector<std::string> collectResults(IFileDialog& dialog, FileDialogKind kind)
        {
            rhi::dx12::ComRef<IShellItem> item;

            if (kind == FileDialogKind::OPEN_MULTIPLE)
                return multipleResults(dialog);
            checkDialog(dialog.GetResult(item.put()));
            return std::vector<std::string>{shellItemPath(*item.get())};
        }
    }
}

namespace gran::platform
{
    /*
    ** Диалог настраивается и показывается модально над окном-владельцем (оно отключается на это
    ** время). Отказ пользователя (ERROR_CANCELLED) — пустой результат, а не ошибка.
    **
    ** The dialog is set up and shown modally over the owner window (it is disabled for that
    ** time). The user cancelling (ERROR_CANCELLED) is an empty result, not an error.
    */
    std::vector<std::string> showFileDialog(FileDialogKind kind, const FileDialogOptions& options, Window* owner)
    {
        HWND parent = nullptr;

        win32::enterApartment();
        rhi::dx12::ComRef<IFileDialog> dialog = win32::createDialog(kind);
        win32::applyDialogOptions(*dialog.get(), kind, options);
        if (owner != nullptr)
            parent = static_cast<HWND>(owner->nativeHandle());
        HRESULT result = dialog->Show(parent);
        if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return std::vector<std::string>{};
        win32::checkDialog(result);
        return win32::collectResults(*dialog.get(), kind);
    }
}
