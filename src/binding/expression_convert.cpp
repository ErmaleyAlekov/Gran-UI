#include "gran/binding/internal/expression_binding.hpp"

#include <exception>
#include <utility>

namespace gran::binding
{
    /*
    ** Все привязки — заново: какие из них пользуются преобразователем, видно только по частям. Пересчёт
    ** принудительный, а не «первый»: правка пользователя в поле сначала уходит в источник, а не затирается.
    **
    ** All the bindings anew: which ones use the converter is seen only by their parts. The recomputation
    ** is forced, not a "first" one: a user edit in a control goes into the source first instead of being overwritten.
    */
    void ExpressionBindings::setConverter(const std::string& name, Converter converter)
    {
        _converters[name] = std::move(converter);
        for (std::unique_ptr<ExpressionBinding>& binding : _bindings)
            binding->forced = true;
    }

    // Исключение из toView ловит refresh: свойство остаётся прежним / An exception from toView is caught by refresh: the property stays as it was
    std::string ExpressionBindings::toView(const std::string& converter, const std::string& value) const
    {
        if (converter.empty())
            return value;
        auto found = _converters.find(converter);
        if (found == _converters.end() || !found->second.toView)
            return value;
        return found->second.toView(value);
    }

    /*
    ** Обратное преобразование для двусторонней привязки. Исключение из кода приложения не уходит
    ** в цикл кадра: значение просто не записывается в источник.
    **
    ** The back conversion for a two-way binding. An exception from the application code does not escape
    ** into the frame loop: the value is simply not written into the source.
    */
    bool ExpressionBindings::toSource(const std::string& converter, const std::string& value, std::string& result) const
    {
        auto found = _converters.find(converter);

        if (found == _converters.end() || !found->second.toSource)
            return false;
        try
        {
            result = found->second.toSource(value);
        }
        catch (const std::exception&)
        {
            return false;
        }
        return true;
    }
}
