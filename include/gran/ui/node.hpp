#ifndef GRAN_UI_NODE_HPP
#define GRAN_UI_NODE_HPP

#include "gran/style/style_sheet.hpp"
#include "gran/utils/geometry.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::ui
{
    // Вид элемента разметки / The markup element kind
    enum class ElementKind : std::uint8_t
    {
        STACK, BOX, GRID, CANVAS, SCROLL, TEXT, IMAGE, BUTTON, INPUT, CHECKBOX, SLIDER, TEXT_AREA, SPAN, POPUP, MENU_ITEM, WEB_VIEW
    };

    /*
    ** Клавиша отправки многострочного поля (onSubmit). CONTROL_ENTER: Enter — новая строка,
    ** Ctrl+Enter — отправка (как в редакторах); ENTER: Enter — отправка, Shift+Enter — новая строка
    ** (как в мессенджерах).
    **
    ** The submit key of a multi-line input (onSubmit). CONTROL_ENTER: Enter is a new line,
    ** Ctrl+Enter submits (as in editors); ENTER: Enter submits, Shift+Enter is a new line
    ** (as in messengers).
    */
    enum class SubmitKey : std::uint8_t { CONTROL_ENTER, ENTER };

    // Как вписать картинку в блок / How to fit an image into the box
    enum class ImageFit : std::uint8_t { FILL, CONTAIN, COVER, NONE };

    // Направление прокрутки / The scroll direction
    enum class ScrollDirection : std::uint8_t { VERTICAL, HORIZONTAL, BOTH };

    // Флаги «нужно пересчитать» / The "must recompute" flags
    enum DirtyFlag : std::uint8_t
    {
        DIRTY_STYLE = 1,
        DIRTY_LAYOUT = 2,
        DIRTY_PAINT = 4,
        DIRTY_DESCENDANT = 8
    };

    /*
    ** Содержимое элемента, не относящееся к стилю: текст, источник картинки,
    ** значения полей ввода и имена действий. Хранится плоско (без иерархии классов
    ** виджетов): у каждого вида свои поля, остальные пусты.
    **
    ** The element content not related to style: the text, the image source,
    ** input values and action names. Stored flat (without a widget class hierarchy):
    ** each kind uses its own fields, the others stay empty.
    */
    struct NodeContent
    {
        std::string text;
        std::string placeholder;
        std::string source;
        std::string tooltip;
        std::string onClick;
        std::string onChange;
        std::string onSubmit;
        // Имя модели списка (items="…"): дети строятся из шаблона по записям модели / The list model name (items="…"): children are built from the template by the model records
        std::string items;
        // Действие на строку из страницы <WebView> (Application::onWebMessage) / The action for a string from a <WebView> page (Application::onWebMessage)
        std::string onMessage;
        float minimum = 0.0f;
        float maximum = 100.0f;
        float value = 0.0f;
        float step = 0.0f;
        std::size_t maxLength = 0;
        std::size_t caret = 0;
        // Второй конец выделения (байт UTF-8); выделение — между anchor и caret, пусто при равенстве
        // The other selection end (a UTF-8 byte); the selection lies between anchor and caret, empty when equal
        std::size_t anchor = 0;
        // Наименьшее число видимых строк TextArea (как rows у <textarea>) / The least visible line count of a TextArea (as rows of <textarea>)
        std::size_t rows = 2;
        bool checked = false;
        bool password = false;
        bool readOnly = false;
        ImageFit fit = ImageFit::FILL;
        ScrollDirection direction = ScrollDirection::VERTICAL;
        SubmitKey submitKey = SubmitKey::CONTROL_ENTER;
        // Popup: рамка якоря в координатах окна (нулевая высота — точка); passive — подсказка (не ловит мышь)
        // Popup: the anchor frame in window coordinates (a zero height — a point); passive — a tooltip (does not catch the mouse)
        Rect popupAnchor;
        bool popupPassive = false;
    };

    // Запомненное измерение: доступный размер → результат / A remembered measurement: the available size → the result
    struct MeasureEntry
    {
        Size available;
        Size result;
    };

    /*
    ** Результат компоновки: рамка блока (border box) в координатах окна, размер содержимого
    ** (для прокрутки) и кэш измерений текущего прохода (номер прохода — generation).
    **
    ** The layout result: the border box in window coordinates, the content size
    ** (for scrolling) and the measurement cache of the current pass (the pass number is generation).
    */
    struct LayoutBox
    {
        Rect frame;
        Size contentSize;
        Vec2 scrollOffset;
        MeasureEntry cache[2];
        std::uint32_t generation = 0;
        std::uint8_t cacheCount = 0;
    };

    /*
    ** Узел дерева интерфейса. Родитель владеет детьми (unique_ptr), ребёнок знает
    ** родителя невладеющим указателем — как в «Берёсте», без циклов владения.
    **
    ** A UI tree node. A parent owns its children (unique_ptr), a child knows
    ** its parent through a non-owning pointer — as in Beresta, without ownership cycles.
    */
    struct Node
    {
        ElementKind kind = ElementKind::BOX;
        std::string id;
        const style::StyleRule* rule = nullptr;
        style::DeclarationBlock declared;
        NodeContent content;
        style::ComputedStyle computed;
        LayoutBox layout;
        bool visible = true;
        bool enabled = true;
        bool styled = false;
        std::uint8_t stateMask = 0;
        std::uint8_t dirty = DIRTY_STYLE | DIRTY_LAYOUT | DIRTY_PAINT;
        std::size_t line = 0;
        std::size_t column = 0;
        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
        // Шаблон одной записи списка (<ItemTemplate>) — вне дерева, только образец для копий / The template of one list record (<ItemTemplate>) — outside the tree, only a sample for copies
        std::unique_ptr<Node> itemTemplate;
    };

    // Добавляет ребёнка и возвращает его (node.cpp) / Adds a child and returns it (node.cpp)
    Node& appendChild(Node& parent, std::unique_ptr<Node> child);

    // Вставляет ребёнка перед позицией index (за концом — последним) / Inserts a child before the index position (past the end — as the last one)
    Node& insertChild(Node& parent, std::size_t index, std::unique_ptr<Node> child);

    // Имя элемента разметки ("Stack", "Button"...) / The markup element name ("Stack", "Button"...)
    const char* elementName(ElementKind kind) noexcept;

    /*
    ** Помечает узел. Предки получают DIRTY_DESCENDANT (проход стилей найдёт узел, не обходя
    ** чистые ветви), а при DIRTY_LAYOUT — и DIRTY_LAYOUT: размер ребёнка меняет размер родителя.
    **
    ** Marks the node. The ancestors get DIRTY_DESCENDANT (the style pass finds the node without walking
    ** clean branches), and with DIRTY_LAYOUT — DIRTY_LAYOUT too: a child size changes the parent size.
    */
    void markDirty(Node& node, std::uint8_t flags) noexcept;

    // Начальные состояния из разметки: enabled="false" → DISABLED, checked="true" → CHECKED (всё поддерево)
    // The initial states from the markup: enabled="false" → DISABLED, checked="true" → CHECKED (the whole subtree)
    void applyInitialStates(Node& node) noexcept;

    // ---- node_kinds.cpp ----

    // Поле правки текста: однострочное Input или многострочное TextArea / A text editing field: a single-line Input or a multi-line TextArea
    bool isTextField(const Node& node) noexcept;

    // Узел участвует в компоновке родителя: видим и не всплывающий слой (Popup размещается поверх всего)
    // The node takes part in the parent layout: visible and not a popup layer (a Popup is placed over everything)
    bool inFlow(const Node& node) noexcept;

    // Ближайший всплывающий слой среди узла и его предков (nullptr — узел не в слое) / The nearest popup layer among the node and its ancestors (nullptr — the node is not in a layer)
    Node* popupOf(Node* node) noexcept;

    // Включить или выключить узел (enabled и состояние DISABLED) / Enabling or disabling the node (enabled and the DISABLED state)
    void setEnabled(Node& node, bool enabled);

    // ---- node_tree.cpp ----

    // Узел поддерева по id, поиск в глубину (nullptr — нет такого) / A subtree node by id, depth-first (nullptr — no such node)
    Node* findNode(Node& subtree, const std::string& id) noexcept;

    /*
    ** Копия поддерева для вставки во время работы (шаблоны в разметке: скрытый образец →
    ** копия → appendChild). Копируются вид, id, стиль, объявления, содержимое, видимость, шаблон списка;
    ** вычисленный стиль, компоновка и состояния — нет: копия получает их заново при вставке.
    **
    ** A subtree copy for inserting at run time (templates in the markup: a hidden sample →
    ** a copy → appendChild). The kind, id, style, declarations, content, visibility, the list template are copied;
    ** the computed style, layout and states are not: the copy gets them anew on insertion.
    */
    std::unique_ptr<Node> cloneTree(const Node& node);

    /*
    ** Оформленный текст: дети Text вида SPAN — участки по порядку (у каждого свой стиль),
    ** текст Text — их склейка (по нему работают выделение, копирование, доступность).
    ** Есть ли у узла участки / Whether the node has runs.
    **
    ** Styled text: the SPAN children of a Text are the runs in order (each with its own style),
    ** the Text text is them joined (selection, copying, accessibility work on it).
    */
    bool hasSpans(const Node& node) noexcept;

    // Текст Text заново из участков (после их правки из кода) и перекомпоновка / The Text text anew from the runs (after editing them from code) and a relayout
    void joinSpans(Node& node);

    // ---- node_transform.cpp ----

    // CSS-преобразование узла вокруг transformOrigin (тождественное, если transform не задан)
    // The node CSS transform around transformOrigin (the identity if transform is not set)
    Affine2D nodeTransform(const Node& node) noexcept;

    // Из координат компоновки узла в координаты окна (преобразования всех предков и самого узла)
    // From the node layout coordinates into window coordinates (the transforms of all ancestors and the node itself)
    Affine2D nodeToWindow(const Node& node) noexcept;

    // Точка окна → координаты компоновки узла (для попадания, ползунка, текста) / A window point → the node layout coordinates (for hits, sliders, text)
    Vec2 windowToNode(const Node& node, Vec2 point) noexcept;
}

#endif
