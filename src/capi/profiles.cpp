#include "gran/capi/internal/bridge.hpp"

// Строки имени и XML принадлежат вызывающей стороне только на время вызова.
// Name and XML strings are borrowed from the caller for the duration of the call.
gran_status gran_profile_import(gran_app* app, const char* name, const char* xml)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name && xml);
    app->application.importProfile(name, xml);
    GRAN_CAPI_END
}

// Выбор профиля использует его сохранённое имя.
// Select a profile by its persisted name.
gran_status gran_profile_select(gran_app* app, const char* name)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name);
    app->application.selectProfile(name);
    GRAN_CAPI_END
}

// Возвращаем выделенную строку; при ошибке результат остаётся нулевым.
// Return an allocated string; the result stays null on failure.
gran_status gran_profile_export(gran_app* app, const char* name, char** xml)
{
    if (xml) *xml = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name && xml);
    *xml = gran::capi::copyString(app->application.exportProfile(name));
    GRAN_CAPI_END
}

// Пустая область означает сброс всего профиля.
// An empty region resets the entire profile.
gran_status gran_profile_reset(gran_app* app, const char* scope)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    app->application.resetProfile(gran::capi::text(scope));
    GRAN_CAPI_END
}
