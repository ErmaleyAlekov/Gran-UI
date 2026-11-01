#include "gran/data/internal/source_readers.hpp"

#include "beresta.hpp"

namespace gran::data::internal
{
    XmlRecordHandler::XmlRecordHandler(const SourceSpec& spec, const FieldPaths& fields, XmlRecordSink& sink)
        : _spec(spec), _fields(fields), _sink(sink), _steps(parseXmlPath(spec.select))
    {
    }

    XmlRecordHandler::~XmlRecordHandler() = default;

    /*
    ** Вне записи — только путь открытых элементов. Элемент, на котором путь совпал, становится корнем
    ** документа записи; его потомки до закрытия собираются в DOM (вложенные совпадения не ищутся).
    **
    ** Outside a record — only the path of the open elements. The element where the path matched becomes the root
    ** of the record document; its descendants up to the closing are built into a DOM (nested matches are not looked for).
    */
    void XmlRecordHandler::startElement(const beresta::SaxElement& element)
    {
        std::unique_ptr<beresta::Element> created;

        if (_open.empty())
        {
            _stack.emplace_back(element.name);
            if (!matchXmlPath(_steps, _stack))
                return;
        }
        created = std::make_unique<beresta::Element>(std::string(element.name));
        for (const beresta::SaxAttribute& attribute : element.attributes)
            created->setAttribute(attribute.name, std::string(attribute.value));
        if (_open.empty())
        {
            _document = std::make_unique<beresta::Document>();
            _open.push_back(_document->setRoot(std::move(created)));
            return;
        }
        beresta::Node* added = _open.back()->appendChild(std::move(created));
        _open.push_back(static_cast<beresta::Element*>(added));
    }

    // Закрытие корня записи отдаёт её получателю и освобождает её DOM / Closing the record root hands it to the receiver and frees its DOM
    void XmlRecordHandler::endElement(const beresta::SaxElement&)
    {
        if (_open.empty())
        {
            _stack.pop_back();
            return;
        }
        _open.pop_back();
        if (!_open.empty())
            return;
        _stack.pop_back();
        TreeItem item = convertElement(*_document->getRoot(), _spec, _fields);
        _document.reset();
        _sink.record(std::move(item));
    }

    // Текст, пришедший несколькими кусками, склеивается в один узел / Text that came in several chunks is joined into one node
    void XmlRecordHandler::characters(std::string_view text)
    {
        if (_open.empty())
            return;
        beresta::Node* last = _open.back()->getLastChild();
        if (last != nullptr && last->getType() == beresta::NodeType::TEXT)
        {
            beresta::Text* node = static_cast<beresta::Text*>(last);
            node->setValue(node->getValue() + std::string(text));
            return;
        }
        _open.back()->appendChild(std::make_unique<beresta::Text>(std::string(text)));
    }

    void XmlRecordHandler::cdata(std::string_view text)
    {
        characters(text);
    }
}
