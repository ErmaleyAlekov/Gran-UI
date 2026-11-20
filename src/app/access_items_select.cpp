#include "gran/app/internal/access_items.hpp"
#include "gran/data/tree_list.hpp"

namespace gran::app
{
    // MULTI (щелчок переключает) и EXTENDED (Ctrl и Shift) выделяют несколько строк / MULTI (a click toggles) and EXTENDED (Ctrl and Shift) select several rows
    bool AccessItems::multipleSelection(const ui::Node& list) const noexcept
    {
        binding::ItemRef source = _lists.listOf(list);

        if (source.selection == nullptr)
            return false;
        return source.selection->mode() == data::SelectionMode::MULTI
            || source.selection->mode() == data::SelectionMode::EXTENDED;
    }

    // Глубина есть только у плоского дерева; его доступ к строке может бросить — ответ провайдеру ОС без исключений
    // Only a flat tree has a depth; its row access may throw — the answer to the OS provider is without exceptions
    std::size_t AccessItems::itemDepth(const ui::Node& item) const noexcept
    {
        binding::ItemRef record = _lists.itemOf(item);
        const data::TreeList* rows = dynamic_cast<const data::TreeList*>(record.model);

        if (rows == nullptr || record.index >= rows->size())
            return 0;
        try
        {
            return rows->depth(record.index);
        }
        catch (...)
        {
            return 0;
        }
    }
}
