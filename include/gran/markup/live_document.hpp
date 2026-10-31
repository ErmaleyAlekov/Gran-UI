#ifndef GRAN_MARKUP_LIVE_DOCUMENT_HPP
#define GRAN_MARKUP_LIVE_DOCUMENT_HPP

#include "gran/markup/markup_loader.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace beresta
{
    class Element;
}

/*
** Живой документ: DOM «Берёсты» после загрузки остаётся в UiDocument, узлы знают свои элементы.
** Здесь — операции разметочного уровня: чтение и правка атрибутов с проверкой схемой, перенос
** вставок и удалений в DOM, сохранение в XML и план частичной перезагрузки. Узлы дерева окна
** меняет приложение (gran::Application): ему нужны привязки, ввод и анимации.
**
** The live document: the Beresta DOM stays in UiDocument after loading, the nodes know their elements.
** Here are the markup level operations: reading and editing attributes with the schema check, carrying
** insertions and removals into the DOM, saving to XML and the partial reload plan. The application
** (gran::Application) changes the window tree nodes: it needs the bindings, the input and the animations.
*/
namespace gran::markup
{
    /*
    ** План частичной перезагрузки. relink — узлы остаются (с состоянием, слушателями и адресом),
    ** меняется только ссылка на элемент нового DOM; rebuild — узел перестраивается на месте из нового
    ** элемента. Поддеревья rebuild не пересекаются и не содержат узлов relink.
    **
    ** The partial reload plan. relink — the nodes stay (with the state, the listeners and the address),
    ** only the link to the new DOM element changes; rebuild — the node is rebuilt in place from the new
    ** element. The rebuild subtrees do not overlap and contain no relink nodes.
    */
    struct PatchPlan
    {
        std::vector<std::pair<ui::Node*, const beresta::Element*>> relink;
        std::vector<std::pair<ui::Node*, const beresta::Element*>> rebuild;
    };

    // ---- live_dom.cpp ----

    /*
    ** Убрать элемент узла из DOM перед удалением узла из дерева; ссылки поддерева обнуляются.
    ** Узел не из разметки (копия списка, узел из кода) — ничего не происходит.
    **
    ** Remove the node element from the DOM before removing the node from the tree; the subtree links are cleared.
    ** A node not from the markup (a list copy, a node from code) — nothing happens.
    */
    void detachMarkup(ui::Node& node);

    // ---- live_attributes.cpp ----

    // Значение атрибута в разметке узла; nullopt — атрибута нет или узел не из разметки
    // The attribute value in the node markup; nullopt — no attribute or the node is not from the markup
    std::optional<std::string> markupAttribute(const ui::Node& node, std::string_view name);

    /*
    ** Правка атрибута элемента узла (value == nullopt — удалить атрибут). Весь DOM проверяется схемой,
    ** затем из элемента строится новое поддерево — его возвращают, узел в дереве не трогается: заменить
    ** его на месте — дело приложения. Ошибка схемы или значения — MarkupException, узел не из разметки —
    ** GranException(INVALID_ARGUMENT); в обоих случаях DOM остаётся прежним.
    **
    ** Editing an attribute of the node element (value == nullopt — remove the attribute). The whole DOM is checked by the schema,
    ** then a new subtree is built from the element — it is returned, the node in the tree is not touched: replacing
    ** it in place is the application's job. A schema or value error is MarkupException, a node not from the markup is
    ** GranException(INVALID_ARGUMENT); in both cases the DOM stays as it was.
    */
    std::unique_ptr<ui::Node> editMarkupAttribute(UiDocument& document, const ui::Node& node, std::string_view name,
        const std::optional<std::string>& value);

    /*
    ** Поддерево из элемента со стилями и темой document (для правки и частичной перезагрузки).
    ** Число узлов документа не меняется: новое поддерево заменяет прежнее.
    **
    ** A subtree from an element with the styles and the theme of document (for editing and the partial reload).
    ** The document node count does not change: the new subtree replaces the former one.
    */
    std::unique_ptr<ui::Node> rebuildFromElement(UiDocument& document, const beresta::Element& element);

    // ---- live_serialize.cpp ----

    // Текущий DOM документа как текст XML (с объявлением <?xml?>) / The current document DOM as XML text (with the <?xml?> declaration)
    std::string serializeMarkup(const UiDocument& document);

    // ---- live_patch.cpp ----

    /*
    ** Ребёнок добавлен во время работы, а не построен из разметки: подсказка и контекстное меню окна,
    ** узлы из кода (append). Таков ребёнок без элемента у обычного контейнера; у Tabs, ComboBox, Text
    ** и списков из данных дети без элемента строит сам загрузчик или привязка — они не в счёт. Такие дети
    ** при перестройке узла из разметки сохраняются, а сравнение при перезагрузке их пропускает.
    **
    ** The child is added at run time, not built from the markup: the window tooltip and context menu,
    ** the nodes from code (append). Such is a child without an element under an ordinary container; for Tabs, ComboBox, Text
    ** and lists from data the children without an element are built by the loader or the binding itself — they do not count. Such children
    ** are kept when the node is rebuilt from the markup, and the comparison on a reload skips them.
    */
    bool isRuntimeChild(const ui::Node& parent, const ui::Node& child) noexcept;

    /*
    ** План перехода от current к next (next — та же разметка, загруженная заново). false — различаются
    ** параметры <Window> или <Resources> (стили, тема, данные, команды): узлы current ссылаются на старые
    ** стили, частичная замена невозможна, нужна полная.
    **
    ** The plan of going from current to next (next is the same markup loaded again). false — the <Window>
    ** parameters or <Resources> differ (the styles, the theme, the data, the commands): the current nodes refer to the old
    ** styles, a partial replacement is impossible, a full one is needed.
    */
    bool planPatch(UiDocument& current, const UiDocument& next, PatchPlan& plan);
}

#endif
