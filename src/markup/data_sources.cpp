#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"

#include "beresta.hpp"

#include "gran/core/i18n.hpp"
#include "gran/style/value_scanner.hpp"

#include <charconv>
#include <limits>

#include "gran/utils/file_system.hpp"

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
            spec.path = fs::joinPath(context.options.baseDirectory, *findAttribute(element, "source"));
            if (const std::string* select = findAttribute(element, "select"))
                spec.select = *select;
            if (const std::string* children = findAttribute(element, "children"))
                spec.children = *children;
            if (const std::string* fetch = findAttribute(element, "fetch"))
                spec.fetch = readCount(element, "fetch", *fetch);
            const std::string* format = findAttribute(element, "format");
            if (format != nullptr && *format == "json")
                spec.format = data::SourceFormat::JSON;
            else if (format != nullptr)
                spec.format = data::SourceFormat::XML;
            readFields(element, spec);
            return spec;
        }
    }

    // Диапазон счётчика зависит от платформы / The counter range is platform dependent
    std::size_t readCount(const beresta::Element& element, std::string_view attribute, const std::string& text)
    {
        std::string_view digits = style::trimSpaces(text);
        const bool negative = !digits.empty() && digits.front() == '-';
        if (negative || (!digits.empty() && digits.front() == '+'))
            digits.remove_prefix(1);
        std::size_t count = 0;
        const char* end = digits.data() + digits.size();
        const auto parsed = std::from_chars(digits.data(), end, count);
        if (parsed.ec != std::errc{} || parsed.ptr != end || (negative && count != 0))
            throwAttributeError(element, attribute, i18n::format(i18n::Message::STYLE_INTEGER_RANGE,
                {text, "0", std::to_string(std::numeric_limits<std::size_t>::max())}));
        return count;
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
        const auto spec = readSpec(element, context);
        try
        {
            std::shared_ptr<data::ListFetcher> fetcher = data::openListSource(spec);
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
        const auto spec = readSpec(element, context);
        try
        {
            set.nodes = data::readTreeSource(spec);
        }
        catch (const GranException& error)
        {
            throwAttributeError(element, "source", error.getMessage());
        }
    }
}
