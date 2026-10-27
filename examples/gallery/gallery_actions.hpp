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
}

#endif
