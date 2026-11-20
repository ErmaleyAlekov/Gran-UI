#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Биты modifiers C ABI копируются из события как есть: значения обязаны совпадать с platform::MODIFIER_*
    // The C ABI modifiers bits are copied from the event as they are: the values must match platform::MODIFIER_*
    static_assert(static_cast<int>(GRAN_MODIFIER_SHIFT) == static_cast<int>(platform::MODIFIER_SHIFT), "GRAN_MODIFIER_SHIFT");
    static_assert(static_cast<int>(GRAN_MODIFIER_MOD) == static_cast<int>(platform::MODIFIER_CONTROL), "GRAN_MODIFIER_MOD");
    static_assert(static_cast<int>(GRAN_MODIFIER_ALT) == static_cast<int>(platform::MODIFIER_ALT), "GRAN_MODIFIER_ALT");
    static_assert(static_cast<int>(GRAN_MODIFIER_META) == static_cast<int>(platform::MODIFIER_META), "GRAN_MODIFIER_META");

    namespace
    {
        // Копии строк события: переживают удаление узлов самим callback / The event string copies: they survive node removal by the callback itself
        struct EventStrings
        {
            std::string type, target, current, related, detail, format, data, model, relatedModel;
            // cellEdit: копии, потому что gran_event_set_cell меняет строки события ядра / cellEdit: copies, because gran_event_set_cell changes the core event strings
            std::string cellField, cellOld, cellValue;
        };

        /*
        ** Снимок и событие ядра, чей callback идёт сейчас в этом потоке: по ним gran_event_set_cell находит событие.
        ** Вложенный callback (обработчик сам вызвал dispatch) запоминает прежнюю пару и возвращает её.
        **
        ** The snapshot and the core event whose callback runs now on this thread: gran_event_set_cell finds the event by them.
        ** A nested callback (the handler itself called dispatch) remembers the former pair and restores it.
        */
        struct CurrentEvent
        {
            const gran_input_event* snapshot = nullptr;
            ui::UiEvent* event = nullptr;
        };

        thread_local CurrentEvent current;

        // Идентификатор узла или пустая строка / The node identifier or an empty string
        std::string nodeId(const ui::Node* node)
        {
            if (node == nullptr) return std::string();
            return node->id;
        }

        // Числа и строки снимка; строки указывают в копии strings / The snapshot numbers and strings; the strings point into the strings copies
        gran_input_event snapshot(const ui::UiEvent& event, const EventStrings& strings)
        {
            gran_input_event out{};
            out.size = sizeof(gran_input_event);
            out.phase = static_cast<int32_t>(event.phase);
            out.type = strings.type.c_str(); out.target = strings.target.c_str();
            out.current = strings.current.c_str(); out.related = strings.related.c_str();
            out.key = platform::keyName(event.key); out.detail = strings.detail.c_str();
            out.drag_format = strings.format.c_str(); out.drag_data = strings.data.c_str();
            out.model = strings.model.c_str(); out.related_model = strings.relatedModel.c_str();
            out.x = event.local.x; out.y = event.local.y;
            out.window_x = event.position.x; out.window_y = event.position.y;
            out.wheel_x = event.wheel.x; out.wheel_y = event.wheel.y;
            out.button = static_cast<int32_t>(event.button); out.clicks = event.clicks;
            out.modifiers = event.modifiers; out.dropped = event.dropped ? 1 : 0;
            out.cell_row = SIZE_MAX;
            if (event.type == ui::EventType::CELL_EDIT) out.cell_row = event.cell.row;
            out.cell_field = strings.cellField.c_str(); out.cell_old = strings.cellOld.c_str();
            out.cell_value = strings.cellValue.c_str();
            return out;
        }
    }

    // Ближайший предок со списком items даёт имя модели (как у Action) / The nearest ancestor with an items list gives the model name (as for Action)
    void listPosition(gran_app* app, const ui::Node* node, std::string& model, size_t& index)
    {
        index = SIZE_MAX;
        if (node == nullptr) return;
        ItemRef item = app->application.itemOf(*node);
        if (item.model == nullptr) return;
        index = item.index;
        for (const ui::Node* parent = node->parent; parent != nullptr; parent = parent->parent)
        {
            if (!parent->content.items.empty()) { model = parent->content.items; return; }
        }
    }

    // Наружу не передаётся Node*: только id и места в списках; флаги возврата — методы события ядра
    // No Node* crosses outside: only the ids and the list places; the return flags are the core event methods
    void EventCallback::operator()(ui::Node& node, ui::UiEvent& event) const
    {
        if (callback == nullptr) return;
        EventStrings strings{event.name, nodeId(event.target), node.id, nodeId(event.related), event.detail,
            event.drag.format, event.drag.data, std::string(), std::string(), event.cell.field, event.cell.oldValue,
            event.cell.value};
        size_t index = SIZE_MAX;
        size_t relatedIndex = SIZE_MAX;
        listPosition(app, &node, strings.model, index);
        listPosition(app, event.related, strings.relatedModel, relatedIndex);
        gran_input_event out = snapshot(event, strings);
        out.index = index; out.related_index = relatedIndex;
        CurrentEvent outer = current;
        current = CurrentEvent{&out, &event};
        int32_t flags = 0;
        // Callback программы на C++ может бросить: пара возвращается и тогда, иначе она указывала бы на снятый кадр стека (ревью Т-75)
        // A C++ program callback may throw: the pair is restored then too, otherwise it would point at a gone stack frame (review T-75)
        try
        {
            flags = callback(user, &out);
        }
        catch (...) { current = outer; throw; }
        current = outer;
        if ((flags & GRAN_EVENT_PREVENT_DEFAULT) != 0) event.preventDefault();
        if ((flags & GRAN_EVENT_STOP_PROPAGATION) != 0) event.stopPropagation();
        if ((flags & GRAN_EVENT_STOP_IMMEDIATE) != 0) event.stopImmediatePropagation();
    }

    // Снимок сравнивается по адресу: строки и числа снимка указывают в копии, живущие только внутри callback
    // The snapshot is compared by address: the snapshot strings and numbers point into copies living only inside the callback
    ui::UiEvent* eventOf(const gran_input_event* snapshot) noexcept
    {
        if (snapshot == nullptr || snapshot != current.snapshot) return nullptr;
        return current.event;
    }
}
