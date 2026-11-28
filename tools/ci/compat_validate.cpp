#include "beresta.hpp"

#include <filesystem>
#include <iostream>

// Входные файлы из исторического корпуса / Input files from the historical corpus
static bool validate(const std::shared_ptr<const beresta::xsd::Schema>& schema, const char* path, bool select)
{
    beresta::CollectingErrorHandler handler;
    beresta::ParserOptions options;

    options.schema = schema;
    options.trackLocations = true;
    options.maxDepth = 256;
    options.errorHandler = &handler;
    beresta::XmlParser(options).parseFile(std::filesystem::u8path(path));
    if (select)
        return !handler.hasErrors();
    for (const auto& diagnostic : handler.getDiagnostics())
        std::cerr << path << ": " << diagnostic.message << '\n';
    return !handler.hasErrors();
}

int main(int argc, char** argv)
{
    if (argc < 4)
        return 2;
    try
    {
        const bool select = std::string(argv[1]) == "--select";
        auto schema = beresta::xsd::Schema::fromFile(std::filesystem::u8path(argv[2]));
        for (int index = 3; index < argc; ++index)
        {
            if (validate(schema, argv[index], select))
                std::cout << index - 3 << '\n';
            else if (!select)
                return 1;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
