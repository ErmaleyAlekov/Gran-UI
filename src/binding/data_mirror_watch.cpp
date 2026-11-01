#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    void DataMirror::watchTree(const std::string& name, data::TreeModel& tree)
    {
        beresta::Element* element = createElement(name);

        if (element != nullptr)
            _entries.push_back(std::make_unique<TreeMirror>(name, tree, *element));
    }

    void DataMirror::watchSelection(const std::string& name, data::SelectionModel& selection)
    {
        beresta::Element* element = createElement(name);

        if (element != nullptr)
            _entries.push_back(std::make_unique<SelectionMirror>(name, selection, *element));
    }

    // Имя, негодное для элемента XML, «Берёста» отвергает исключением / A name unfit for an XML element is rejected by Beresta with an exception
    beresta::Element* DataMirror::createElement(const std::string& name)
    {
        std::unique_ptr<beresta::Element> created;

        try
        {
            created = std::make_unique<beresta::Element>(name);
        }
        catch (const beresta::DomException&)
        {
            return nullptr;
        }
        beresta::Element* element = created.get();
        _document->getRoot()->appendChild(std::move(created));
        return element;
    }

    void DataMirror::flush()
    {
        for (const std::unique_ptr<MirrorSlot>& entry : _entries)
        {
            TreeMirror* tree = dynamic_cast<TreeMirror*>(entry.get());
            if (tree != nullptr)
                tree->rebuild();
        }
    }
}
