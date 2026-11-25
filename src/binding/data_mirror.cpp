#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    DataMirror::DataMirror()
        : _document(std::make_unique<beresta::Document>())
    {
        _document->setRoot(std::make_unique<beresta::Element>("data"));
    }

    // Записи отписываются от моделей раньше, чем уходит документ с их элементами / The entries unsubscribe from the models before the document with their elements goes
    DataMirror::~DataMirror()
    {
        _entries.clear();
    }

    const beresta::Element& DataMirror::root() const
    {
        return *_document->getRoot();
    }

    // Порядок атрибутов — порядок полей записи (словарь упорядочен по имени) / The attribute order is the record field order (the map is sorted by name)
    std::unique_ptr<beresta::Element> mirrorItem(const data::Record& record)
    {
        std::unique_ptr<beresta::Element> item = std::make_unique<beresta::Element>("Item");

        for (const auto& field : record)
        {
            try
            {
                item->setAttribute(field.first, field.second);
            }
            catch (const beresta::DomException&)
            {
                // Поле с именем, негодным для атрибута XML, выражениям не видно (как модель с таким именем)
                // A field with a name unfit for an XML attribute is not visible to expressions (as a model with such a name)
                continue;
            }
        }
        return item;
    }
}
