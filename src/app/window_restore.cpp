#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"

#include <exception>

namespace gran::app
{
    /*
    ** setMarkupAttribute при ошибке схемы бросает MarkupException и оставляет документ прежним — поэтому после
    ** пропуска интерфейс цел. Раньше исключение уходило из openWindows, и программа не запускалась вовсе.
    **
    ** setMarkupAttribute throws MarkupException on a schema error and leaves the document as it was — so after
    ** skipping the interface is intact. Previously the exception left openWindows, and the program did not start at all.
    */
    void restoreAttribute(Application& application, ui::Node& node, const std::string& name, const std::string& value)
    {
        try
        {
            application.setMarkupAttribute(node, name, value);
        }
        catch (const GranException& error)
        {
            log::write(log::Level::WARNING, "settings",
                i18n::format(i18n::Message::SETTINGS_ATTRIBUTE_SKIPPED, {node.id, name, error.getMessage()}));
        }
        // Стандартное исключение из разбора (не GranException) тоже не должно срывать запуск (ревью Т-66)
        // A standard exception from parsing (not GranException) must not break the start either (review T-66)
        catch (const std::exception& error)
        {
            log::write(log::Level::WARNING, "settings",
                i18n::format(i18n::Message::SETTINGS_ATTRIBUTE_SKIPPED, {node.id, name, error.what()}));
        }
    }
}
