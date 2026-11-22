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
        writeChanges();
        refreshFilters();
        _mirror.flush();
        const auto readers = snapshot();
        for (const auto& reader : *readers)
        {
            if (!reader->node) continue;
            const auto version = _treeVersion;
            ExpressionBinding& binding = *reader;
            if (binding.stale || binding.forced || dependenciesChanged(scope, binding))
                changed = refresh(scope, binding) || changed;
            if (version != _treeVersion) found.clear();
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
        const bool stale = binding.stale;
        bool first = stale || binding.forced;
        std::string value;

        binding.stale = false;
        binding.forced = false;
        try
        {
            value = evaluateBinding(scope, binding);
        }
        catch (const std::exception&)
        {
            binding.stale = stale;
            binding.forced = true;
            return false;
        }
        if (!binding.node || (!first && value == binding.applied))
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
        if (!binding.node) return;
        write.source = binding.source;
        write.name = binding.sourceName;
        write.type = targetType(*binding.node, binding.attribute);
        write.binding = &binding;
        writes.push_back(std::move(write));
    }

    // Новая переменная получает тип по полю, которое её создало / A new variable gets the type by the control that created it
    void ExpressionBindings::applyWrite(const PendingWrite& write)
    {
        if (!write.binding->node) return;
        if (write.source == SourceKind::VARIABLE && variable(write.name) == nullptr)
            setVariable(data::Variable{write.name, write.type, write.value});
        else if (write.source == SourceKind::VARIABLE)
            setVariableText(write.name, write.value);
        else
        {
            const ItemRef item = _owner.itemOf(*write.binding->node);
            if (item.model && item.index < item.model->size())
                item.model->set(item.index, write.name, write.value);
        }
    }
}
