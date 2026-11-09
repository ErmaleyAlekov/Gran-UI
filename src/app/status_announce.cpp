#include "gran/app/internal/app_state.hpp"

#include <vector>

namespace gran::app
{
    /*
    ** После кадра: тексты секций строк состояния сверяются с прошлым кадром (привязки {$…} и код уже отработали),
    ** сменившиеся уходят провайдеру доступности окна — диктор прочтёт «Сохранено», не теряя места в документе.
    **
    ** After the frame: the status section texts are compared with the previous frame (the {$…} bindings and the code already ran),
    ** the changed ones go to the window accessibility provider — the screen reader reads "Saved" without losing the place in the document.
    */
    void announceStatus(AppWindow& win)
    {
        std::vector<ui::Node*> changed;

        if (!win.document.root || !win.window)
            return;
        ui::collectStatusChanges(*win.document.root, changed);
        for (const ui::Node* section : changed)
            win.window->accessAnnounce(*section);
    }
}
