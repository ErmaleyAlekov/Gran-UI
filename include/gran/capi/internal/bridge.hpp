#ifndef GRAN_CAPI_INTERNAL_BRIDGE_HPP
#define GRAN_CAPI_INTERNAL_BRIDGE_HPP
#include "gran/capi/gran.h"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include <thread>

// Дескриптор владеет приложением; поток проверяется до обращения к дереву.
// The handle owns the application; its thread is checked before accessing the tree.
struct gran_app
{
    explicit gran_app(gran::AppOptions options);
    gran::Application application;
    std::thread::id thread;
    bool running = false;
};

namespace gran::capi
{
    struct Failure { gran_status status; std::string message; };
    void require(bool condition);
    void checkApp(gran_app* app, bool ownerThread = true);
    const char* text(const char* value) noexcept;
    char* copyString(const std::string& value);
    gran_status capture() noexcept;

    // Функторы держат только callback и user; языковая сторона отвечает за их срок жизни.
    // Functors hold only callback and user; the language side controls their lifetime.
    struct Action
    {
        gran_app* app;
        gran_action_fn callback;
        void* user;
        void operator()(ui::Node& node) const;
        void operator()(ui::Node& node, const std::string& message) const;
        void deliver(ui::Node& node, const std::string& message) const;
    };
    // Обработчик событий интерфейса: снимок со строками-копиями, флаги возврата — в событие ядра
    // A UI event handler: a snapshot with string copies, the return flags go into the core event
    struct EventCallback
    {
        gran_app* app;
        gran_event_fn callback;
        void* user;
        void operator()(ui::Node& node, ui::UiEvent& event) const;
    };

    // Модель и номер записи списка, к которой относится узел (index == SIZE_MAX — не из списка)
    // The model and the number of the list record the node belongs to (index == SIZE_MAX — not from a list)
    void listPosition(gran_app* app, const ui::Node* node, std::string& model, size_t& index);
    struct Task
    {
        gran_task_fn callback;
        void* user;
        void operator()() const;
    };
    data::Record record(const gran_record* value);

    // ---- undo_step.cpp, undo_api.cpp ----

    // Шаг стека отмены из обратных вызовов языка (release — при отбрасывании шага) / An undo stack step of language callbacks (release — when the step is dropped)
    std::unique_ptr<undo::UndoStep> callbackStep(const gran_undo_step& step);

    // Флаги стека GRAN_UNDO_* / The GRAN_UNDO_* stack flags
    uint32_t undoFlags(const undo::UndoStack& stack) noexcept;

    // Обработчик смены стеков отмены: имя стека и флаги / An undo stack change handler: the stack name and the flags
    struct UndoCallback
    {
        gran_undo_fn callback;
        void* user;
        void operator()(const std::string& name, const undo::UndoStack& stack) const;
    };

    // ---- variables_api.cpp ----

    // Преобразователь значений из обратного вызова языка (одна сторона: toView или toSource) / A value converter from a language callback (one side: toView or toSource)
    struct ConvertCallback
    {
        gran_convert_fn callback;
        void* user;
        std::string operator()(const std::string& value) const;
    };
}

// Результат обратного вызова преобразователя: сначала — входное значение (вызов без gran_converted_set его не меняет)
// The result of a converter callback: at first the input value (a call without gran_converted_set does not change it)
struct gran_converted
{
    std::string text;
};

// Единый барьер: исключения C++ никогда не пересекают ABI.
// A shared barrier: C++ exceptions never cross the ABI.
#define GRAN_CAPI_TRY try {
#define GRAN_CAPI_END return GRAN_OK; } catch (...) { return gran::capi::capture(); }
#endif
