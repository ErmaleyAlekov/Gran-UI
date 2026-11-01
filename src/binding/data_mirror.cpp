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

    /*
    ** Имя модели становится именем элемента. Модель из кода может называться как угодно; имя,
    ** которое не годится для XML, «Берёста» отвергает исключением — тогда модель просто не видна
    ** выражениям (списки items="…" с ней работают по-прежнему).
    **
    ** The model name becomes the element name. A model from code may have any name; a name
    ** unfit for XML is rejected by Beresta with an exception — then the model is simply not visible
    ** to the expressions (the items="…" lists work with it as before).
    */
    void DataMirror::watch(const std::string& name, data::ListSource& model)
    {
        beresta::Element* element = createElement(name);

        if (element != nullptr)
            _entries.push_back(std::make_unique<MirrorEntry>(name, model, *element));
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
