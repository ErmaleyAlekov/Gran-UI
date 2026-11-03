#include "gran/res/manifest.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <algorithm>

namespace gran::generated
{
    extern const unsigned char GRAN_RESOURCES_SCHEMA[];
    extern const std::size_t GRAN_RESOURCES_SCHEMA_SIZE;
}

namespace gran::res
{
    namespace
    {
        /*
        ** Разбор манифеста со схемой: синтаксическая ошибка или нарушение схемы — MarkupException с позицией
        ** первой ошибки и всеми сообщениями (как у разметки окна).
        **
        ** Parsing the manifest with the schema: a syntax error or a schema violation is MarkupException with the position
        ** of the first error and all the messages (as for the window markup).
        */
        beresta::Document parseManifest(const std::filesystem::path& path)
        {
            static const std::shared_ptr<const beresta::xsd::Schema> SCHEMA = beresta::xsd::Schema::fromString(std::string_view(
                reinterpret_cast<const char*>(generated::GRAN_RESOURCES_SCHEMA), generated::GRAN_RESOURCES_SCHEMA_SIZE));
            beresta::CollectingErrorHandler handler;
            beresta::ParserOptions options;
            beresta::Document dom;

            options.schema = SCHEMA;
            options.trackLocations = true;
            options.errorHandler = &handler;
            try
            {
                dom = beresta::XmlParser(options).parseFile(path);
            }
            catch (const beresta::ParseException& error)
            {
                throw MarkupException(ErrorCode::MARKUP, error.getMessage(), error.getLine(), error.getColumn());
            }
            catch (const beresta::DomException&)
            {
                throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {path.u8string()}));
            }
            if (handler.hasErrors())
                throw MarkupException(ErrorCode::MARKUP, handler.getDiagnostics().front().message,
                    handler.getDiagnostics().front().position.line, handler.getDiagnostics().front().position.column);
            return dom;
        }

        // Атрибут или значение по умолчанию / An attribute or the default value
        std::string attributeOr(const beresta::Element& element, std::string_view name, const std::string& fallback)
        {
            std::optional<std::string> value = element.getAttribute(name);

            if (!value)
                return fallback;
            return *value;
        }

        // <File> — один файл, <Folder> — файлы папки по шаблонам; нет файла или папки — ошибка / <File> — one file, <Folder> — the folder files by patterns; a missing file or folder is an error
        void readEntry(const beresta::Element& element, const std::filesystem::path& base, std::vector<ManifestFile>& files)
        {
            std::string path = attributeOr(element, "path", std::string());
            std::filesystem::path source = base / std::filesystem::u8path(path);
            std::string alias = attributeOr(element, "alias", path);
            std::error_code error;

            if (element.getLocalName() == "File" && std::filesystem::is_regular_file(source, error))
                files.push_back(ManifestFile{source, alias});
            else if (element.getLocalName() == "Folder" && std::filesystem::is_directory(source, error))
                expandFolder(source, alias, attributeOr(element, "include", "*"),
                    attributeOr(element, "recursive", "true") != "false", files);
            else
                throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_MISSING_FILE, {source.u8string()}));
        }
    }

    // Элементы в порядке документа, затем сортировка / The elements in the document order, then sorting
    Manifest readManifest(const std::filesystem::path& path)
    {
        beresta::Document dom = parseManifest(path);
        const beresta::Element* root = dom.getRoot();
        Manifest manifest;

        manifest.name = attributeOr(*root, "name", std::string());
        manifest.location = path;
        for (const beresta::Node* node = root->getFirstChild(); node != nullptr; node = node->getNextSibling())
        {
            if (node->getType() == beresta::NodeType::ELEMENT)
                readEntry(*static_cast<const beresta::Element*>(node), path.parent_path(), manifest.files);
        }
        sortFiles(manifest.files);
        return manifest;
    }
}
