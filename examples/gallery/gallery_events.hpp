#ifndef GRAN_GALLERY_EVENTS_HPP
#define GRAN_GALLERY_EVENTS_HPP

#include "gran/app/application.hpp"

#include <string>

namespace gallery
{
    // ---- gallery_reset.cpp ----

    // Текст узла id (нет узла — ничего) и перерисовка / The text of the id node (no node — nothing) and a repaint
    void setText(gran::Application& application, const char* id, const std::string& text);

    // Кнопка «Сбросить доску» шлёт своё событие boardReset, доска возвращает колонки к началу
    // The "Reset the board" button sends the boardReset custom event, the board returns the columns to the start
    void registerReset(gran::Application& application);

    // ---- gallery_pointer.cpp ----

    // Поле pad: захват указателя, координаты в системе узла, колесо, своё контекстное меню
    // The pad area: the pointer capture, the coordinates in the node system, the wheel, a custom context menu
    void registerPointer(gran::Application& application);

    // ---- gallery_keys.cpp ----

    // Клавиши в фазе захвата у доски и фокус поля field / The keys in the capture phase at the board and the field focus
    void registerKeys(gran::Application& application);

    // ---- gallery_commands.cpp ----

    // Действия команд samples/commands.xml и флажок, выключающий «Сохранить» / The samples/commands.xml command actions and the check box disabling "Save"
    void registerCommands(gran::Application& application);

    // ---- gallery_notes.cpp ----

    // Удаление заметки с отменой и «*» в заголовке при несохранённых изменениях / Deleting a note with undo and "*" in the title with unsaved changes
    void registerNotes(gran::Application& application);

    // ---- gallery_web.cpp ----

    // Страница <WebView onMessage="fromPage">: статус в окне и ответ обратно в страницу
    // The <WebView onMessage="fromPage"> page: the status in the window and an answer back into the page
    void registerWeb(gran::Application& application);
}

#endif
