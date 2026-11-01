#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

#include <exception>

namespace gran::binding
{
    namespace
    {
        // Тип новой переменной, созданной двусторонней привязкой: отметка — логическая, ползунок — число / The type of a new variable created by a two-way binding: a mark is boolean, a slider is a number
        data::VariableType targetType(const ui::Node& node, const std::string& attribute) noexcept
        {
            if (attribute == "checked")
                return data::VariableType::BOOLEAN;
            if (attribute == "value" && node.kind == ui::ElementKind::SLIDER)
                return data::VariableType::NUMBER;
            return data::VariableType::STRING;
        }
    }

    /*
    ** Сначала все двусторонние записи (пользователь изменил поле — источник получает значение), затем
    ** фильтры-выражения представлений, чьи переменные сменились, затем пересчёт: так зависимые от того же источника привязки видят новое значение в этом же кадре.
    ** Записи сначала собираются, потом применяются: запись в поле записи за прокси может спрятать или
    ** переставить строку, а с ней уничтожить копию шаблона и её привязки — посреди прохода номера сдвинулись бы.
    **
    ** First all the two-way writes (the user changed the control — the source gets the value), then the expression
    ** filters of the views whose variables changed, then the recomputation: this way the bindings depending on the same source see the new value in the same frame.
    ** The writes are collected first and applied then: a write into a record field behind a proxy may hide or
    ** move the row, and with it destroy the template copy and its bindings — the indices would shift in the middle of the pass.
    */
    bool ExpressionBindings::update()
    {
        std::map<std::pair<const ui::Node*, std::string>, ui::Node*> found;
        EvaluationScope scope{_owner, _mirror, *this, found};
        bool changed = false;
        std::vector<PendingWrite> writes;

        for (std::size_t index = 0; index < _bindings.size(); ++index)
        {
            if (!_bindings[index]->stale && _bindings[index]->source != SourceKind::NONE)
                collectWrite(*_bindings[index], writes);
        }
        for (const PendingWrite& write : writes)
            applyWrite(write);
        refreshFilters();
        _mirror.flush();
        for (std::size_t index = 0; index < _bindings.size(); ++index)
        {
            ExpressionBinding& binding = *_bindings[index];
            if (binding.stale || binding.forced || dependenciesChanged(scope, binding))
                changed = refresh(scope, binding) || changed;
        }
        return changed;
    }

    /*
    ** Ошибка вычисления (исключение преобразователя, неверный тип в XPath) оставляет свойство прежним:
    ** интерфейс не падает из-за одного выражения, а синтаксис уже проверен при загрузке.
    **
    ** An evaluation error (a converter exception, a wrong type in XPath) leaves the property as it was:
    ** the interface does not fail because of one expression, and the syntax was already checked at load time.
    */
    bool ExpressionBindings::refresh(EvaluationScope& scope, ExpressionBinding& binding)
    {
        bool first = binding.stale || binding.forced;
        std::string value;

        binding.stale = false;
        binding.forced = false;
        try
        {
            value = evaluateBinding(scope, binding);
        }
        catch (const std::exception&)
        {
            return false;
        }
        if (!first && value == binding.applied)
            return false;
        bool changed = applyTarget(*binding.node, binding.attribute, value);
        binding.applied = value;
        if (binding.source != SourceKind::NONE)
            binding.applied = readTarget(*binding.node, binding.attribute);
        return changed;
    }

    // Преобразователь без toSource делает привязку односторонней / A converter without toSource makes the binding one-way
    void ExpressionBindings::collectWrite(ExpressionBinding& binding, std::vector<PendingWrite>& writes)
    {
        std::string current = readTarget(*binding.node, binding.attribute);
        PendingWrite write;

        if (current == binding.applied)
            return;
        binding.applied = current;
        write.value = current;
        for (const CompiledPart& part : binding.parts)
        {
            if (part.expression && !part.converter.empty() && !toSource(part.converter, current, write.value))
                return;
        }
        write.source = binding.source;
        write.name = binding.sourceName;
        write.type = targetType(*binding.node, binding.attribute);
        write.item = _owner.itemOf(*binding.node);
        writes.push_back(std::move(write));
    }

    // Новая переменная получает тип по полю, которое её создало / A new variable gets the type by the control that created it
    void ExpressionBindings::applyWrite(const PendingWrite& write)
    {
        if (write.source == SourceKind::VARIABLE && variable(write.name) == nullptr)
            setVariable(data::Variable{write.name, write.type, write.value});
        else if (write.source == SourceKind::VARIABLE)
            setVariableText(write.name, write.value);
        else if (write.item.model != nullptr && write.item.index < write.item.model->size())
            write.item.model->set(write.item.index, write.name, write.value);
    }
}
