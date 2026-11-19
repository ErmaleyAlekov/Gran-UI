#include "gran/app/internal/app_state.hpp"
#include "gran/data/sort_filter_proxy.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        /*
        ** Шаг отмены правки ячейки. Запись ищется по имени хранилища и номеру в нём, а не по указателю и номеру
        ** в прокси: порядок прокси меняется сортировкой и фильтром, а номер в хранилище — только вставками
        ** и удалениями (как у любого стека отмены, шаги верны, пока данные меняются через него же). Хранилища
        ** больше нет или запись ушла за конец — шаг ничего не делает.
        **
        ** The undo step of a cell edit. The record is found by the storage name and the index in it, not by a pointer and an index
        ** in a proxy: the proxy order changes by sorting and filtering, while the storage index — only by insertions
        ** and removals (as for any undo stack, the steps are right while the data changes through it). The storage
        ** is gone or the record went past the end — the step does nothing.
        */
        class CellStep final : public undo::UndoStep
        {
        public:
            CellStep(binding::ListBindings& lists, std::string list, std::size_t index, std::string field,
                std::pair<std::string, std::string> values)
                : _lists(lists), _list(std::move(list)), _index(index), _field(std::move(field)), _values(std::move(values))
            {
            }

            void undo() override
            {
                apply(_values.first);
            }

            void redo() override
            {
                apply(_values.second);
            }

        private:
            void apply(const std::string& value)
            {
                data::ListSource* source = _lists.list(_list);

                if (source != nullptr && _index < source->size())
                    source->set(_index, _field, value);
            }

            binding::ListBindings& _lists;
            std::string _list;
            std::size_t _index;
            std::string _field;
            // Значение до правки и после / The value before the edit and after
            std::pair<std::string, std::string> _values;
        };
    }

    // Прокси разворачиваются до хранилища; хранилище без имени (не из разметки) — шага нет / The proxies are unwrapped down to the storage; a storage without a name (not from the markup) — no step
    void pushCellStep(AppWindow& win, const GridEdit& edit, std::size_t index, const std::string& before,
        const std::string& after)
    {
        data::ListSource* source = &edit.record->source();

        if (!edit.undoable)
            return;
        for (data::SortFilterProxy* proxy = dynamic_cast<data::SortFilterProxy*>(source); proxy != nullptr;
             proxy = dynamic_cast<data::SortFilterProxy*>(source))
        {
            index = proxy->sourceIndex(index);
            source = &proxy->source();
        }
        std::string name = win.shared.lists.nameOf(*source);
        if (name.empty())
            return;
        std::unique_ptr<UndoLink>& link = win.shared.undoStacks[edit.undoStack];
        if (!link)
            link = std::make_unique<UndoLink>(win.shared, edit.undoStack);
        link->stack.push(std::make_unique<CellStep>(win.shared.lists, std::move(name), index, edit.field,
            std::make_pair(before, after)));
    }
}
