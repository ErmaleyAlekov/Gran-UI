#include "gran/data/internal/source_readers.hpp"
#include "gran/core/i18n.hpp"

#include "gran/res/resources.hpp"

#include "beresta.hpp"

#include <filesystem>

namespace gran::data::internal
{
    namespace
    {
        // Получатель, который просто копит узлы (дерево читается целиком) / A receiver that just collects the nodes (a tree is read whole)
        class Collector final : public XmlRecordSink
        {
        public:
            void record(TreeItem item) override
            {
                items.push_back(std::move(item));
            }

            std::vector<TreeItem> items;
        };
    }

    /*
    ** Ошибка разбора «Берёсты» уже знает строку и столбец — они переходят в DataException вместе
    ** с её текстом. Исключения получателя (отмена подгрузки) проходят наружу без изменений.
    **
    ** A Beresta parse error already knows the line and the column — they go into DataException together
    ** with its text. The receiver exceptions (the loading cancel) pass outside unchanged.
    */
    void parseXmlSource(const SourceSpec& spec, XmlRecordSink& sink)
    {
        FieldPaths fields = compileFields(spec);
        XmlRecordHandler handler(spec, fields, sink);
        std::unique_ptr<std::istream> stream = res::openStream(std::filesystem::u8path(spec.path));

        // Файл диска или ресурс res:/ — SAX читает поток порциями, DOM целиком не строится / A disk file or a res:/ resource — SAX reads the stream by portions, the whole DOM is not built
        if (!*stream)
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {spec.path}));
        try
        {
            beresta::SaxParser().parse(*stream, handler);
        }
        catch (const beresta::ParseException& error)
        {
            std::string line = std::to_string(error.getLine());
            std::string column = std::to_string(error.getColumn());
            throw DataException(i18n::format(i18n::Message::DATA_SOURCE_ERROR, {spec.path, line, column,
                i18n::format(i18n::Message::DATA_XML_ERROR, {error.what()})}), spec.path, error.getLine(), error.getColumn());
        }
    }

    std::vector<TreeItem> readXmlTree(const SourceSpec& spec)
    {
        Collector collector;

        parseXmlSource(spec, collector);
        return std::move(collector.items);
    }
}
