#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    bool ExpressionFilter::changed()
    {
        bool result = false;

        if (_bindings == nullptr)
            return false;
        for (VersionWatch& watch : _variables)
        {
            std::uint64_t version = _bindings->variableVersion(watch.name);
            if (version != watch.seen)
                result = true;
            watch.seen = version;
        }
        return result;
    }

    void ExpressionFilter::invalidate()
    {
        if (_proxy != nullptr)
            _proxy->invalidateFilter();
        if (_tree != nullptr)
            _tree->invalidateFilter();
    }

    void ExpressionFilter::detach() noexcept
    {
        _bindings = nullptr;
    }

    /*
    ** Узел контекста — отдельный элемент Item записи, а не её элемент в зеркале: фильтр зовут из
    ** уведомлений источника, и зеркало в этот момент может ещё не знать о правке. Ошибка вычисления
    ** (неверный тип) прячет запись, а не роняет список.
    **
    ** The context node is a separate Item element of the record, not its element in the mirror: the filter is called from
    ** the source notifications, and the mirror may not know about the edit at that moment yet. An evaluation error
    ** (a wrong type) hides the record instead of breaking the list.
    */
    bool ExpressionFilter::evaluate(const data::Record& record) const
    {
        beresta::xpath::XPathContext context;
        std::unique_ptr<beresta::Element> item = mirrorItem(record);

        if (_bindings == nullptr)
            return true;
        for (const VersionWatch& watch : _variables)
            context.variables.insert_or_assign(watch.name, variableValue(_bindings->variable(watch.name)));
        try
        {
            return _expression->evaluate(*item, context).toBoolean();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
}
