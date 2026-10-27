#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    namespace
    {
        // Вызов API у узла не того вида — ошибка программиста, а не тихое «ничего» / An API call on a node of a wrong kind is a programmer error, not a silent "nothing"
        void requireKind(const ui::Node& node, ui::ElementKind kind)
        {
            if (node.kind != kind)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_WRONG_ELEMENT,
                    {ui::elementName(node.kind), ui::elementName(kind)}));
        }
    }

    // Место диалога считает компоновка (центр окна), якорь не нужен / The layout computes the dialog place (the window center), no anchor is needed
    void Application::showDialog(ui::Node& dialog)
    {
        requireKind(dialog, ui::ElementKind::DIALOG);
        if (_state->router)
            _state->router->openPopup(dialog, Rect{}, false);
    }

    // Закрытие закрывает и открытые внутри меню и списки / Closing also closes the menus and lists open inside
    void Application::hideDialog(ui::Node& dialog)
    {
        requireKind(dialog, ui::ElementKind::DIALOG);
        if (_state->router)
            _state->router->closePopup(dialog);
    }

    // Работает и до run(): выбор — свойство разметки, а не ввода / Works before run() too: the choice is a markup property, not an input one
    void Application::selectOption(ui::Node& comboBox, const std::string& value)
    {
        requireKind(comboBox, ui::ElementKind::COMBO_BOX);
        ui::selectOption(comboBox, ui::findOption(comboBox, value));
        comboBox.content.option = value;
    }
}
