#ifndef GRAN_MARKUP_MARKUP_LOADER_HPP
#define GRAN_MARKUP_MARKUP_LOADER_HPP

#include "gran/data/list_model.hpp"
#include "gran/ui/node.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace beresta::xsd
{
    class Schema;
}

namespace gran::markup
{
    // Подключаемый шрифт <Font family="…" source="…"/> / A font face <Font family="…" source="…"/>
    struct FontFace
    {
        std::string family;
        std::string source;
        int weight = 400;
        bool italic = false;
    };

    // Значения темы: имя → текст / Theme values: a name → a text
    using TokenMap = std::map<std::string, std::string, std::less<>>;

    // Данные списка из разметки <Data name="…"><Item поле="…"/></Data> / List data from the markup <Data name="…"><Item field="…"/></Data>
    struct DataSet
    {
        std::string name;
        std::vector<data::Record> records;
    };

    // Параметры окна из <Window> / The window parameters from <Window>
    struct WindowSpec
    {
        std::string title;
        float width = 800.0f;
        float height = 600.0f;
        float minWidth = 0.0f;
        float minHeight = 0.0f;
        bool resizable = true;
        style::DeclarationBlock declared;
    };

    /*
    ** Загруженная разметка: окно, стили, шрифты и корень дерева. DOM «Берёсты»
    ** после построения не хранится — дерево UI компактнее и живёт само по себе.
    **
    ** The loaded markup: the window, styles, fonts and the tree root. The Beresta DOM
    ** is not kept after building — the UI tree is more compact and lives on its own.
    */
    struct UiDocument
    {
        WindowSpec window;
        style::StyleSheet styles;
        std::vector<FontFace> fonts;
        std::unique_ptr<ui::Node> root;
        std::size_t nodeCount = 0;
        // Значения темы — для фрагментов, вставляемых во время работы / The theme values — for fragments inserted at run time
        TokenMap tokens;
        // Начальные данные моделей списков / The initial data of the list models
        std::vector<DataSet> data;
        // Каталог для относительных путей картинок (из LoadOptions) / The directory for relative image paths (from LoadOptions)
        std::string baseDirectory;
    };

    // Настройки загрузки / The loading settings
    struct LoadOptions
    {
        std::string baseDirectory;
        std::size_t maxNodes = 100000;
    };

    // ---- markup_loader.cpp ----

    /*
    ** Разбор и проверка схемой за один проход «Берёстой», затем построение дерева.
    ** Ошибки схемы собираются все сразу; MarkupException содержит позицию первой,
    ** а текст — все сообщения через перевод строки.
    **
    ** Parsing and the schema check in one Beresta pass, then building the tree.
    ** Schema errors are collected all at once; MarkupException holds the position of the first one,
    ** and the text — all messages separated by line breaks.
    */
    UiDocument loadMarkupString(std::string_view text, const LoadOptions& options);

    // То же из файла (baseDirectory по умолчанию — каталог файла) / The same from a file (baseDirectory defaults to the file directory)
    UiDocument loadMarkupFile(const std::string& path, const LoadOptions& options);

    // ---- markup_fragment.cpp ----

    /*
    ** Фрагмент разметки (один корневой элемент интерфейса) для вставки во время работы.
    ** Проверяется той же схемой, что и основной файл; стили ($значения темы, style="…")
    ** берутся из document. Позиции ошибок — относительно начала фрагмента.
    **
    ** A markup fragment (one root UI element) for inserting at run time.
    ** It is checked by the same schema as the main file; the styles ($theme values, style="…")
    ** come from document. Error positions are relative to the fragment start.
    */
    std::unique_ptr<ui::Node> buildFragment(std::string_view text, UiDocument& document);

    // ---- markup_schema.cpp ----

    // Встроенная схема: загружается один раз, неизменяема и разделяется потоками
    // The embedded schema: loaded once, immutable and shared between threads
    std::shared_ptr<const beresta::xsd::Schema> markupSchema();
}

#endif
