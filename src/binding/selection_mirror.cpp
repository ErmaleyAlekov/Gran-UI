#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::binding
{
    MirrorSlot::MirrorSlot(std::string slotName, beresta::Element& target)
        : name(std::move(slotName)), element(target)
    {
    }

    MirrorSlot::~MirrorSlot() = default;

    SelectionMirror::SelectionMirror(std::string modelName, data::SelectionModel& model, beresta::Element& target)
        : MirrorSlot(std::move(modelName), target), selection(model)
    {
        selectionChanged();
        selection.subscribe(this);
    }

    SelectionMirror::~SelectionMirror()
    {
        selection.unsubscribe(this);
    }

    // Отмеченных записей обычно мало — элемент строится заново целиком / There are usually few marked records — the element is rebuilt whole
    void SelectionMirror::selectionChanged()
    {
        const data::ListSource& source = selection.source();

        element.clearChildren();
        for (std::size_t index : selection.selected())
        {
            if (index < source.size())
                element.appendChild(mirrorItem(source.at(index)));
        }
        ++version;
    }
}
