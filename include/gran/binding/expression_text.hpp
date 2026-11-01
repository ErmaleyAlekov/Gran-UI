#ifndef GRAN_BINDING_EXPRESSION_TEXT_HPP
#define GRAN_BINDING_EXPRESSION_TEXT_HPP

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gran::binding
{
    /*
    ** Часть текста атрибута с выражениями: литерал (двойные {{ и }} уже заменены на { и })
    ** или выражение — то, что стоит между { и }, вместе с необязательным «; преобразователь».
    **
    ** A part of an attribute text with expressions: a literal ({{ and }} already replaced by { and })
    ** or an expression — what stands between { and }, together with an optional "; converter".
    */
    struct TextPart
    {
        std::string text;
        bool expression = false;
    };

    /*
    ** Преобразователь значения (как IValueConverter в WPF): toView — из значения выражения в текст
    ** свойства, toSource — обратно для двусторонней привязки (пустой — привязка только односторонняя).
    ** Вызывается в потоке интерфейса; исключение из него оставляет свойство прежним.
    **
    ** A value converter (as IValueConverter in WPF): toView — from the expression value to the property text,
    ** toSource — back for a two-way binding (empty — the binding is one-way only).
    ** It is called in the UI thread; an exception from it leaves the property as it was.
    */
    struct Converter
    {
        std::function<std::string(const std::string&)> toView;
        std::function<std::string(const std::string&)> toSource;
    };

    // ---- expression_text.cpp ----

    // Имя поля записи (NCName без $, @, скобок и пробелов): {name} — поле, а не выражение XPath / A record field name (an NCName without $, @, brackets and spaces): {name} is a field, not an XPath expression
    bool isFieldName(std::string_view text) noexcept;

    // Всё значение — одно выражение «{…}» (не «{{») / The whole value is one "{…}" expression (not "{{")
    bool isWholeExpression(std::string_view value) noexcept;

    /*
    ** Есть ли в тексте выражение, а не только подстановки полей {поле}: такие тексты вычисляет
    ** привязка выражений, а прежние шаблоны {поле} остаются быстрой подстановкой списков.
    **
    ** Whether the text has an expression and not only {field} placeholders: such texts are computed
    ** by the expression binding, while the former {field} templates stay the fast list substitution.
    */
    bool hasExpressions(std::string_view value) noexcept;

    // Разбить текст на литералы и выражения; незакрытая «{» остаётся литералом / Split a text into literals and expressions; an unclosed "{" stays a literal
    std::vector<TextPart> splitText(std::string_view value);

    // ---- expression_check.cpp ----

    // Отделить «; преобразователь» от выражения (вне строковых литералов) / Separate "; converter" from the expression (outside string literals)
    void splitConverter(std::string_view part, std::string& expression, std::string& converter);

    // Скомпилировать все выражения текста; ошибка — GranException(MARKUP) с текстом из каталога / Compile all text expressions; an error is GranException(MARKUP) with a catalog text
    void checkExpressionText(std::string_view value);
}

#endif
