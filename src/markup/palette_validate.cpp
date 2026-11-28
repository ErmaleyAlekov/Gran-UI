#include "gran/markup/internal/components.hpp"
#include "gran/markup/internal/overlay.hpp"
#include "beresta.hpp"

#include <algorithm>
#include <sstream>

namespace gran::markup::internal
{
    namespace
    {
        // Внешнее значение ограничено опубликованным параметром с проверяемым типом.
        // An external value is confined to a published parameter with a validated type.
        void checkParameters(const beresta::Element& entry, const ComponentSpec& component)
        {
            std::istringstream names(entry.getAttribute("parameters").value_or(""));
            for (std::string name; names >> name;)
            {
                const auto found = std::find_if(component.params.begin(), component.params.end(),
                    [&](const auto& param) { return param.name == name; });
                if (found == component.params.end() || found->type == ParamType::ACTION || found->type == ParamType::MODEL
                    || (found->type == ParamType::STRING && found->values.empty()))
                    denyOverlay(entry, name);
            }
        }
    }

    // Шаблон палитры проверяется после загрузки всех подключённых библиотек.
    // Validate palette prototypes after all included libraries have loaded.
    void validatePalette(const beresta::Element& resources, BuildContext& context)
    {
        for (auto* section = firstElementChild(resources); section; section = nextElementSibling(*section))
        {
            if (section->getLocalName() != "Palette")
                continue;
            for (auto* entry = firstElementChild(*section); entry; entry = nextElementSibling(*entry))
            {
                if (entry->getLocalName() != "Template")
                    continue;
                auto* instance = firstElementChild(*entry);
                if (!isComponentInstance(*instance))
                {
                    if (entry->hasAttribute("parameters")) denyOverlay(*entry, "parameters");
                    continue;
                }
                checkParameters(*entry, lookupComponent(*instance, context));
                BuildContext check{context.options, context.document, context.tokens, 0};
                expandComponent(*instance, check);
            }
        }
    }
}
