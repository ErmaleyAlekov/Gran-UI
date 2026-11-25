#include "gran/data/internal/observer_errors.hpp"

namespace gran::data::internal
{
    thread_local ObserverErrors* ObserverErrors::active = nullptr;

    ObserverErrors::ObserverErrors() noexcept : previous(active) { active = this; }
    ObserverErrors::~ObserverErrors() { active = previous; }

    // Вне транзакции сохраняется обычная немедленная передача исключения.
    // Outside a transaction the exception propagates immediately as usual.
    void ObserverErrors::capture()
    {
        if (!active)
            throw;
        if (!active->first)
            active->first = std::current_exception();
    }

    void ObserverErrors::check() const
    {
        if (first)
            std::rethrow_exception(first);
    }

    void ObserverErrors::checkCurrent()
    {
        if (active) active->check();
    }
}
