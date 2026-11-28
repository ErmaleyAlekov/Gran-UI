#ifndef GRAN_MARKUP_CUSTOMIZATION_HPP
#define GRAN_MARKUP_CUSTOMIZATION_HPP

#include <cstdint>

namespace beresta
{
    class Element;
}

namespace gran::markup
{
    enum class Customization : std::uint8_t { MOVE, HIDE, STYLE, INSERT, COMMAND };

    // Первое явное правило от элемента к окну; без правила правки закрыты.
    // The first explicit rule from the element to the window wins; edits default to denied.
    bool permitsCustomization(const beresta::Element& element, Customization permission);
}

#endif
