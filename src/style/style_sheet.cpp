#include "gran/style/style_sheet.hpp"

#include <utility>

namespace gran::style
{
    // Правило создаётся в куче: адрес не меняется при росте вектора
    // The rule is created on the heap: its address does not change when the vector grows
    StyleRule& StyleSheet::add(std::string name)
    {
        _rules.push_back(std::make_unique<StyleRule>());
        _rules.back()->name = std::move(name);
        return *_rules.back();
    }

    // Поиск по имени (стилей немного, поиск только при загрузке)
    // A lookup by name (there are few styles, the lookup happens only at load time)
    StyleRule* StyleSheet::find(std::string_view name) const noexcept
    {
        for (const std::unique_ptr<StyleRule>& rule : _rules)
        {
            if (rule->name == name)
                return rule.get();
        }
        return nullptr;
    }

    // Представления имён живут, пока живут правила / The name views live as long as the rules do
    std::vector<std::string_view> StyleSheet::names() const
    {
        std::vector<std::string_view> result;

        result.reserve(_rules.size());
        for (const std::unique_ptr<StyleRule>& rule : _rules)
            result.push_back(rule->name);
        return result;
    }

    // Наборы хранятся по значению: аниматор ищет их по имени, а не держит указатель
    // The sets are stored by value: the animator looks them up by name and does not keep a pointer
    KeyframesRule& StyleSheet::addKeyframes(std::string name)
    {
        _keyframes.push_back(KeyframesRule{});
        _keyframes.back().name = std::move(name);
        return _keyframes.back();
    }

    // Поиск по имени (наборов немного, поиск — при смене стиля узла) / A lookup by name (there are few sets, the lookup happens on a node style change)
    const KeyframesRule* StyleSheet::findKeyframes(std::string_view name) const noexcept
    {
        for (const KeyframesRule& rule : _keyframes)
        {
            if (rule.name == name)
                return &rule;
        }
        return nullptr;
    }
}
