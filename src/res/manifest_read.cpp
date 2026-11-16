#include "gran/res/manifest.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"
#include "gran/utils/file_system.hpp"

#include <filesystem>

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
        beresta::Document parseManifest(const std::string& path)
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
                // API «Берёсты» принимает std::filesystem::path: u8path сохраняет UTF-8 и на Windows
                // The Beresta API takes std::filesystem::path: u8path keeps UTF-8 on Windows as well
                dom = beresta::XmlParser(options).parseFile(std::filesystem::u8path(path));
            }
            catch (const beresta::ParseException& error)
            {
                throw MarkupException(ErrorCode::MARKUP, error.getMessage(), error.getLine(), error.getColumn());
            }
            catch (const beresta::DomException&)
            {
                throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {path}));
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
        void readEntry(const beresta::Element& element, const std::string& base, std::vector<ManifestFile>& files)
        {
            std::string path = attributeOr(element, "path", std::string());
            std::string source = fs::joinPath(base, path);
            std::string alias = attributeOr(element, "alias", path);
            fs::FileInfo info = fs::queryPath(source);

            if (element.getLocalName() == "File" && info.regular)
                files.push_back(ManifestFile{source, alias});
            else if (element.getLocalName() == "Folder" && info.directory)
                expandFolder(source, alias, attributeOr(element, "include", "*"),
                    attributeOr(element, "recursive", "true") != "false", files);
            else
                throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_MISSING_FILE, {source}));
        }
    }

    // Элементы в порядке документа, затем сортировка / The elements in the document order, then sorting
    Manifest readManifest(const std::string& path)
    {
        beresta::Document dom = parseManifest(path);
        const beresta::Element* root = dom.getRoot();
        Manifest manifest;

        manifest.name = attributeOr(*root, "name", std::string());
        manifest.location = path;
        for (const beresta::Node* node = root->getFirstChild(); node != nullptr; node = node->getNextSibling())
        {
            if (node->getType() == beresta::NodeType::ELEMENT)
                readEntry(*static_cast<const beresta::Element*>(node), fs::parentPath(path), manifest.files);
        }
        sortFiles(manifest.files);
        return manifest;
    }
}
