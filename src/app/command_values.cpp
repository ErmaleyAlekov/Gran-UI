#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

namespace gran::app
{
    // Только явные логические значения; опечатки не превращаются в false.
    // Only explicit boolean values; typos do not silently become false.
    static bool booleanValue(const std::string& value)
    {
        if (value == "true" || value == "1")
            return true;
        if (value == "false" || value == "0")
            return false;
        throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_ARGUMENT));
    }

    // Узел ищется заново: удаление и reload не оставляют внешних висячих указателей.
    // Resolve the node again: removal and reload leave no external dangling pointers.
    ui::Node& requireNode(Application& app, const std::string& id)
    {
        ui::Node* node = app.find(id);
        if (id.empty() || node == nullptr)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_NODE, {id}));
        return *node;
    }

    // Текст полей сохраняет границы каретки и выделения в новом содержимом; значение из кода сбрасывает историю правок поля.
    // Field text keeps caret and selection bounds inside the new content; a value from code resets the field edit history.
    static void writeText(ui::Node& node, const std::string& value)
    {
        node.content.text = value;
        node.content.history.reset();
        node.content.caret = value.size();
        node.content.anchor = value.size();
        ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Типизированные операции используют существующие правила виджетов и групп.
    // Typed operations reuse existing widget and group rules.
    void writeProperty(Application& app, ui::Node& node, const std::string& property, const std::string& value)
    {
        if (property == "text")
            writeText(node, value);
        else if (property == "style")
            app.setStyle(node, value);
        else if (property == "checked")
            app.setChecked(node, booleanValue(value));
        else if (property == "value")
            app.setValue(node, style::parseNumber(value));
        else if (property == "enabled")
            ui::setEnabled(node, booleanValue(value));
        else if (property == "visible")
        {
            node.visible = booleanValue(value);
            ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }
        else if (property == "option" && node.kind == ui::ElementKind::TABS)
            app.selectTab(node, value);
        else if (property == "option")
            app.selectOption(node, value);
        else
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_PROPERTY, {property}));
    }
}
