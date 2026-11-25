#include "gran/binding/internal/reload_checkpoint.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::binding
{
    ReloadCheckpoint::~ReloadCheckpoint() { owner._reload = nullptr; }

    bool ReloadCheckpoint::published(const std::string& name) const
    {
        return originalNames.count(name) != 0;
    }

    // Структурное изменение из обработчика может сохранить ссылку на временную модель.
    // A structural edit from a callback could retain a reference to a provisional model.
    void ListBindings::requireStableDocument() const
    {
        if (_reload)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_RELOAD_ACTIVE));
    }

    void ListBindings::requirePublished(const std::string& name) const
    {
        if (_reload && !_reload->published(name)) requireStableDocument();
    }
}
