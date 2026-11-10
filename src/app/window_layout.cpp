#include "gran/app/internal/app_state.hpp"
#include "gran/markup/live_document.hpp"

#include <sstream>

namespace gran::app
{
    namespace
    {
        // Атрибут узла с persist: id узла, имя атрибута, значение / A node attribute with persist: the node id, the attribute name, the value
        struct PersistedAttribute
        {
            std::string node;
            std::string name;
            std::string value;
        };

        /*
        ** Атрибуты persist поддерева (обход до правок: setMarkupAttribute перестраивает узлы).
        ** Узел без id пропускается — его не найти при следующем запуске.
        **
        ** The persist attributes of the subtree (walked before the edits: setMarkupAttribute rebuilds the nodes).
        ** A node without an id is skipped — it cannot be found on the next start.
        */
        void collectPersisted(Application& application, const ui::Node& node, std::vector<PersistedAttribute>& found)
        {
            std::optional<std::string> names = application.markupAttribute(node, "persist");

            if (names && !node.id.empty())
            {
                std::istringstream list(*names);
                std::string name;
                while (list >> name)
                    found.push_back(PersistedAttribute{node.id, name, application.markupAttribute(node, name).value_or("")});
            }
            for (const std::unique_ptr<ui::Node>& child : node.children)
                collectPersisted(application, *child, found);
        }

        // Тип сохранённой переменной — по её объявлению <Variable type> / The saved variable type — by its <Variable type> declaration
        settings::ValueType valueType(data::VariableType type) noexcept
        {
            if (type == data::VariableType::NUMBER)
                return settings::ValueType::NUMBER;
            if (type == data::VariableType::BOOLEAN)
                return settings::ValueType::BOOLEAN;
            return settings::ValueType::STRING;
        }
    }

    // Место окна из настроек — до создания бэкенда (цепочка обмена сразу нужного размера) / The window place from the settings — before the backend creation (the swap chain is of the right size at once)
    void restorePlacement(AppWindow& win)
    {
        platform::WindowPlacement placement;
        const std::string& layout = win.document.window.layout;

        if (!layout.empty() && win.window && win.shared.settings.windowPlacement(layout, placement))
            win.window->setPlacement(placement);
    }

    /*
    ** До первого кадра: сохранённые атрибуты persist записываются в живую разметку (узлы перестраиваются),
    ** переменные persist получают сохранённые значения своего типа.
    **
    ** Before the first frame: the saved persist attributes are written into the live markup (the nodes are rebuilt),
    ** the persist variables get the saved values of their type.
    */
    void restoreState(AppWindow& win)
    {
        Application& application = *win.shared.owner;
        std::vector<PersistedAttribute> attributes;
        settings::Value value;

        if (!win.document.window.layout.empty() && win.document.root)
            collectPersisted(application, *win.document.root, attributes);
        for (const PersistedAttribute& attribute : attributes)
        {
            ui::Node* node = ui::findNode(*win.document.root, attribute.node);
            if (node != nullptr && win.shared.settings.attribute({win.document.window.layout, attribute.node, attribute.name},
                value.text))
                application.setMarkupAttribute(*node, attribute.name, value.text);
        }
        for (const data::Variable& variable : win.persistedVariables)
        {
            if (!variable.persist || !win.shared.settings.variable(variable.name, value))
                continue;
            if (value.type == settings::ValueType::NUMBER)
                application.setVariableNumber(variable.name, std::strtod(value.text.c_str(), nullptr));
            else if (value.type == settings::ValueType::BOOLEAN)
                application.setVariableBoolean(variable.name, value.text == "true");
            else
                application.setVariable(variable.name, value.text);
        }
    }

    // Место окна, атрибуты persist и переменные persist — в настройки (перед закрытием окна) / The window place, the persist attributes and the persist variables go into the settings (before closing the window)
    void saveState(AppWindow& win)
    {
        // Состояние без программы (модульные тесты оконного слоя создают State сами) — сохранять нечего:
        // переменные и атрибуты читаются через Application. Найдено UBSan: раньше указатель разыменовывался сразу.
        // A state without a program (window layer unit tests build State themselves) — nothing to save:
        // variables and attributes are read through Application. Found by UBSan: the pointer used to be dereferenced at once.
        if (win.shared.owner == nullptr)
            return;
        Application& application = *win.shared.owner;
        const std::string& layout = win.document.window.layout;
        platform::WindowPlacement placement;
        std::vector<PersistedAttribute> attributes;

        if (!layout.empty() && win.window && win.window->placement(placement))
            win.shared.settings.setWindowPlacement(layout, placement);
        if (!layout.empty() && win.document.root)
        {
            // Положение разделителей меняет мышь, а persist читает разметку / The mouse changes the splitter positions, while persist reads the markup
            markup::writeLiveAttributes(*win.document.root);
            collectPersisted(application, *win.document.root, attributes);
        }
        for (const PersistedAttribute& attribute : attributes)
            win.shared.settings.setAttribute({layout, attribute.node, attribute.name}, attribute.value);
        for (const data::Variable& variable : win.persistedVariables)
        {
            std::optional<std::string> current = application.variable(variable.name);
            if (variable.persist && current)
                win.shared.settings.setVariable(variable.name, settings::Value{valueType(variable.type), *current, {}});
        }
    }
}
