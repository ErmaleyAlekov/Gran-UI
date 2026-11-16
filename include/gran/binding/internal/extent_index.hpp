#ifndef GRAN_BINDING_INTERNAL_EXTENT_INDEX_HPP
#define GRAN_BINDING_INTERNAL_EXTENT_INDEX_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gran::binding
{
    /*
    ** Индекс размеров записей виртуального списка по главной оси (размер копии вместе с промежутком).
    ** Размер записи либо измерен (её копия побывала в окне и была скомпонована), либо неизвестен —
    ** тогда вместо него берётся оценка: средний размер измеренных записей. Так строки разной высоты
    ** стоят на своих местах, а записи, которых ещё не видели, занимают правдоподобное место.
    **
    ** Почему дерево Фенвика, а не массив накопленных сумм: при прокрутке измеряются новые записи,
    ** и каждое измерение должно обновлять суммы за O(log n), а не O(n). Узел дерева хранит сумму
    ** измеренных размеров и их количество на своём отрезке — сумма с оценкой считается из обоих
    ** чисел, поэтому смена оценки не требует перестройки. Вставка и удаление записей (редкие
    ** по сравнению с прокруткой) помечают дерево устаревшим, и оно строится заново за O(n)
    ** при следующем запросе.
    **
    ** The record size index of a virtual list along the main axis (a copy size together with the gap).
    ** A record size is either measured (its copy has been in the window and was laid out) or unknown —
    ** then an estimate is used instead: the average size of the measured records. This way rows of different
    ** heights stand in their places, and the records not seen yet take a plausible place.
    **
    ** Why a Fenwick tree and not an array of prefix sums: while scrolling new records get measured,
    ** and each measurement must update the sums in O(log n), not O(n). A tree node keeps the sum
    ** of the measured sizes and their count on its range — the sum with the estimate is computed from both
    ** numbers, so changing the estimate needs no rebuild. Inserting and removing records (rare
    ** compared to scrolling) marks the tree stale, and it is rebuilt in O(n)
    ** on the next query.
    */
    class ExtentIndex final
    {
    public:
        // ---- extent_index.cpp ----

        // Число записей / The record count
        std::size_t size() const noexcept;

        // Вставить count записей неизвестного размера перед index / Insert count records of unknown size before index
        void insert(std::size_t index, std::size_t count);

        // Удалить записи [index, index + count) (выход за конец обрезается) / Remove the records [index, index + count) (going past the end is clipped)
        void erase(std::size_t index, std::size_t count);

        /*
        ** Перестановка записей: старая запись i встаёт на место oldToNew[i]. Отображение
        ** другого размера (не от этого списка) сбрасывает все размеры в неизвестные.
        **
        ** A permutation of records: the old record i moves to the place oldToNew[i]. A mapping
        ** of another size (not from this list) resets all sizes to unknown.
        */
        void permute(const std::vector<std::size_t>& oldToNew);

        // ---- extent_index_sizes.cpp ----

        // Измеренный размер записи (отрицательный считается нулём; номер вне списка — без действия) / The measured size of a record (a negative one counts as zero; an index outside the list — no action)
        void set(std::size_t index, float extent);

        // Забыть размер записи (её содержимое сменилось вне окна) / Forget a record size (its content changed outside the window)
        void forget(std::size_t index);

        // Размер записи: измеренный или оценка / A record size: the measured one or the estimate
        float sizeAt(std::size_t index) const;

        // Средний размер измеренных записей (0 — ничего не измерено) / The average size of the measured records (0 — nothing is measured)
        float estimate() const;

        // ---- extent_index_offsets.cpp ----

        // Начало записи index — сумма размеров записей перед ней (index > size() — как size()) / The start of record index — the sum of the record sizes before it (index > size() — as size())
        double offsetOf(std::size_t index) const;

        /*
        ** Номер записи, на которую приходится координата offset: наибольший i, у которого
        ** offsetOf(i) <= offset. Координата до начала — 0, после конца — последняя запись;
        ** у пустого списка — 0.
        **
        ** The index of the record the coordinate offset falls on: the greatest i with
        ** offsetOf(i) <= offset. A coordinate before the start — 0, after the end — the last record;
        ** for an empty list — 0.
        */
        std::size_t indexAt(double offset) const;

        // Размер всего списка / The size of the whole list
        double total() const;

    private:
        // Сумма измеренных размеров и их количество на отрезке / The sum of the measured sizes and their count on a range
        struct Prefix
        {
            double sum = 0.0;
            std::size_t known = 0;
        };

        // ---- extent_index_tree.cpp ----

        // Перестроить дерево по _sizes, если оно устарело (O(n)) / Rebuild the tree by _sizes if it is stale (O(n))
        void build() const;

        // Добавить к позиции дерева position (с единицы) приращение суммы и изменить количество на known (−1, 0, +1)
        // Add the sum increment to the tree position position (one-based) and change the count by known (−1, 0, +1)
        void add(std::size_t position, double sum, int known);

        // Сумма и количество измеренных среди первых count записей / The sum and count of the measured ones among the first count records
        Prefix prefix(std::size_t count) const;

        // Размеры записей; отрицательный — неизвестен / The record sizes; a negative one is unknown
        std::vector<float> _sizes;
        // Дерево Фенвика с единицы: суммы измеренных и их количество (mutable — строится лениво из const-запросов).
        // Количество — size_t, как и число записей: узел не переполнится ни на каком списке (ревью Т-60)
        // The one-based Fenwick tree: the measured sums and their counts (mutable — built lazily from const queries).
        // The count is size_t, as the record count is: a node cannot overflow on any list (review T-60)
        mutable std::vector<double> _treeSum;
        mutable std::vector<std::size_t> _treeKnown;
        mutable bool _stale = true;
    };
}

#endif
