#include "gran/markup/internal/overlay.hpp"
#include "gran/markup/live_document.hpp"

namespace gran::generated
{
    extern const unsigned char GRAN_OVERLAY_SCHEMA[];
    extern const std::size_t GRAN_OVERLAY_SCHEMA_SIZE;
}

namespace gran::markup::internal
{
    // Текст профиля проходит отдельную схему до выполнения первой операции.
    // Validate the profile against its own schema before applying the first operation.
    beresta::Document parseOverlay(std::string_view xml)
    {
        checkOverlaySize(xml.size());
        static const auto schema = beresta::xsd::Schema::fromString(std::string_view(
            reinterpret_cast<const char*>(generated::GRAN_OVERLAY_SCHEMA), generated::GRAN_OVERLAY_SCHEMA_SIZE));
        return parseMarkupText(xml, schema);
    }
}

namespace gran::markup
{
    namespace
    {
        // Узлы внутреннего дерева компонента не являются адресами в пользовательском XML.
        // A component's internal nodes are not addresses in the user's XML.
        void indexTargets(const ui::Node& node, internal::OverlayContext& context)
        {
            auto* element = internal::editableElement(node);
            if (element && element->getOwnerDocument() == context.candidate.dom.get()
                && element->getAttribute("id") == node.id && !node.id.empty())
                context.targets.emplace(node.id, element);
            for (const auto& child : node.children)
                indexTargets(*child, context);
        }

        // Разрешения проверяются перед записью в рабочую копию DOM.
        // Check permissions before writing to the working DOM copy.
        void editAttribute(beresta::Element& target, const beresta::Element& operation)
        {
            const auto name = operation.getLocalName();
            if (name == "Hide")
            {
                internal::requirePermission(target, Customization::HIDE);
                target.setAttribute("visible", "false");
                return;
            }
            internal::requirePermission(target, Customization::STYLE);
            if (name == "Resize")
            {
                target.setAttribute("width", *internal::findAttribute(operation, "width"));
                target.setAttribute("height", *internal::findAttribute(operation, "height"));
                return;
            }
            const auto& value = *internal::findAttribute(operation, "value");
            if (value.find_first_of("{}$") != std::string::npos)
                internal::denyOverlay(operation, value);
            target.setAttribute(*internal::findAttribute(operation, "name"), value);
        }

        // Схема уже проверила имя операции и обязательные атрибуты.
        // The schema has already validated the operation name and required attributes.
        void applyOperation(internal::OverlayContext& context, const beresta::Element& operation)
        {
            const auto name = operation.getLocalName();
            if (name == "Move" || name == "Insert")
                internal::applyPlacement(context, operation);
            else if (name == "Token")
                internal::applyToken(context, operation);
            else if (name == "Rebind")
                internal::applyRebind(context, operation);
            else if (name == "Shortcut")
                internal::applyShortcut(context, operation);
            else if (auto* target = internal::overlayTarget(context, operation, "id"))
                editAttribute(*target, operation);
        }
    }

    // Только полностью перестроенный и проверенный кандидат возвращается вызывающей стороне.
    // Return the candidate to the caller only after it is fully rebuilt and validated.
    OverlayResult applyOverlay(const UiDocument& base, std::string_view overlay, const LoadOptions& options)
    {
        auto operations = internal::parseOverlay(overlay);
        LoadOptions load = options;
        if (load.baseDirectory.empty())
            load.baseDirectory = base.baseDirectory;
        auto candidate = loadMarkupString(serializeMarkup(base), load);
        OverlayResult result;
        internal::OverlayContext context{candidate, {}, result.missingIds, {}};
        indexTargets(*candidate.root, context);
        for (auto* op = internal::firstElementChild(*operations.getRoot()); op; op = internal::nextElementSibling(*op))
            applyOperation(context, *op);
        internal::materializeTokens(context);
        result.document = loadMarkupString(serializeMarkup(candidate), load);
        return result;
    }
}
