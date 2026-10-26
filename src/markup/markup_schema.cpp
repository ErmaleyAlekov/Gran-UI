#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup
{
    /*
    ** Схема строится один раз при первом обращении (локальная статическая переменная
    ** инициализируется потокобезопасно). Схема неизменяема — её разделяют все окна и потоки.
    **
    ** The schema is built once on first access (a local static variable is
    ** initialized thread-safely). The schema is immutable — all windows and threads share it.
    */
    std::shared_ptr<const beresta::xsd::Schema> markupSchema()
    {
        static const std::shared_ptr<const beresta::xsd::Schema> SCHEMA = beresta::xsd::Schema::fromString(
            std::string_view(reinterpret_cast<const char*>(generated::GRAN_UI_SCHEMA), generated::GRAN_UI_SCHEMA_SIZE));

        return SCHEMA;
    }
}
