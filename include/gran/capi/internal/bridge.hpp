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
    struct Task
    {
        gran_task_fn callback;
        void* user;
        void operator()() const;
    };
    data::Record record(const gran_record* value);
}

// Единый барьер: исключения C++ никогда не пересекают ABI.
// A shared barrier: C++ exceptions never cross the ABI.
#define GRAN_CAPI_TRY try {
#define GRAN_CAPI_END return GRAN_OK; } catch (...) { return gran::capi::capture(); }
#endif
