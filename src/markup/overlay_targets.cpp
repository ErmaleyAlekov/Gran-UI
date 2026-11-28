#include "gran/markup/internal/overlay.hpp"
#include "gran/core/i18n.hpp"
#include "gran/core/gran_exception.hpp"

namespace gran::markup
{
    // Ограничиваем объём XML до выделения памяти на копию профиля.
    // Bound the XML size before allocating a profile copy.
    void checkOverlaySize(std::size_t bytes)
    {
        if (bytes > MAX_OVERLAY_BYTES)
            throw GranException(ErrorCode::LIMIT_EXCEEDED, i18n::format(i18n::Message::OVERLAY_LIMIT,
                {std::to_string(MAX_OVERLAY_BYTES)}));
    }
}

namespace gran::markup::internal
{
    // В сообщении сохраняется позиция отклонённой правки.
    // Preserve the location of the rejected edit in the diagnostic.
    void denyOverlay(const beresta::Element& element, std::string_view reason)
    {
        throwElementError(element, i18n::format(i18n::Message::OVERLAY_DENIED, {reason}));
    }

    // Все операции проходят единое правило наследования.
    // All operations use the same inheritance rule.
    void requirePermission(const beresta::Element& target, Customization permission)
    {
        if (!permitsCustomization(target, permission))
            denyOverlay(target, target.getAttribute("id").value_or(target.getName()));
    }

    // Удалённый при обновлении id пропускается с предупреждением в результате.
    // An ID removed by an update is skipped and reported in the result.
    beresta::Element* overlayTarget(OverlayContext& context, const beresta::Element& operation, std::string_view key)
    {
        const auto& id = *findAttribute(operation, key);
        const auto found = context.targets.find(id);
        if (found != context.targets.end())
            return found->second;
        context.missingIds.push_back(id);
        return nullptr;
    }
}
