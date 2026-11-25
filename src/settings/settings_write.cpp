#include "gran/settings/internal/settings_xml.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::settings::internal
{
    namespace
    {
        // Экранирование текста и атрибутов XML (& < > " и перевод строки в атрибуте) / Escaping the XML text and attributes
        std::string escape(const std::string& text)
        {
            std::string result;

            for (char symbol : text)
            {
                if (symbol == '&')
                    result += "&amp;";
                else if (symbol == '<')
                    result += "&lt;";
                else if (symbol == '>')
                    result += "&gt;";
                else if (symbol == '"')
                    result += "&quot;";
                else if (symbol == '\n')
                    result += "&#10;";
                else if (symbol == '\r')
                    result += "&#13;";
                else if (symbol == '\t')
                    result += "&#9;";
                else
                    result += symbol;
            }
            return result;
        }

        // Имя типа для атрибута type / The type name for the type attribute
        const char* typeName(ValueType type) noexcept
        {
            if (type == ValueType::NUMBER)
                return "number";
            if (type == ValueType::BOOLEAN)
                return "boolean";
            return "string";
        }

        // <Value>, <Variable> или <List> одного значения / <Value>, <Variable> or <List> of one value
        void writeValue(std::string& out, bool variable, const std::string& name, const Value& value)
        {
            std::string element = variable ? "Variable" : "Value";
            if (value.type == ValueType::LIST)
            {
                element = variable ? "VariableList" : "List";
                out += "  <" + element + " name=\"" + escape(name) + "\">\n";
                for (const std::string& item : value.items)
                    out += "    <Item>" + escape(item) + "</Item>\n";
                out += "  </" + element + ">\n";
                return;
            }
            out += std::string("  <") + element + " name=\"" + escape(name) + "\" type=\"" + typeName(value.type) + "\">"
                + escape(value.text) + "</" + element + ">\n";
        }

        // Окна и атрибуты узлов / The windows and the node attributes
        void writeLayout(std::string& out, const Settings::Model& model)
        {
            for (const std::pair<const std::string, platform::WindowPlacement>& window : model.windows)
            {
                const platform::WindowPlacement& place = window.second;
                std::string state = "normal";
                if (place.maximized)
                    state = "maximized";
                out += "  <Window layout=\"" + escape(window.first) + "\" x=\"" + std::to_string(place.x) + "\" y=\""
                    + std::to_string(place.y) + "\" width=\"" + std::to_string(place.width) + "\" height=\""
                    + std::to_string(place.height) + "\" state=\"" + state + "\"/>\n";
            }
            for (const std::pair<const AttributeKey, std::string>& attribute : model.attributes)
                out += "  <Attribute layout=\"" + escape(std::get<0>(attribute.first)) + "\" node=\""
                    + escape(std::get<1>(attribute.first)) + "\" name=\"" + escape(std::get<2>(attribute.first))
                    + "\" value=\"" + escape(attribute.second) + "\"/>\n";
        }
    }

    // Порядок: окна, атрибуты, значения, переменные — по именам (std::map) / The order: the windows, the attributes, the values, the variables — by names (std::map)
    std::string writeSettings(const Settings::Model& model)
    {
        std::string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Settings version=\"1\">\n";

        writeLayout(out, model);
        for (const std::pair<const std::string, Value>& value : model.values)
            writeValue(out, false, value.first, value.second);
        for (const std::pair<const std::string, Value>& variable : model.variables)
            writeValue(out, true, variable.first, variable.second);
        out += "</Settings>\n";
        for (std::size_t offset = 0; offset < out.size(); )
        {
            const auto start = offset;
            const auto ch = decodeUtf8(out, offset);
            if ((ch < 32 && ch != 9 && ch != 10 && ch != 13) || ch == 0xfffe || ch == 0xffff ||
                (ch == REPLACEMENT_CHARACTER && offset - start == 1))
                return {};
        }
        return out;
    }
}
