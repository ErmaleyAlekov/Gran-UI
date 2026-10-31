#include "gran/capi/internal/bridge.hpp"

#include <optional>

// Нет атрибута — NULL, а не пустая строка: пустое значение атрибута тоже бывает / No attribute is NULL, not an empty string: an empty attribute value exists too
gran_status gran_app_markup_read(gran_app* app, const char* target, const char* name, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(target != nullptr && name != nullptr && result != nullptr);
    std::optional<std::string> value = app->application.markupAttribute(
        gran::app::requireNode(app->application, target), name);
    if (value)
        *result = gran::capi::copyString(*value);
    GRAN_CAPI_END
}

// value == NULL удаляет атрибут: одна функция на оба случая, как у gran_app_apply / value == NULL removes the attribute: one function for both cases, as with gran_app_apply
gran_status gran_app_markup_write(gran_app* app, const char* target, const char* name, const char* value)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(target != nullptr && name != nullptr);
    gran::ui::Node& node = gran::app::requireNode(app->application, target);
    if (value != nullptr)
        app->application.setMarkupAttribute(node, name, value);
    else
        app->application.removeMarkupAttribute(node, name);
    GRAN_CAPI_END
}

// Окно выбирается любым его узлом; NULL — главное / The window is chosen by any of its nodes; NULL — the main one
gran_status gran_app_markup_save(gran_app* app, const char* window, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(result != nullptr);
    const gran::ui::Node* node = nullptr;
    if (window != nullptr)
        node = &gran::app::requireNode(app->application, window);
    *result = gran::capi::copyString(app->application.saveMarkup(node));
    GRAN_CAPI_END
}
