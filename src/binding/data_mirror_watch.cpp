#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::binding
{
    namespace
    {
        using Pending = std::map<std::string, const void*>;
        struct Registration
        {
            Pending& pending;
            Pending::iterator position;
            ~Registration() { pending.erase(position); }
        };

        [[noreturn]] void duplicateName(const std::string& name)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::DATA_MODEL_KIND, {name, "mirror"}));
        }
    }

    template<class Slot, class Source>
    void DataMirror::watchSource(const std::string& name, Source& source)
    {
        for (const auto& entry : _entries)
        {
            if (entry->name != name)
                continue;
            auto* slot = dynamic_cast<Slot*>(entry.get());
            if (slot && slot->observes(source))
                return;
            duplicateName(name);
        }
        auto element = createElement(name);
        if (!element)
            return;
        const auto pending = _registering.emplace(name, &source);
        if (!pending.second)
        {
            if (pending.first->second == &source)
                return;
            duplicateName(name);
        }
        Registration registration{_registering, pending.first};
        auto slot = std::make_unique<Slot>(name, source, *element);
        _entries.reserve(_entries.size() + 1);
        _document->getRoot()->appendChild(std::move(element));
        _entries.push_back(std::move(slot));
    }

    void DataMirror::watch(const std::string& name, data::ListSource& model)
    {
        watchSource<MirrorEntry>(name, model);
    }

    void DataMirror::watchTree(const std::string& name, data::TreeModel& tree)
    {
        watchSource<TreeMirror>(name, tree);
    }

    void DataMirror::watchSelection(const std::string& name, data::SelectionModel& selection)
    {
        watchSource<SelectionMirror>(name, selection);
    }

    // Имя, негодное для элемента XML, «Берёста» отвергает исключением / A name unfit for an XML element is rejected by Beresta with an exception
    std::unique_ptr<beresta::Element> DataMirror::createElement(const std::string& name)
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
        return created;
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
