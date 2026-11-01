#ifndef GRAN_DATA_SELECTION_MODEL_HPP
#define GRAN_DATA_SELECTION_MODEL_HPP

#include "gran/data/list_source.hpp"

#include <cstdint>
#include <utility>

namespace gran::data
{
    /*
    ** Режим выделения: NONE — нет выделения (только текущая строка), SINGLE — одна запись,
    ** MULTI — щелчок переключает отметку, EXTENDED — как проводник: щелчок — одна, Ctrl — добавить,
    ** Shift — диапазон от опорной записи.
    **
    ** The selection mode: NONE — no selection (the current row only), SINGLE — one record,
    ** MULTI — a click toggles the mark, EXTENDED — as a file manager: a click — one, Ctrl — add,
    ** Shift — a range from the anchor record.
    */
    enum class SelectionMode : std::uint8_t { NONE, SINGLE, MULTI, EXTENDED };

    // Наблюдатель выделения: изменились отметки или текущая запись / A selection observer: the marks or the current record changed
    class SelectionObserver
    {
    public:
        virtual ~SelectionObserver() = default;
        virtual void selectionChanged() = 0;
    };

    // Нет записи (текущей, опорной) / No record (current, anchor)
    constexpr std::size_t NO_ROW = static_cast<std::size_t>(-1);

    /*
    ** Модель выделения над любым источником (в том числе прокси и плоским деревом). Отметки следуют
    ** за записями: вставка сдвигает их, удаление вырезает (текущая переходит на соседа), перестановка
    ** и сортировка переносят. Номера — строки источника, на который модель подписана.
    **
    ** A selection model over any source (including a proxy and a flat tree). The marks follow
    ** the records: an insertion shifts them, a removal cuts them out (the current one goes to a neighbour), a permutation
    ** and a sort carry them over. The indices are the rows of the source the model is subscribed to.
    */
    class SelectionModel final : private ListObserver
    {
    public:
        // ---- selection_model.cpp ----

        // Источник должен жить дольше модели / The source must outlive the model
        SelectionModel(ListSource& source, SelectionMode mode);
        ~SelectionModel() override;
        SelectionModel(const SelectionModel&) = delete;
        SelectionModel& operator=(const SelectionModel&) = delete;

        // Источник и режим (смена режима на более узкий сокращает выделение) / The source and the mode (a narrower mode shrinks the selection)
        ListSource& source() const noexcept;
        SelectionMode mode() const noexcept;
        void setMode(SelectionMode mode);

        // ---- selection_query.cpp ----

        // Текущая и опорная записи (NO_ROW — нет) / The current and the anchor records (NO_ROW — none)
        std::size_t current() const noexcept;
        std::size_t anchor() const noexcept;

        // Отмечена ли запись; число отмеченных / Whether the record is marked; the marked count
        bool isSelected(std::size_t index) const noexcept;
        std::size_t count() const noexcept;

        // Отмеченные записи по возрастанию / The marked records ascending
        std::vector<std::size_t> selected() const;

        // ---- selection_edit.cpp ----

        // Отметить, снять, переключить одну запись (SINGLE снимает прочие) / Mark, unmark, toggle one record (SINGLE drops the others)
        void select(std::size_t index);
        void deselect(std::size_t index);
        void toggle(std::size_t index);

        // Отметить записи [first, last] включительно (в любом порядке концов) / Mark the records [first, last] inclusive (the ends in any order)
        void selectRange(std::size_t first, std::size_t last);

        // Снять все отметки / Drop all marks
        void clear();

        // ---- selection_click.cpp ----

        // Отметить все (MULTI и EXTENDED) / Mark all (MULTI and EXTENDED)
        void selectAll();

        // Сделать запись текущей (NO_ROW — снять); отметки не меняются / Make a record current (NO_ROW — drop); the marks do not change
        void setCurrent(std::size_t index);

        /*
        ** Щелчок или клавиша по записи с модификаторами — по правилам режима: запись становится текущей,
        ** отметки меняются как в проводнике, опорная запись — для следующего Shift.
        **
        ** A click or a key on a record with modifiers — by the mode rules: the record becomes current,
        ** the marks change as in a file manager, the anchor record — for the next Shift.
        */
        void activate(std::size_t index, bool control, bool shift);

        // Счётчик изменений: зависимые выражения сверяют его / The change counter: dependent expressions compare it
        std::uint64_t version() const noexcept;

        // ---- selection_notify.cpp ----

        // Подписка без владения; отписка возможна во время уведомления / Subscription without ownership; unsubscribing during a notification is allowed
        void subscribe(SelectionObserver* observer);
        void unsubscribe(SelectionObserver* observer) noexcept;

    private:
        // ---- selection_events.cpp ----
        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void moved(std::size_t from, std::size_t count, std::size_t to) override;
        void reordered(const std::vector<std::size_t>& oldToNew) override;

        // ---- selection_notify.cpp ----

        void changed(std::size_t index) override;

        // Отметить запись (флаг и счётчик) без уведомления; true — отметка изменилась / Mark a record (the flag and the counter) without a notification; true — the mark changed
        bool mark(std::size_t index, bool value);

        // Сообщить подписчикам и сдвинуть версию / Tell the subscribers and bump the version
        void publish();

        ListSource& _source;
        SelectionMode _mode;
        // Отметки по записям источника (байт на запись — проще и быстрее диапазонов при вставках в конец) / The marks per source record (a byte per record — simpler and faster than ranges on appends)
        std::vector<std::uint8_t> _marks;
        std::size_t _count = 0;
        std::size_t _current = NO_ROW;
        std::size_t _anchor = NO_ROW;
        std::uint64_t _version = 1;
        std::vector<SelectionObserver*> _observers;
    };
}

#endif
