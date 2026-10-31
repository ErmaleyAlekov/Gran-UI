#include "messages.hpp"
#include "gran/core/i18n.hpp"
#include <cstring>
namespace editor
{
    // Каталог прикладных сообщений не изменяет общий каталог движка.
    // The application catalogue leaves the engine's shared catalogue untouched.
    namespace
    {
        static const char* english[] = {
            "Untitled-", "Ready", "Cannot open: expected UTF-8 text up to 4 MiB.",
            "Save failed. Your buffer is still open.", "Saved: ",
            "File changed on disk. Use Save As to keep both versions.",
            "Save changes before closing? ", "Close", "No document open",
            "Search in the active file", "Git integration is not connected yet.",
            "Build and debugger integration is not connected yet.",
            "Extension loading is not available yet.",
            "Gran Editor uses the modern dark theme. Settings persistence is planned.",
            "Local workspace. No account required.",
            "No language server connected; diagnostics are not available.",
            "Gran UI editor is ready. File operations appear in the status bar.",
            "An integrated shell is not connected yet. Use your system terminal.",
            "Ctrl+N  New file\nCtrl+O  Open file\nCtrl+S  Save\nCtrl+Shift+S  Save As\nCtrl+W  Close tab\nCtrl+Tab  Next tab\nCtrl+Shift+Tab  Previous tab\nCtrl+Shift+T  Reopen closed tab\nCtrl+F  Find\nCtrl+B  Toggle sidebar",
            "No match in the active file.", "Cannot read folder.", "No closed tabs to reopen."
        };
        static const char* russian[] = {
            "Без имени-", "Готово", "Не удалось открыть: нужен текст UTF-8 до 4 МиБ.",
            "Не удалось сохранить. Буфер остаётся открытым.", "Сохранено: ",
            "Файл изменился на диске. Используйте «Сохранить как», чтобы оставить обе версии.",
            "Сохранить изменения перед закрытием? ", "Закрыть", "Нет открытого документа",
            "Поиск в текущем файле", "Интеграция Git пока не подключена.",
            "Сборка и отладчик пока не подключены.", "Загрузка расширений пока недоступна.",
            "Gran Editor использует современную тёмную тему. Сохранение настроек запланировано.",
            "Локальная рабочая папка. Учётная запись не нужна.",
            "Языковой сервер не подключён; диагностика недоступна.",
            "Редактор Gran UI готов. Операции с файлами отображаются в строке состояния.",
            "Встроенная оболочка пока не подключена. Используйте системный терминал.",
            "Ctrl+N  Новый файл\nCtrl+O  Открыть файл\nCtrl+S  Сохранить\nCtrl+Shift+S  Сохранить как\nCtrl+W  Закрыть вкладку\nCtrl+Tab  Следующая вкладка\nCtrl+Shift+Tab  Предыдущая вкладка\nCtrl+Shift+T  Открыть закрытую\nCtrl+F  Поиск\nCtrl+B  Боковая панель",
            "Совпадений в текущем файле нет.", "Не удалось прочитать папку.", "Закрытых вкладок нет."
        };
    }

    // Язык берётся из общего выбора движка. / Use the engine's language selection.
    const char* message(Message id)
    {
        if (std::strcmp(gran::i18n::language(), "ru") == 0)
            return russian[static_cast<unsigned>(id)];
        return english[static_cast<unsigned>(id)];
    }
}
