#ifndef GRAN_DATA_TREE_LIST_HPP
#define GRAN_DATA_TREE_LIST_HPP

#include "gran/data/record_order.hpp"
#include "gran/data/tree_model.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace gran::data
{
    // Фильтр узлов дерева (паттерн «Стратегия»): код, обёртка или выражение XPath / A tree node filter (the Strategy pattern): code, a wrapper or an XPath expression
    class NodeFilter
    {
    public:
        virtual ~NodeFilter() = default;

        // Подходит ли узел сам по себе (предки подходящих показываются всегда) / Whether the node matches by itself (the ancestors of matching ones are always shown)
        virtual bool accepts(const TreeModel& tree, NodeId node) const = 0;
    };

    /*
    ** Плоское представление дерева: видимые (раскрытые) узлы подряд — будущие строки TreeView.
    ** Это обычный источник списка, поэтому items="…" с <ItemTemplate>, виртуальные списки, прокси
    ** и выделение работают с деревом без изменений; глубина и признаки строки — depth/expanded/
    ** hasChildren (в выражениях — $depth, $expanded, $hasChildren). Братья сортируются ключами,
    ** фильтр оставляет подходящие узлы и всех их предков (предки при смене фильтра раскрываются).
    ** Любая правка дерева пересчитывает строки и рассылает только разницу: вставка, удаление,
    ** перестановка или правка затронутых строк.
    **
    ** A flat tree view: the visible (expanded) nodes in a row — the future TreeView rows.
    ** It is an ordinary list source, so items="…" with <ItemTemplate>, virtual lists, proxies
    ** and the selection work with a tree without changes; the row depth and flags are depth/expanded/
    ** hasChildren (in expressions — $depth, $expanded, $hasChildren). Siblings are sorted by keys,
    ** the filter keeps the matching nodes and all their ancestors (the ancestors are expanded when the filter changes).
    ** Any tree edit recomputes the rows and delivers only the difference: an insertion, a removal,
    ** a permutation or an edit of the affected rows.
    */
    class TreeList final : public ListSource, private TreeObserver
    {
    public:
        // ---- tree_list.cpp ----

        // Дерево должно жить дольше представления / The tree must outlive the view
        explicit TreeList(TreeModel& tree);
        ~TreeList() override;
        TreeList(const TreeList&) = delete;
        TreeList& operator=(const TreeList&) = delete;

        std::size_t size() const noexcept override;
        const Record& at(std::size_t index) const override;
        void set(std::size_t index, const std::string& field, std::string value) override;

        // ---- tree_list_query.cpp ----

        // Дерево представления / The view tree
        TreeModel& tree() const noexcept;

        // Узел строки, её глубина (0 — верхний уровень), раскрыта ли, есть ли дети / The row node, its depth (0 — the top level), whether it is expanded, whether it has children
        NodeId node(std::size_t row) const;
        std::size_t depth(std::size_t row) const;
        bool expanded(std::size_t row) const;
        bool hasChildren(std::size_t row) const;

        // ---- tree_list_expand.cpp ----

        // Строка узла; size_t(-1) — узел не виден / The node row; size_t(-1) — the node is not visible
        std::size_t rowOf(NodeId node) const noexcept;

        // Раскрыть (ленивый узел подгружает детей), свернуть, переключить строку / Expand (a lazy node fetches its children), collapse, toggle a row
        void expand(std::size_t row);
        void collapse(std::size_t row);
        void toggle(std::size_t row);

        // ---- tree_list_levels.cpp ----

        // Раскрыть узлы первых levels уровней, остальные свернуть (0 — всё свёрнуто) / Expand the nodes of the first levels levels, collapse the others (0 — everything is collapsed)
        void expandToDepth(std::size_t levels);

        // ---- tree_list_order.cpp ----

        // Ключи сортировки братьев / The sibling sort keys
        void setSort(SortKeys keys);
        const SortKeys& sort() const noexcept;

        // Фильтр узлов (nullptr — все) и пересчёт после смены его условий / The node filter (nullptr — all) and a recomputation after its conditions changed
        void setFilter(std::shared_ptr<NodeFilter> filter);
        void invalidateFilter();

    private:
        // Строка представления / A view row
        struct Row
        {
            NodeId node = 0;
            std::uint32_t depth = 0;
            bool expanded = false;
            bool hasChildren = false;
        };

        // Сравнитель братьев для std::stable_sort / The sibling comparer for std::stable_sort
        struct SiblingOrder
        {
            const TreeModel* tree;
            const SortKeys* keys;
            bool operator()(NodeId left, NodeId right) const;
        };

        // ---- tree_list_observe.cpp ----
        void inserted(NodeId parent, std::size_t row, std::size_t count) override;
        void removed(NodeId parent, std::size_t row, std::size_t count) override;
        void moved(NodeId node, NodeId oldParent, std::size_t oldRow) override;
        void changed(NodeId node) override;

        // ---- tree_list_order.cpp ----

        // Пересчитать строки и разослать разницу / Recompute the rows and deliver the difference
        void refresh();

        // ---- tree_list_build.cpp ----

        // Видимые строки по текущему дереву / The visible rows by the current tree
        std::vector<Row> build();

        // Строки детей parent с глубиной depth (рекурсивно по раскрытым) / The rows of the parent children at depth (recursively over the expanded ones)
        void appendChildren(std::vector<Row>& rows, NodeId parent, std::uint32_t depth);

        // Дети узла в порядке показа, без отфильтрованных / The node children in display order, without the filtered out ones
        std::vector<NodeId> visibleChildren(NodeId parent) const;

        // Пометить узлы, которые подходят сами или через потомков; true — узел остаётся / Mark the nodes that match by themselves or through descendants; true — the node stays
        bool keep(NodeId node);

        // ---- tree_list_publish.cpp ----

        // Разослать разницу между прежними и новыми строками / Deliver the difference between the former and the new rows
        void publish(std::vector<Row> rows);

        // Разослать changed строкам, у которых сменились глубина или признаки / Deliver changed to the rows whose depth or flags changed
        void publishFlags(const std::vector<Row>& old, const std::vector<std::size_t>& oldToNew);

        TreeModel& _tree;
        std::vector<Row> _rows;
        std::unordered_set<NodeId> _expanded;
        SortKeys _keys;
        std::shared_ptr<NodeFilter> _filter;
        // Итог фильтра для узлов при последнем пересчёте (только с фильтром) / The filter result for the nodes at the last recomputation (only with a filter)
        std::unordered_map<NodeId, bool> _kept;
    };
}

#endif
