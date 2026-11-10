#include "gran/markup/internal/build_context.hpp"
#include "gran/style/value_parsers.hpp"

namespace gran::markup::internal
{
    namespace
    {
        using C = ui::NodeContent;

        // Строковые поля содержимого / The string content fields
        const struct { const char* name; std::string C::* member; } STRINGS[] = {
            {"text", &C::text}, {"placeholder", &C::placeholder}, {"source", &C::source},
            {"tooltip", &C::tooltip}, {"onClick", &C::onClick}, {"onChange", &C::onChange},
            {"onSubmit", &C::onSubmit}, {"items", &C::items}, {"selection", &C::selection}, {"onMessage", &C::onMessage},
            {"popup", &C::popup}, {"group", &C::group}, {"accessibleName", &C::accessibleName},
            {"labeledBy", &C::labeledBy}, {"dragFormat", &C::dragFormat}, {"dragData", &C::dragData},
            {"acceptDrop", &C::acceptDrop},
            {"command", &C::command}};

        // Числовые поля / The numeric fields
        const struct { const char* name; float C::* member; } NUMBERS[] = {
            {"min", &C::minimum}, {"max", &C::maximum}, {"step", &C::step}};

        // Логические поля / The boolean fields
        const struct { const char* name; bool C::* member; } BOOLEANS[] = {
            {"checked", &C::checked}, {"password", &C::password}, {"readOnly", &C::readOnly}, {"undo", &C::undoable}, {"virtual", &C::virtualize},
            {"closable", &C::closable}, {"draggable", &C::draggable}, {"sizeGrip", &C::sizeGrip}};

        // Перечисления (fit у Image, direction у Scroll, submitKey у TextArea, overflowMode у детей ToolBar) и целые числа
    // The enumerations (fit of Image, direction of Scroll, submitKey of TextArea, overflowMode of ToolBar children) and the integers
        bool readEnumeration(ui::Node& node, std::string_view name, const std::string& value)
        {
            static const style::Keyword FITS[] = {{"fill", 0}, {"contain", 1}, {"cover", 2}, {"none", 3}};
            static const style::Keyword DIRECTIONS[] = {{"vertical", 0}, {"horizontal", 1}, {"both", 2}};
            static const style::Keyword SUBMITS[] = {{"ctrlEnter", 0}, {"enter", 1}};
            static const style::Keyword ROLES[] = {{"none", 0}, {"drag", 1}, {"minimize", 2}, {"maximize", 3}, {"close", 4}, {"resize", 5}};
            static const style::Keyword OVERFLOWS[] = {{"asNeeded", 0}, {"always", 1}, {"never", 2}};

            if (name == "windowRole")
                node.content.windowRole = static_cast<platform::WindowRole>(style::parseKeyword(value, ROLES, 6));
            else if (name == "fit")
                node.content.fit = static_cast<ui::ImageFit>(style::parseKeyword(value, FITS, 4));
            else if (name == "direction")
                node.content.direction = static_cast<ui::ScrollDirection>(style::parseKeyword(value, DIRECTIONS, 3));
            else if (name == "overflowMode")
                node.content.overflowMode = static_cast<ui::OverflowMode>(style::parseKeyword(value, OVERFLOWS, 3));
            else if (name == "submitKey")
                node.content.submitKey = static_cast<ui::SubmitKey>(style::parseKeyword(value, SUBMITS, 2));
            else if (name == "maxLength")
                node.content.maxLength = static_cast<std::size_t>(style::parseNumber(value));
            else if (name == "rows" && node.kind == ui::ElementKind::TEXT_AREA)
                node.content.rows = static_cast<std::size_t>(style::parseNumber(value));
            else
                return false;
            return true;
        }

        /*
        ** value: число у Slider и ProgressBar, значение выбора у MenuItem, ComboBox, RadioButton, Tabs и Tab,
        ** текст у прочих.
        **
        ** value: a number for Slider and ProgressBar, the choice value for MenuItem, ComboBox, RadioButton, Tabs and Tab,
        ** a text for the others.
        */
        void readValue(ui::Node& node, const std::string& value)
        {
            ui::ElementKind kind = node.kind;

            if (kind == ui::ElementKind::SLIDER || kind == ui::ElementKind::PROGRESS_BAR)
                node.content.value = style::parseNumber(value);
            else if (kind == ui::ElementKind::MENU_ITEM || kind == ui::ElementKind::COMBO_BOX || kind == ui::ElementKind::RADIO_BUTTON
                || kind == ui::ElementKind::TABS || kind == ui::ElementKind::TAB)
                node.content.option = value;
            else
                node.content.text = value;
        }

        // Поле из таблиц строк, чисел и логических значений; false — имени в таблицах нет
        // A field from the string, number and boolean tables; false — the name is not in the tables
        bool readField(ui::NodeContent& content, std::string_view name, const std::string& value)
        {
            for (const auto& field : STRINGS)
            {
                if (name == field.name)
                {
                    content.*field.member = value;
                    return true;
                }
            }
            for (const auto& field : NUMBERS)
            {
                if (name == field.name)
                {
                    content.*field.member = style::parseNumber(value);
                    return true;
                }
            }
            for (const auto& field : BOOLEANS)
            {
                if (name == field.name)
                {
                    content.*field.member = style::parseBoolean(value);
                    return true;
                }
            }
            return false;
        }
    }

    // Атрибут содержимого по таблицам; значения уже проверены схемой
    // A content attribute by the tables; the values are already checked by the schema
    bool readContentAttribute(ui::Node& node, std::string_view name, const std::string& value)
    {
        if (name == "value")
        {
            readValue(node, value);
            return true;
        }
        if (readField(node.content, name, value) || readEventAttribute(node, name, value))
            return true;
        return readEnumeration(node, name, value) || readSplitterAttribute(node, name, value);
    }
}
