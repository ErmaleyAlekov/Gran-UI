#ifndef GRAN_MARKUP_COMPONENTS_HPP
#define GRAN_MARKUP_COMPONENTS_HPP

#include "gran/markup/markup_loader.hpp"

#include <string>
#include <vector>

namespace gran::markup
{
    /*
    ** Сгенерированная XSD библиотеки компонентов (targetNamespace = namespace библиотеки): элемент
    ** на компонент, параметры — атрибуты с типами gran-ui.xsd. Её можно сохранить рядом с разметкой
    ** для подсказок в редакторе XML (вместе с gran-ui.xsd, на чьи типы она ссылается).
    **
    ** The generated XSD of a component library (targetNamespace = the library namespace): an element
    ** per component, the parameters are attributes with the gran-ui.xsd types. It can be saved next to the markup
    ** for hints in an XML editor (together with gran-ui.xsd, whose types it refers to).
    */
    struct ComponentSchema
    {
        std::string namespaceUri;
        std::string file;
        std::string text;
    };

    // ---- component_public.cpp ----

    // Схемы всех подключённых библиотек по порядку <xi:include> / The schemas of all included libraries in the <xi:include> order
    std::vector<ComponentSchema> componentSchemas(const UiDocument& document);

    // Пути файлов библиотек (для наблюдения за файлами при --watch) / The library file paths (for watching the files with --watch)
    std::vector<std::string> libraryFiles(const UiDocument& document);
}

#endif
