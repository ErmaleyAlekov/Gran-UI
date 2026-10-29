#include "gran/markup/internal/build_context.hpp"

#include <cctype>

namespace gran::markup::internal
{
    namespace
    {
        /*
        ** Имя атрибута → вид события: on + имя с заглавной буквы + необязательный суффикс Capture.
        ** onClick, onChange и прочие действия элементов событиями не считаются (у них нет вида).
        **
        ** An attribute name → the event kind: on + the capitalized name + an optional Capture suffix.
        ** onClick, onChange and the other control actions are not events (they have no kind).
        */
        bool parseEventAttribute(std::string_view attribute, ui::EventType& type, bool& capture)
        {
            const std::string_view suffix = "Capture";

            if (attribute.size() < 3 || attribute.compare(0, 2, "on") != 0
                || std::isupper(static_cast<unsigned char>(attribute[2])) == 0)
                return false;
            std::string_view body = attribute.substr(2);
            capture = body.size() > suffix.size() && body.compare(body.size() - suffix.size(), suffix.size(), suffix) == 0;
            if (capture)
                body.remove_suffix(suffix.size());
            std::string name(body);
            name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(name[0])));
            type = ui::eventType(name);
            return type != ui::EventType::CUSTOM;
        }

        /*
        ** on="имя:действие имя2:действие2": пары через пробелы, имя — до первого двоеточия (действие может
        ** само содержать двоеточия: gran:focus:поле). Встроенное имя (pointerDown) даёт встроенный вид.
        **
        ** on="name:action name2:action2": pairs by spaces, the name is up to the first colon (the action may
        ** contain colons itself: gran:focus:field). A built-in name (pointerDown) gives the built-in kind.
        */
        void readCustomEvents(ui::Node& node, const std::string& value)
        {
            std::size_t start = 0;

            while (start < value.size())
            {
                std::size_t end = start;
                while (end < value.size() && std::isspace(static_cast<unsigned char>(value[end])) == 0)
                    ++end;
                std::size_t colon = value.find(':', start);
                if (colon != std::string::npos && colon > start && colon + 1 < end)
                {
                    ui::EventBinding binding;
                    binding.name = value.substr(start, colon - start);
                    binding.type = ui::eventType(binding.name);
                    binding.action = value.substr(colon + 1, end - colon - 1);
                    node.content.events.push_back(binding);
                }
                start = end + 1;
            }
        }
    }

    // Значения уже проверены схемой (тип Action и CustomEvents) / The values are already checked by the schema (the Action and CustomEvents types)
    bool readEventAttribute(ui::Node& node, std::string_view name, const std::string& value)
    {
        ui::EventBinding binding;

        if (name == "on")
        {
            readCustomEvents(node, value);
            return true;
        }
        if (!parseEventAttribute(name, binding.type, binding.capture))
            return false;
        binding.action = value;
        node.content.events.push_back(binding);
        // onDrop — ещё и зона файлов из Проводника (Application::onFileDrop) / onDrop is also a zone for files from Explorer (Application::onFileDrop)
        if (binding.type == ui::EventType::DROP && !binding.capture)
            node.content.onDrop = value;
        return true;
    }
}
