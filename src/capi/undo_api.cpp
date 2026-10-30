#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Состояние стека флагами GRAN_UNDO_* / The stack state as GRAN_UNDO_* flags
    uint32_t undoFlags(const undo::UndoStack& stack) noexcept
    {
        uint32_t flags = 0;

        if (stack.canUndo())
            flags |= GRAN_UNDO_CAN_UNDO;
        if (stack.canRedo())
            flags |= GRAN_UNDO_CAN_REDO;
        if (stack.isClean())
            flags |= GRAN_UNDO_CLEAN;
        return flags;
    }

    // Имя стека живёт только на время вызова / The stack name lives only during the call
    void UndoCallback::operator()(const std::string& name, const undo::UndoStack& stack) const
    {
        callback(user, name.c_str(), undoFlags(stack));
    }
}

// Шаг уже выполнен языковой стороной: стек только записывает его / The step is already done by the language side: the stack only records it
gran_status gran_app_undo_push(gran_app* app, const char* stack, const gran_undo_step* step)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(step != nullptr && step->size == sizeof(gran_undo_step));
    app->application.undoStack(gran::capi::text(stack)).push(gran::capi::callbackStep(*step));
    GRAN_CAPI_END
}

gran_status gran_app_undo_state(gran_app* app, const char* stack, uint32_t* flags)
{
    if (flags != nullptr) *flags = 0;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(flags != nullptr);
    *flags = gran::capi::undoFlags(app->application.undoStack(gran::capi::text(stack)));
    GRAN_CAPI_END
}

// Пустой callback снимает обработчик / A null callback clears the handler
gran_status gran_app_on_undo(gran_app* app, gran_undo_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    if (callback == nullptr)
        app->application.onUndoChanged(gran::UndoHandler());
    else
        app->application.onUndoChanged(gran::capi::UndoCallback{callback, user});
    GRAN_CAPI_END
}
