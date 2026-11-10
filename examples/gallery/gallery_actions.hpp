#ifndef GRAN_GALLERY_ACTIONS_HPP
#define GRAN_GALLERY_ACTIONS_HPP

#include "gran/res/resources.hpp"
#include "gran/app/application.hpp"

#include <string>

namespace gallery
{
    /*
    ** Действия витрины: save (имя из поля — в строку состояния), more (меню moreMenu у кнопки),
    ** command и choice (подпись пункта или выбранное значение — в строку состояния),
    ** closeDialog (закрыть диалог, в котором нажата кнопка).
    **
    ** The showcase actions: save (the name from the field goes into the status line), more (the moreMenu menu at the button),
    ** command and choice (the item label or the selected value goes into the status line),
    ** closeDialog (closing the dialog where the button was pressed).
    */
    void registerActions(gran::Application& application);

    /*
    ** Таймеры витрины (gallery_timers.cpp): ProgressBar timerProgress растёт по кругу, Text elapsed
    ** считает секунды, действие delayed пишет в строку состояния через 2 секунды.
    **
    ** The showcase timers (gallery_timers.cpp): the timerProgress ProgressBar grows in a circle, the elapsed Text
    ** counts seconds, the delayed action writes into the status line in 2 seconds.
    */
    void registerTimers(gran::Application& application);

    /*
    ** Платформа (gallery_platform.cpp, samples/platform.xml): openFile, saveFile, chooseFolder — системные
    ** диалоги; newWindow — второе окно tool_window.xml рядом с разметкой markup; closeTool и toolClosing
    ** (onClose второго окна) — закрыть его; filesDropped — брошенные файлы в строку состояния.
    **
    ** The platform (gallery_platform.cpp, samples/platform.xml): openFile, saveFile, chooseFolder — the system
    ** dialogs; newWindow — the second window tool_window.xml beside the markup markup; closeTool and toolClosing
    ** (the second window onClose) — close it; filesDropped — the dropped files go into the status line.
    */
    void registerPlatform(gran::Application& application, const std::string& markup);

    // Настройки диалога сохранения образца (gallery_save.cpp) / The sample save dialog options (gallery_save.cpp)
    gran::FileDialogOptions saveSampleOptions();

    /*
    ** Записывает образец в файл path (UTF-8) и возвращает текст для строки состояния (gallery_save.cpp).
    **
    ** Writes the sample into the path file (UTF-8) and returns the text for the status line (gallery_save.cpp).
    */
    std::string saveSample(const std::string& path);

    /*
    ** События (gallery_board.cpp, samples/events.xml): moveTask (onDrop колонки) и flipTask (двойной щелчок)
    ** переносят карточки между моделями todo и done; reset/resetBoard — своё событие boardReset; поле pad —
    ** захват указателя, колесо, своё контекстное меню; keyLog — клавиши в фазе захвата; фокус поля field.
    **
    ** The events (gallery_board.cpp, samples/events.xml): moveTask (the column onDrop) and flipTask (a double click)
    ** move cards between the todo and done models; reset/resetBoard is the boardReset custom event; the pad area is
    ** the pointer capture, the wheel, a custom context menu; keyLog is the keys in the capture phase; the field focus.
    */
    void registerEvents(gran::Application& application);

    /*
    ** Окна и программа (gallery_program.cpp, samples/program.xml): programDialog — модальный диалог
    ** program_dialog.xml, подчинённый окну кнопки (результат — в строку состояния); programTool —
    ** tool_window.xml с владельцем, без модальности.
    **
    ** Windows and the program (gallery_program.cpp, samples/program.xml): programDialog is the modal dialog
    ** program_dialog.xml owned by the button window (the result goes into the status line); programTool is
    ** tool_window.xml with an owner, without modality.
    */
    void registerProgram(gran::Application& application, const std::string& markup);

    // Фоновая работа примера (gallery_program.cpp): 50 шагов с прогрессом и проверкой отмены / The sample background work (gallery_program.cpp): 50 steps with progress and a cancellation check
    void slowWork(gran::TaskContext& task);

    // Кнопки taskStart / taskCancel samples/program.xml (gallery_tasks.cpp) / The taskStart / taskCancel buttons of samples/program.xml (gallery_tasks.cpp)
    void registerTasks(gran::Application& application);

    // Кнопки logWrite / crashNow и сведения о журнале и прошлом сбое (gallery_log.cpp) / The logWrite / crashNow buttons and the log and previous crash information (gallery_log.cpp)
    void registerLog(gran::Application& application);

    // Кнопка settingsBar и счётчик запусков в настройках (gallery_settings.cpp) / The settingsBar button and the launch counter in the settings (gallery_settings.cpp)
    void registerSettings(gran::Application& application);

    // Закрытие и перестановка вкладок документов samples/editor_frame.xml (gallery_tabs.cpp) / Closing and reordering the document tabs of samples/editor_frame.xml (gallery_tabs.cpp)
    void registerTabs(gran::Application& application);
}

// Пакет ресурсов галереи, созданный gran_rcc из examples/gallery/resources.xml (gen/examples/gallery/resources.cpp)
// The gallery resource bundle created by gran_rcc from examples/gallery/resources.xml (gen/examples/gallery/resources.cpp)
const gran::res::Bundle& granResources_gallery();

#endif
