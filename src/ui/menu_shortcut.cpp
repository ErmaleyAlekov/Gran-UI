#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Текст без переноса: подпись сочетания всегда в одну строку / A text without wrapping: the shortcut label is always one line
    Node shortcutLabel(const Node& item)
    {
        Node label;

        label.kind = ElementKind::TEXT;
        label.computed = item.computed;
        label.computed.textWrap = static_cast<int>(style::TextWrap::NOWRAP);
        label.content.text = item.content.shortcut;
        return label;
    }
}
