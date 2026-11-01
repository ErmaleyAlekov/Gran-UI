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

    void ExpressionBindings::watchModel(const std::string& name, data::ListModel& model)
    {
        _mirror.watch(name, model);
    }

    /*
    ** Сначала все двусторонние записи (пользователь изменил поле — источник получает значение),
    ** затем пересчёт: так зависимые от того же источника привязки видят новое значение в этом же кадре.
    ** Запись источника не меняет дерево (правка поля записи трогает только тексты копий), поэтому
    ** номера привязок во время прохода стабильны.
    **
    ** First all the two-way writes (the user changed the control — the source gets the value),
    ** then the recomputation: this way the bindings depending on the same source see the new value in the same frame.
    ** Writing a source does not change the tree (a record field edit touches only the copy texts), so
    ** the binding numbers are stable during the pass.
    */
    bool ExpressionBindings::update()
    {
        std::map<std::pair<const ui::Node*, std::string>, ui::Node*> found;
        EvaluationScope scope{_owner, _mirror, *this, found};
        bool changed = false;

        for (std::size_t index = 0; index < _bindings.size(); ++index)
        {
            if (!_bindings[index]->stale && _bindings[index]->source != SourceKind::NONE)
                writeBack(*_bindings[index]);
        }
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
    void ExpressionBindings::writeBack(ExpressionBinding& binding)
    {
        std::string current = readTarget(*binding.node, binding.attribute);
        std::string value = current;

        if (current == binding.applied)
            return;
        binding.applied = current;
        for (const CompiledPart& part : binding.parts)
        {
            if (part.expression && !part.converter.empty() && !toSource(part.converter, current, value))
                return;
        }
        if (binding.source == SourceKind::VARIABLE && variable(binding.sourceName) == nullptr)
            setVariable(data::Variable{binding.sourceName, targetType(*binding.node, binding.attribute), value});
        else if (binding.source == SourceKind::VARIABLE)
            setVariableText(binding.sourceName, value);
        else
        {
            ItemRef item = _owner.itemOf(*binding.node);
            if (item.model != nullptr)
                item.model->set(item.index, binding.sourceName, value);
        }
    }
}
