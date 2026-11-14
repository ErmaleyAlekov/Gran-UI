#include "gran/platform/window.hpp"
#include "gran/platform/file_dialog.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

/*
** Сборка без платформы ОС (NATIVE=0): окон ОС и системных диалогов нет. Так собирается ядро на macOS, пока нет
** платформы Cocoa (шаг M6), и ядро на Linux без X11 (серверы, контейнеры CI). Работает только безоконная
** платформа (AppOptions::headless, src/platform/headless/): её используют модульные тесты и AppDriver.
** Вызов функции окна ОС — понятная ошибка, а не неразрешённый символ при компоновке программы.
**
** A build without the OS platform (NATIVE=0): there are no OS windows and no system dialogs. The core is built so on
** macOS until the Cocoa platform exists (step M6), and on Linux without X11 (servers, CI containers). Only the
** windowless platform works (AppOptions::headless, src/platform/headless/): the unit tests and AppDriver use it.
** Calling an OS window function is a clear error, not an unresolved symbol when the program is linked.
*/
namespace gran::platform
{
    // Окно ОС в этой сборке не создаётся / An OS window is not created in this build
    std::unique_ptr<Window> createWindow(const WindowDesc& desc)
    {
        (void)desc;
        throw GranException(ErrorCode::PLATFORM,
            i18n::format(i18n::Message::PLATFORM_NOT_BUILT, {"platform::createWindow"}));
    }

    // Системного диалога файлов в этой сборке нет / There is no system file dialog in this build
    std::vector<std::string> showFileDialog(FileDialogKind kind, const FileDialogOptions& options, Window* owner)
    {
        (void)kind;
        (void)options;
        (void)owner;
        throw GranException(ErrorCode::PLATFORM,
            i18n::format(i18n::Message::PLATFORM_NOT_BUILT, {"platform::showFileDialog"}));
    }
}
