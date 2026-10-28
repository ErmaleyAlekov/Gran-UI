#ifndef GRAN_GALLERY_ACTIONS_HPP
#define GRAN_GALLERY_ACTIONS_HPP

#include "gran/app/application.hpp"

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
}

#endif
