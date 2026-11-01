#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    void SelectionCallback::selectionChanged()
    {
        callback(user, name.c_str());
    }
}

gran_status gran_tree_expand(gran_app* app, const char* view, size_t row, int32_t action)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(view != nullptr && action >= -1 && action <= 1);
    gran::data::TreeList& rows = app->application.treeList(view);
    if (action == 1)
        rows.expand(row);
    else if (action == 0)
        rows.collapse(row);
    else
        rows.toggle(row);
    GRAN_CAPI_END
}

/*
** Команда — одна операция модели выделения; CLICK — правила щелчка с модификаторами из last (1 Shift, 2 Ctrl).
** A command is one selection model operation; CLICK — the click rules with the modifiers from last (1 Shift, 2 Ctrl).
*/
gran_status gran_selection_change(gran_app* app, const char* selection, int32_t command, size_t index, size_t last)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(selection != nullptr && command >= GRAN_SELECTION_SELECT && command <= GRAN_SELECTION_CLICK);
    gran::data::SelectionModel& model = app->application.selection(selection);
    if (command == GRAN_SELECTION_SELECT) model.select(index);
    else if (command == GRAN_SELECTION_DESELECT) model.deselect(index);
    else if (command == GRAN_SELECTION_TOGGLE) model.toggle(index);
    else if (command == GRAN_SELECTION_RANGE) model.selectRange(index, last);
    else if (command == GRAN_SELECTION_CLEAR) model.clear();
    else if (command == GRAN_SELECTION_ALL) model.selectAll();
    else if (command == GRAN_SELECTION_CURRENT) model.setCurrent(index);
    else model.activate(index, (last & 2) != 0, (last & 1) != 0);
    GRAN_CAPI_END
}

// Два вызова: сначала узнать число (indices == NULL), затем прочитать номера / Two calls: first learn the count (indices == NULL), then read the indices
gran_status gran_selection_read(gran_app* app, const char* selection, size_t* current, size_t* indices, size_t* count)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(selection != nullptr && count != nullptr);
    gran::data::SelectionModel& model = app->application.selection(selection);
    std::vector<size_t> marked = model.selected();
    if (current != nullptr)
        *current = model.current();
    for (size_t index = 0; indices != nullptr && index < marked.size() && index < *count; ++index)
        indices[index] = marked[index];
    *count = marked.size();
    GRAN_CAPI_END
}

// Прежний наблюдатель этой модели отписывается и заменяется / The former observer of this model unsubscribes and is replaced
gran_status gran_selection_on_change(gran_app* app, const char* selection, gran_selection_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(selection != nullptr);
    gran::data::SelectionModel& model = app->application.selection(selection);
    std::unique_ptr<gran::capi::SelectionCallback>& slot = app->selectionCallbacks[selection];
    if (slot != nullptr)
        slot->model->unsubscribe(slot.get());
    slot.reset();
    if (callback == nullptr)
        return GRAN_OK;
    slot = std::make_unique<gran::capi::SelectionCallback>();
    slot->callback = callback;
    slot->user = user;
    slot->name = selection;
    slot->model = &model;
    model.subscribe(slot.get());
    GRAN_CAPI_END
}
