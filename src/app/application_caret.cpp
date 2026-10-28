#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    /*
    ** Прокрутка к каретке — только когда каретка, текст или высота поля изменились: колесо мыши
    ** может увести видимую часть от каретки, и следующий кадр не должен возвращать её назад.
    ** Сравниваются значения, а не флаги: ввод, буфер обмена и код приложения меняют каретку
    ** разными путями, а проверка здесь ловит их все.
    **
    ** Scrolling to the caret happens only when the caret, the text or the field height changed: the mouse wheel
    ** may move the visible part away from the caret, and the next frame must not bring it back.
    ** Values are compared, not flags: input, the clipboard and the application code change the caret
    ** in different ways, and the check here catches them all.
    */
    void revealCaret(AppWindow& win)
    {
        // Маршрутизатор отдаёт узел только для чтения, но узел принадлежит документу приложения (win.document)
        // The router gives the node read-only, but the node belongs to the application document (win.document)
        ui::Node* focus = const_cast<ui::Node*>(win.router->focused());

        if (focus == nullptr || focus->kind != ui::ElementKind::TEXT_AREA || !win.renderer)
        {
            win.revealNode = nullptr;
            return;
        }
        if (win.revealNode == focus && win.revealCaret == focus->content.caret
            && win.revealLength == focus->content.text.size() && win.revealHeight == focus->layout.frame.height)
            return;
        win.revealNode = focus;
        win.revealCaret = focus->content.caret;
        win.revealLength = focus->content.text.size();
        win.revealHeight = focus->layout.frame.height;
        if (render::scrollToCaret(win.renderer->textEngine(), *focus))
            ui::markDirty(*focus, ui::DIRTY_PAINT);
    }
}
