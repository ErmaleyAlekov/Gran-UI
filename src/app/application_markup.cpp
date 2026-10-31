#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/markup/live_document.hpp"

namespace gran
{
    namespace
    {
        /*
        ** Правка через документ окна, которому принадлежит узел: схема, стили и тема — его. Узел вне окон
        ** (ещё не вставлен или уже удалён) правке не подлежит. text, value и checked — сами значения
        ** состояния: их запись меняет и состояние узла, остальные атрибуты состояние оставляют.
        **
        ** Editing through the document of the window the node belongs to: the schema, the styles and the theme are its own. A node outside the windows
        ** (not inserted yet or already removed) is not editable. text, value and checked are the state values
        ** themselves: writing them changes the node state too, the other attributes keep the state.
        */
        void editAttribute(Application::State& state, ui::Node& node, const std::string& name,
            const std::optional<std::string>& value)
        {
            app::AppWindow* win = app::findWindow(state, node);

            if (win == nullptr)
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::MARKUP_NOT_LIVE, {ui::elementName(node.kind)}));
            std::unique_ptr<ui::Node> fresh = markup::editMarkupAttribute(win->document, node, name, value);
            bool stateValue = name == "text" || name == "value" || name == "checked";
            app::rebuildInPlace(*win, node, std::move(fresh), !stateValue);
        }
    }

    // Чтение не зависит от окна: элемент узла — в живом DOM его документа / Reading does not depend on the window: the node element is in the live DOM of its document
    std::optional<std::string> Application::markupAttribute(const ui::Node& node, const std::string& name) const
    {
        return markup::markupAttribute(node, name);
    }

    void Application::setMarkupAttribute(ui::Node& node, const std::string& name, const std::string& value)
    {
        editAttribute(*_state, node, name, value);
    }

    void Application::removeMarkupAttribute(ui::Node& node, const std::string& name)
    {
        editAttribute(*_state, node, name, std::nullopt);
    }
}
