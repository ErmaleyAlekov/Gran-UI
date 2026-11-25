#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"
#include "gran/data/internal/list_notify.hpp"

#include <utility>

namespace gran::binding
{
    // Уже имеющиеся записи отражаются сразу, затем — подписка / The records already present are mirrored at once, then the subscription
    MirrorEntry::MirrorEntry(std::string modelName, data::ListSource& source, beresta::Element& target)
        : MirrorSlot(std::move(modelName), target), model(source)
    {
        for (std::size_t index = 0; index < model.size(); ++index)
            element.appendChild(mirrorItem(model.at(index)));
        model.subscribe(this);
    }

    MirrorEntry::~MirrorEntry()
    {
        model.unsubscribe(this);
    }

    // Дети элемента модели — только Item, поэтому номер записи равен номеру ребёнка / The model element children are only Items, so the record number equals the child number
    void MirrorEntry::inserted(std::size_t index, std::size_t count)
    {
        const auto size = model.size();
        if (index > size)
            data::internal::checkIndex(index, size);
        if (index > element.getChildCount())
            data::internal::checkIndex(index, element.getChildCount());
        if (count > size - index)
            data::internal::checkIndex(count - 1, size - index);
        std::vector<std::unique_ptr<beresta::Element>> items;
        items.reserve(count);
        for (std::size_t offset = 0; offset < count; ++offset)
            items.push_back(mirrorItem(model.at(index + offset)));
        std::size_t added = 0;
        try
        {
            for (auto& item : items)
            {
                element.insertChild(index + added, std::move(item));
                ++added;
            }
        }
        catch (...)
        {
            while (added > 0)
                element.removeChild(index + --added);
            throw;
        }
        ++version;
    }

    void MirrorEntry::removed(std::size_t index, std::size_t count)
    {
        for (std::size_t offset = 0; offset < count && index < element.getChildCount(); ++offset)
            element.removeChild(index);
        ++version;
    }

    // Элемент записи заменяется целиком: поля могли и появиться, и исчезнуть / The record element is replaced whole: fields might both appear and disappear
    void MirrorEntry::changed(std::size_t index)
    {
        if (index < element.getChildCount())
            element.replaceChild(index, mirrorItem(model.at(index)));
        ++version;
    }
}
