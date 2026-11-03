#ifndef GRAN_SETTINGS_INTERNAL_SETTINGS_XML_HPP
#define GRAN_SETTINGS_INTERNAL_SETTINGS_XML_HPP

#include "gran/settings/settings.hpp"

#include <string>
#include <string_view>

namespace gran::settings::internal
{
    // ---- settings_read.cpp ----

    // XML настроек → модель (схема gran-settings.xsd); false — ошибка XML или схемы / The settings XML → the model (the gran-settings.xsd schema); false — an XML or schema error
    bool readSettings(std::string_view text, Settings::Model& model);

    // ---- settings_write.cpp ----

    // Модель → XML (отступы, экранирование, порядок по именам — файл удобно сравнивать) / The model → XML (indents, escaping, the order by names — the file is easy to compare)
    std::string writeSettings(const Settings::Model& model);
}

#endif
