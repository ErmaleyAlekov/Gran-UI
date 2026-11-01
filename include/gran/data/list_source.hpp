#ifndef GRAN_DATA_LIST_SOURCE_HPP
#define GRAN_DATA_LIST_SOURCE_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace gran::data
{
    // Запись списка: имя поля → текст (как атрибуты элемента <Item>)
    // A list record: a field name → a text (like the attributes of an <Item> element)
    using Record = std::map<std::string, std::string, std::less<>>;

    // Тип столбца таблицы: как сравнивать и показывать значения / A table column type: how to compare and show the values
    enum class ColumnType : std::uint8_t { TEXT, NUMBER, DATE, BOOLEAN };

    // Столбец таблицы: поле записи, заголовок и тип / A table column: the record field, the title and the type
    struct Column
    {
        std::string name;
        std::string title;
        ColumnType type = ColumnType::TEXT;
    };

    // Описание столбцов таблицы (у простого списка — пустое) / The table column description (empty for a plain list)
    using ColumnSet = std::vector<Column>;

    /*
    ** Наблюдатель списка (паттерн Observer). Привязка в разметке, зеркало выражений, прокси и модель
    ** выделения подписываются на список и по каждому уведомлению правят только затронутое.
    ** Уведомление приходит ПОСЛЕ изменения: подписчик читает уже новые данные. Три старых метода
    ** обязательны; перемещение и пересортировка по умолчанию сводятся к удалению и вставке — подписчик,
    ** которому важно сохранить состояние строк (фокус, выделение), переопределяет их.
    **
    ** A list observer (the Observer pattern). A markup binding, the expression mirror, proxies and the selection
    ** model subscribe to a list and on each notification edit only what is affected.
    ** A notification comes AFTER the change: the subscriber reads the new data already. The three old methods
    ** are mandatory; a move and a re-sort by default come down to a removal and an insertion — a subscriber
    ** that must keep the row state (the focus, the selection) overrides them.
    */
    class ListObserver
    {
    public:
        virtual ~ListObserver() = default;

        // Вставлено count записей начиная с index / count records were inserted starting at index
        virtual void inserted(std::size_t index, std::size_t count) = 0;

        // Удалено count записей начиная с index / count records were removed starting at index
        virtual void removed(std::size_t index, std::size_t count) = 0;

        // Поля записи index изменились / The fields of the record index changed
        virtual void changed(std::size_t index) = 0;

        // ---- list_source_defaults.cpp ----

        // Поля записей [first, first + count) изменились; по умолчанию — changed каждой / The fields of the records [first, first + count) changed; by default — changed of each
        virtual void changedRange(std::size_t first, std::size_t count);

        /*
        ** Записи [from, from + count) переставлены так, что теперь начинаются с to (номер в новом порядке).
        ** По умолчанию — removed(from, count) и inserted(to, count).
        **
        ** The records [from, from + count) were moved so that they now start at to (an index in the new order).
        ** By default — removed(from, count) and inserted(to, count).
        */
        virtual void moved(std::size_t from, std::size_t count, std::size_t to);

        /*
        ** Тот же набор записей в новом порядке: oldToNew[старый номер] = новый номер. По умолчанию —
        ** удаление всех и вставка всех.
        **
        ** The same record set in a new order: oldToNew[old index] = new index. By default —
        ** removing all and inserting all.
        */
        virtual void reordered(const std::vector<std::size_t>& oldToNew);
    };

    /*
    ** Источник записей для привязок и прокси (паттерн «Шаблонный метод» для уведомлений): хранилище
    ** ListModel, прокси сортировки и фильтра, плоское представление дерева. Привязки в разметке,
    ** зеркало выражений и модель выделения работают с любым источником одинаково, поэтому прокси
    ** можно ставить друг на друга. Работает только в потоке интерфейса.
    **
    ** A record source for bindings and proxies (the Template Method pattern for notifications): the ListModel
    ** storage, the sort and filter proxy, the flat tree view. Markup bindings, the expression mirror and the
    ** selection model work with any source alike, so proxies can be stacked. It works only in the UI thread.
    */
    class ListSource
    {
    public:
        // ---- list_source_defaults.cpp ----
        virtual ~ListSource();

        // Число записей / The record count
        virtual std::size_t size() const noexcept = 0;

        // Запись по номеру; вне диапазона — GranException(INVALID_ARGUMENT) / A record by index; out of range — GranException(INVALID_ARGUMENT)
        virtual const Record& at(std::size_t index) const = 0;

        // Сменить одно поле записи (прокси пишет в свой источник) / Change one record field (a proxy writes into its source)
        virtual void set(std::size_t index, const std::string& field, std::string value) = 0;

        // Описание столбцов (у списка без столбцов — пустое) / The column description (empty for a list without columns)
        virtual const ColumnSet& columns() const noexcept;

        // ---- list_source.cpp ----

        // Есть ли ещё записи для ленивой подгрузки / Whether there are more records for lazy loading
        virtual bool canFetchMore() const;

        // Подгрузить следующую порцию (записи приходят обычными уведомлениями inserted) / Fetch the next portion (the records come as ordinary inserted notifications)
        virtual void fetchMore();

        // Подписать наблюдателя (без владения; повторная подписка игнорируется) / Subscribe an observer (not owned; a repeated subscription is ignored)
        void subscribe(ListObserver* observer);

        // Отписать наблюдателя (можно и во время уведомления) / Unsubscribe an observer (allowed during a notification too)
        void unsubscribe(ListObserver* observer) noexcept;

        // Подписан ли наблюдатель / Whether the observer is subscribed
        bool subscribed(const ListObserver* observer) const noexcept;

    protected:
        // ---- list_source_notify.cpp ----

        // Разослать вставку / Deliver an insertion
        void notifyInserted(std::size_t index, std::size_t count);

        // Разослать удаление / Deliver a removal
        void notifyRemoved(std::size_t index, std::size_t count);

        // Разослать правку записей [first, first + count) / Deliver an edit of the records [first, first + count)
        void notifyChanged(std::size_t first, std::size_t count);

        // Разослать перемещение / Deliver a move
        void notifyMoved(std::size_t from, std::size_t count, std::size_t to);

        // Разослать пересортировку / Deliver a re-sort
        void notifyReordered(const std::vector<std::size_t>& oldToNew);

    private:
        std::vector<ListObserver*> _observers;
    };
}

#endif
