#ifndef GRAN_DATA_TREE_MODEL_HPP
#define GRAN_DATA_TREE_MODEL_HPP

#include "gran/data/list_source.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace gran::data
{
    // Стабильный номер узла дерева: не меняется при вставках, удалениях и перемещениях; 0 — нет узла
    // A stable tree node number: it does not change on insertions, removals and moves; 0 — no node
    using NodeId = std::uint64_t;

    /*
    ** Наблюдатель дерева. Уведомление приходит после изменения; родитель и номер — места в дереве
    ** (у удалённых узлов номера уже нет, поэтому removed сообщает место, где они были).
    **
    ** A tree observer. A notification comes after the change; the parent and the row are places in the tree
    ** (the removed nodes have no number any more, so removed reports the place where they were).
    */
    class TreeObserver
    {
    public:
        virtual ~TreeObserver() = default;

        // У parent вставлено count детей начиная с row / count children were inserted into parent starting at row
        virtual void inserted(NodeId parent, std::size_t row, std::size_t count) = 0;

        // У parent удалено count детей начиная с row (вместе с поддеревьями) / count children of parent were removed starting at row (with their subtrees)
        virtual void removed(NodeId parent, std::size_t row, std::size_t count) = 0;

        // Узел node переехал из (oldParent, oldRow) на новое место / The node moved from (oldParent, oldRow) to a new place
        virtual void moved(NodeId node, NodeId oldParent, std::size_t oldRow) = 0;

        // Поля узла изменились (или признак «дети не загружены») / The node fields changed (or the "children not loaded" flag)
        virtual void changed(NodeId node) = 0;
    };

    class TreeModel;

    // Узел дерева из источника, разметки или кода: запись и дети (пакетная вставка) / A tree node from a source, the markup or code: the record and the children (a batch insertion)
    struct TreeItem
    {
        Record record;
        std::vector<TreeItem> children;
    };

    // Подгрузчик детей ленивого узла (паттерн «Стратегия»): вставляет детей сам / A lazy node children fetcher (the Strategy pattern): it inserts the children itself
    class TreeFetcher
    {
    public:
        virtual ~TreeFetcher() = default;

        // Загрузить детей node (вызывается один раз при первом раскрытии) / Load the children of node (called once on the first expansion)
        virtual void fetchChildren(TreeModel& model, NodeId node) = 0;
    };

    /*
    ** Модель дерева: узлы с записями (как у списка) и стабильными номерами. Невидимый корень root()
    ** держит узлы верхнего уровня. Ленивый узел («дети есть, но не загружены») показывает раскрытие;
    ** первое раскрытие зовёт подгрузчик. Работает только в потоке интерфейса.
    **
    ** A tree model: nodes with records (as in a list) and stable numbers. The invisible root root()
    ** holds the top level nodes. A lazy node ("there are children, but they are not loaded") shows an expander;
    ** the first expansion calls the fetcher. It works only in the UI thread.
    */
    class TreeModel
    {
    public:
        // ---- tree_model.cpp ----
        TreeModel();

        // Невидимый корень / The invisible root
        NodeId root() const noexcept;

        // Есть ли такой узел / Whether there is such a node
        bool contains(NodeId node) const noexcept;

        // Число детей и ребёнок по номеру (ошибки — GranException) / The child count and a child by row (errors are GranException)
        std::size_t childCount(NodeId node) const;
        NodeId child(NodeId node, std::size_t row) const;

        // ---- tree_model_query.cpp ----

        // Родитель (у корня — 0) и номер среди детей родителя / The parent (0 for the root) and the row among the parent children
        NodeId parent(NodeId node) const;
        std::size_t row(NodeId node) const;

        // Запись узла / The node record
        const Record& record(NodeId node) const;

        // Ленивый ли узел; есть ли дети (загруженные или ленивые) / Whether the node is lazy; whether it has children (loaded or lazy)
        bool lazy(NodeId node) const;
        bool hasChildren(NodeId node) const;

        // ---- tree_model_edit.cpp ----

        // Вставить узел ребёнком parent перед row (row == childCount — в конец) / Insert a node as a child of parent before row (row == childCount — at the end)
        NodeId insert(NodeId parent, std::size_t row, Record record);

        // Добавить ребёнка в конец / Append a child at the end
        NodeId append(NodeId parent, Record record);

        // Удалить узел с поддеревом (корень не удаляется) / Remove a node with its subtree (the root is not removed)
        void remove(NodeId node);

        // Сменить поле записи узла / Change a field of the node record
        void set(NodeId node, const std::string& field, std::string value);

        // ---- tree_model_clear.cpp ----

        // Удалить всех детей узла / Remove all children of the node
        void clearChildren(NodeId node);

        /*
        ** Добавить поддеревья детьми parent в конец одним уведомлением inserted(parent, row, items.size()):
        ** загрузка большого дерева не пересчитывает представления и зеркало на каждый узел.
        **
        ** Append subtrees as children of parent at the end with one inserted(parent, row, items.size()) notification:
        ** loading a big tree does not recompute the views and the mirror for every node.
        */
        void appendItems(NodeId parent, std::vector<TreeItem> items);

        // ---- tree_model_move.cpp ----

        // Перенести узел к newParent на место row (в новом порядке); в своё поддерево — ошибка / Move a node to newParent at row (in the new order); into its own subtree — an error
        void move(NodeId node, NodeId newParent, std::size_t row);

        // Удалить все узлы (подгрузчик остаётся) / Remove all nodes (the fetcher stays)
        void clear();

        // Признак «дети не загружены» / The "children not loaded" flag
        void setLazy(NodeId node, bool lazy);

        // Подгрузчик детей ленивых узлов / The fetcher of the lazy node children
        void setFetcher(std::shared_ptr<TreeFetcher> fetcher);

        // Загрузить детей ленивого узла (не ленивый — ничего) / Load the children of a lazy node (not lazy — nothing)
        void fetchChildren(NodeId node);

        // ---- tree_model_observers.cpp ----

        // Подписка как у списка: без владения, отписка возможна во время уведомления / Subscription as for a list: not owned, unsubscribing during a notification is allowed
        void subscribe(TreeObserver* observer);
        void unsubscribe(TreeObserver* observer) noexcept;
        bool subscribed(const TreeObserver* observer) const noexcept;

    private:
        // Узел хранилища / A storage node
        struct Node
        {
            Record record;
            NodeId parent = 0;
            std::vector<NodeId> children;
            bool lazy = false;
        };

        // ---- tree_model_clear.cpp ----

        // Стереть поддерево из хранилища (без уведомлений) / Erase a subtree from the storage (without notifications)
        void erase(NodeId id);

        // Вписать поддерево ребёнком parent (без уведомлений) / Attach a subtree as a child of parent (without notifications)
        void attach(NodeId parent, TreeItem item);

        // ---- tree_model_observers.cpp ----

        // Узел по номеру; нет такого — GranException(INVALID_ARGUMENT) / A node by number; none — GranException(INVALID_ARGUMENT)
        const Node& node(NodeId id) const;

        /*
        ** Вид уведомления и рассылка по копии списка подписчиков: INSERTED/REMOVED — (родитель, номер, число),
        ** MOVED — (узел, прежний родитель, прежний номер), CHANGED — (узел).
        **
        ** The notification kind and delivery over a copy of the subscriber list: INSERTED/REMOVED — (parent, row, count),
        ** MOVED — (node, former parent, former row), CHANGED — (node).
        */
        enum class Event : std::uint8_t { INSERTED, REMOVED, MOVED, CHANGED };
        void notify(Event event, NodeId node, NodeId other, std::size_t row, std::size_t count);

        std::unordered_map<NodeId, Node> _nodes;
        std::vector<TreeObserver*> _observers;
        std::shared_ptr<TreeFetcher> _fetcher;
        NodeId _next = 2;
    };
}

#endif
