#ifndef GRAN_CAPI_INTERNAL_BRIDGE_HPP
#define GRAN_CAPI_INTERNAL_BRIDGE_HPP
#include "gran/capi/gran.h"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include <map>
#include <memory>
#include <thread>

namespace gran::capi
{
    /*
    ** Подгрузчик списка из обратного вызова языка: next() зовёт fn(user, model), а тот вставляет записи
    ** сам через gran_model_insert; конец записей язык сообщает gran_model_set_more(…, 0).
    **
    ** A list fetcher from a language callback: next() calls fn(user, model), and it inserts the records
    ** itself through gran_model_insert; the language reports the end of the records with gran_model_set_more(…, 0).
    */
    class ListCallbackFetcher final : public data::ListFetcher
    {
    public:
        // ---- capi_fetchers.cpp ----
        ListCallbackFetcher(gran_fetch_fn callback, void* user, std::string model);
        bool hasMore() const override;
        std::vector<data::Record> next() override;

        bool more = true;

    private:
        gran_fetch_fn _callback;
        void* _user;
        std::string _model;
    };

    // Подгрузчик детей ленивых узлов дерева из обратного вызова языка / A lazy tree node children fetcher from a language callback
    struct TreeCallbackFetcher final : data::TreeFetcher
    {
        gran_tree_fetch_fn callback;
        void* user;
        std::string tree;
        TreeCallbackFetcher(gran_tree_fetch_fn function, void* data, std::string name);
        void fetchChildren(data::TreeModel& model, data::NodeId node) override;
    };

    // Наблюдатель выделения, сообщающий языку его имя / A selection observer telling the language its name
    struct SelectionCallback final : data::SelectionObserver
    {
        gran_selection_fn callback;
        void* user;
        std::string name;
        data::SelectionModel* model;
        void selectionChanged() override;
    };
}

// Дескриптор владеет приложением; поток проверяется до обращения к дереву. Подгрузчики и наблюдатели выделения
// объявлены после приложения: уничтожаются раньше и успевают отписаться от его моделей.
// The handle owns the application; its thread is checked before accessing the tree. The fetchers and the selection observers
// are declared after the application: they are destroyed earlier and manage to unsubscribe from its models.
struct gran_app
{
    explicit gran_app(gran::AppOptions options);
    gran::Application application;
    std::thread::id thread;
    bool running = false;
    std::map<std::string, std::shared_ptr<gran::capi::ListCallbackFetcher>> fetchers;
    std::map<std::string, std::unique_ptr<gran::capi::SelectionCallback>> selectionCallbacks;
};

namespace gran::capi
{
    struct Failure { gran_status status; std::string message; };

    // Ошибка аргумента: бросает Failure{GRAN_INVALID_ARGUMENT} / An argument error: throws Failure{GRAN_INVALID_ARGUMENT}
    [[noreturn]] void failArgument();

    /*
    ** Проверка аргумента. Шаблон — ради видимого тела: статический анализатор (clang-tidy, П.8) не видит тела
    ** функций из других файлов и считал, что после require(p != nullptr) выполнение идёт дальше с p == nullptr, —
    ** сотня ложных «разыменований нуля». С телом в заголовке он видит [[noreturn]] failArgument и путь отсекает.
    **
    ** An argument check. A template for the sake of a visible body: the static analyzer (clang-tidy, P.8) does not see the bodies
    ** of functions from other files and assumed that after require(p != nullptr) the execution goes on with p == nullptr —
    ** a hundred false "null dereferences". With the body in the header it sees [[noreturn]] failArgument and cuts the path off.
    */
    template <typename Condition>
    void require(const Condition& condition)
    {
        if (!condition)
            failArgument();
    }
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

    // ---- models_api.cpp ----

    // Описание источника из gran_source (размер структуры и поля проверяются) / A source description from gran_source (the structure size and the fields are checked)
    data::SourceSpec sourceSpec(const gran_source* source);

    // ---- tree_nodes.cpp ----

    // Номер ABI → узел ядра (0 — корень) и обратно (корень — 0) / An ABI number → a core node (0 — the root) and back (the root — 0)
    data::NodeId treeNode(const data::TreeModel& tree, uint64_t node);
    uint64_t abiNode(const data::TreeModel& tree, data::NodeId node) noexcept;

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
