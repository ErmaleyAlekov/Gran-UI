#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"

#include "beresta.hpp"

#include <cstdlib>
#include <filesystem>

namespace gran::markup::internal
{
    namespace
    {
        // Поля <Field name path> источника / The <Field name path> fields of the source
        void readFields(const beresta::Element& element, data::SourceSpec& spec)
        {
            for (const beresta::Element* child = firstElementChild(element); child != nullptr; child = nextElementSibling(*child))
            {
                if (child->getLocalName() == "Field")
                    spec.fields.push_back(data::FieldSpec{*findAttribute(*child, "name"), *findAttribute(*child, "path")});
            }
        }

        /*
        ** Описание источника из атрибутов source/select/children/fetch/format и детей <Field name path>.
        ** Относительный путь — от каталога файла разметки, как у картинок и шрифтов.
        **
        ** The source description from the source/select/children/fetch/format attributes and the <Field name path> children.
        ** A relative path is from the markup file directory, as for images and fonts.
        */
        data::SourceSpec readSpec(const beresta::Element& element, const BuildContext& context)
        {
            data::SourceSpec spec;
            std::filesystem::path path = std::filesystem::u8path(*findAttribute(element, "source"));

            if (path.is_relative() && !context.options.baseDirectory.empty())
                path = std::filesystem::u8path(context.options.baseDirectory) / path;
            spec.path = path.u8string();
            if (const std::string* select = findAttribute(element, "select"))
                spec.select = *select;
            if (const std::string* children = findAttribute(element, "children"))
                spec.children = *children;
            if (const std::string* fetch = findAttribute(element, "fetch"))
                spec.fetch = static_cast<std::size_t>(std::strtoul(fetch->c_str(), nullptr, 10));
            const std::string* format = findAttribute(element, "format");
            if (format != nullptr && *format == "json")
                spec.format = data::SourceFormat::JSON;
            else if (format != nullptr)
                spec.format = data::SourceFormat::XML;
            readFields(element, spec);
            return spec;
        }
    }

    // Ошибка файла становится ошибкой разметки с позицией элемента <Data> / A file error becomes a markup error with the <Data> element position
    void readListData(const beresta::Element& element, BuildContext& context, DataSet& set)
    {
        set.kind = DataKind::LIST;
        set.columns = readColumns(element);
        if (findAttribute(element, "source") == nullptr)
        {
            for (const data::TreeItem& node : readNodes(element))
                set.records.push_back(node.record);
            return;
        }
        try
        {
            std::shared_ptr<data::ListFetcher> fetcher = data::openListSource(readSpec(element, context));
            set.records = fetcher->next();
            if (fetcher->hasMore())
                set.fetcher = std::move(fetcher);
        }
        catch (const GranException& error)
        {
            throwAttributeError(element, "source", error.getMessage());
        }
    }

    void readTreeData(const beresta::Element& element, BuildContext& context, DataSet& set)
    {
        set.kind = DataKind::TREE;
        if (findAttribute(element, "source") == nullptr)
        {
            set.nodes = readNodes(element);
            return;
        }
        try
        {
            set.nodes = data::readTreeSource(readSpec(element, context));
        }
        catch (const GranException& error)
        {
            throwAttributeError(element, "source", error.getMessage());
        }
    }
}
