#include "gran/markup/components.hpp"
#include "gran/markup/internal/components.hpp"

#include "beresta.hpp"

namespace gran::markup
{
    std::vector<ComponentSchema> componentSchemas(const UiDocument& document)
    {
        std::vector<ComponentSchema> schemas;

        if (!document.components)
            return schemas;
        for (const ComponentLibrary& library : document.components->libraries)
            schemas.push_back(ComponentSchema{library.namespaceUri, library.path, library.schemaText});
        return schemas;
    }

    std::vector<std::string> libraryFiles(const UiDocument& document)
    {
        std::vector<std::string> files;

        if (!document.components)
            return files;
        for (const ComponentLibrary& library : document.components->libraries)
            files.push_back(library.path);
        return files;
    }
}

namespace gran::markup::internal
{
    // Те же файлы в том же порядке и с тем же содержимым (сравнение деревьев, как у isEqualNode окна) / The same files in the same order with the same content (a tree comparison, as isEqualNode of the window)
    bool sameComponents(const UiDocument& current, const UiDocument& next)
    {
        std::size_t count = 0;

        if (current.components)
            count = current.components->libraries.size();
        if (!next.components)
            return count == 0;
        if (next.components->libraries.size() != count)
            return false;
        for (std::size_t index = 0; index < count; ++index)
        {
            const ComponentLibrary& left = current.components->libraries[index];
            const ComponentLibrary& right = next.components->libraries[index];
            if (left.path != right.path || !left.dom->isEqualNode(*right.dom))
                return false;
        }
        return true;
    }
}
