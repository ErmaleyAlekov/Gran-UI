#include "gran/app/app_driver.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    // Водитель годится только для окон без ОС: настоящее окно ждало бы мышь пользователя
    // The driver fits only windows without an OS: a real window would wait for the user mouse
    AppDriver::AppDriver(Application& app)
        : _app(app), _alive(app._state->driverToken)
    {
        if (!_app._state->options.headless)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_DRIVER_NOT_HEADLESS));
    }

    /*
    ** Живое приложение продолжает без водителя (как после выхода из run()); разрушенное уже сняло время «run()»
    ** само (поле driverScope), поэтому здесь его не трогаем.
    **
    ** A live application goes on without the driver (as after leaving run()); a destroyed one has already removed
    ** the "run()" time itself (the driverScope field), so it is not touched here.
    */
    AppDriver::~AppDriver()
    {
        if (!_alive.expired())
            _app._state->driverScope.reset();
    }

    // Все методы водителя приходят к состоянию только через эту проверку / All the driver methods reach the state only through this check
    Application::State& AppDriver::state() const
    {
        if (_alive.expired())
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_DRIVER_DETACHED));
        return *_app._state;
    }
}
