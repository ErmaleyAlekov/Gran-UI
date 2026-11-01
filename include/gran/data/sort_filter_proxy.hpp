#ifndef GRAN_DATA_SORT_FILTER_PROXY_HPP
#define GRAN_DATA_SORT_FILTER_PROXY_HPP

#include "gran/data/record_order.hpp"

#include <memory>

namespace gran::data
{
    /*
    ** Прокси сортировки и фильтра поверх любого источника (паттерн «Заместитель» + Observer): сам
    ** записей не хранит, только порядок номеров источника. Правки источника обрабатываются точечно:
    ** вставка — двоичный поиск места, правка поля — проверка фильтра и перемещение на новое место,
    ** смена ключей — пересортировка (reordered). Запись через прокси уходит в источник, поэтому
    ** двусторонние привязки в шаблоне работают сквозь него. При равных ключах сохраняется порядок
    ** источника (сортировка устойчива).
    **
    ** A sort and filter proxy over any source (the Proxy pattern + Observer): it keeps no records itself,
    ** only the order of the source indices. Source edits are handled point by point:
    ** an insertion — a binary search for the place, a field edit — a filter check and a move to the new place,
    ** a key change — a re-sort (reordered). A write through the proxy goes into the source, so
    ** the two-way bindings in a template work through it. With equal keys the source order is kept
    ** (the sort is stable).
    */
    class SortFilterProxy final : public ListSource, private ListObserver
    {
    public:
        // ---- sort_filter_proxy.cpp ----

        // Источник должен жить дольше прокси / The source must outlive the proxy
        explicit SortFilterProxy(ListSource& source);
        ~SortFilterProxy() override;
        SortFilterProxy(const SortFilterProxy&) = delete;
        SortFilterProxy& operator=(const SortFilterProxy&) = delete;

        std::size_t size() const noexcept override;
        const Record& at(std::size_t index) const override;
        void set(std::size_t index, const std::string& field, std::string value) override;

        // ---- sort_filter_query.cpp ----

        // Источник прокси / The proxy source
        ListSource& source() const noexcept;

        // Номер записи источника для строки прокси / The source record index for a proxy row
        std::size_t sourceIndex(std::size_t index) const;

        // Строка прокси для записи источника; size_t(-1) — запись скрыта фильтром / The proxy row for a source record; size_t(-1) — the record is hidden by the filter
        std::size_t proxyIndex(std::size_t sourceIndex) const noexcept;

        // Столбцы — источника / The columns are the source ones
        const ColumnSet& columns() const noexcept override;

        // ---- sort_filter_order.cpp ----

        // Ключи сортировки (пусто — порядок источника) / The sort keys (empty — the source order)
        void setSort(SortKeys keys);
        const SortKeys& sort() const noexcept;

        // Фильтр (nullptr — все записи) / The filter (nullptr — all records)
        void setFilter(std::shared_ptr<RecordFilter> filter);

        // Ответ фильтра мог измениться (например, сменилась переменная выражения) / The filter answer may have changed (for example, an expression variable changed)
        void invalidateFilter();

        // ---- sort_filter_rows.cpp ----

        // Подгрузка — у источника / Fetching is the source's
        bool canFetchMore() const override;
        void fetchMore() override;

    private:
        // ---- sort_filter_events.cpp ----
        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void changed(std::size_t index) override;
        void moved(std::size_t from, std::size_t count, std::size_t to) override;
        void reordered(const std::vector<std::size_t>& oldToNew) override;

        // ---- sort_filter_order.cpp ----

        // Пересортировать по ключам и разослать reordered, если порядок изменился / Re-sort by the keys and deliver reordered if the order changed
        void resort();

        // ---- sort_filter_place.cpp ----

        // Проходит ли запись источника фильтр / Whether the source record passes the filter
        bool accepts(std::size_t sourceIndex) const;

        // Место записи источника в порядке прокси (двоичный поиск) / The place of a source record in the proxy order (a binary search)
        std::size_t position(std::size_t sourceIndex) const;

        // Порядок «ключи, затем номер источника» / The "keys, then the source index" order
        bool before(std::size_t left, std::size_t right) const;

        // Вставить запись источника на её место и разослать inserted / Insert a source record at its place and deliver inserted
        void place(std::size_t sourceIndex);

        // Сравнитель для алгоритмов стандартной библиотеки / A comparer for the standard library algorithms
        struct Order
        {
            const SortFilterProxy* proxy;
            bool operator()(std::size_t left, std::size_t right) const;
        };

        // ---- sort_filter_rows.cpp ----

        // Убрать строки прокси (номера по убыванию) с уведомлениями по непрерывным участкам / Remove proxy rows (indices descending) with notifications by contiguous runs
        void eraseRows(const std::vector<std::size_t>& rows);

        // ---- sort_filter_query.cpp ----

        // Обратная таблица источник → прокси (пересчитывается лениво) / The reverse table source → proxy (recomputed lazily)
        const std::vector<std::size_t>& reverse() const;

        ListSource& _source;
        SortKeys _keys;
        std::shared_ptr<RecordFilter> _filter;
        // Номера записей источника в порядке прокси / The source record indices in the proxy order
        std::vector<std::size_t> _map;
        mutable std::vector<std::size_t> _reverse;
        mutable bool _reverseValid = false;
    };
}

#endif
