#ifndef GRAN_MARKUP_MARKUP_LOADER_HPP
#define GRAN_MARKUP_MARKUP_LOADER_HPP

#include "gran/data/list_model.hpp"
#include "gran/ui/command.hpp"
#include "gran/ui/node.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace beresta
{
    class Document;
}

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
        // Действие onClose (пусто — окно закрывается само) / The onClose action (empty — the window closes by itself)
        std::string onClose;
        platform::TitleBar titleBar = platform::TitleBar::SYSTEM;
        style::DeclarationBlock declared;
    };

    /*
    ** Загруженная разметка: окно, стили, шрифты, корень дерева и живой DOM «Берёсты». DOM —
    ** источник правды для разметки: правки атрибутов идут в него и перестраивают узлы
    ** (live_document.hpp), из него же документ сохраняется обратно в XML и сравнивается
    ** при перезагрузке. Узлы ссылаются на свои элементы (ui::Node::domElement).
    **
    ** The loaded markup: the window, styles, fonts, the tree root and the live Beresta DOM. The DOM is
    ** the source of truth for the markup: attribute edits go into it and rebuild the nodes
    ** (live_document.hpp), the document is saved back to XML from it and compared
    ** on a reload. The nodes refer to their elements (ui::Node::domElement).
    */
    struct UiDocument
    {
        WindowSpec window;
        style::StyleSheet styles;
        std::vector<FontFace> fonts;
        /*
        ** shared_ptr, а не unique_ptr: удалитель запоминается при создании, поэтому заголовок обходится
        ** объявлением beresta::Document без включения «Берёсты». Объявлен до root: поля уничтожаются
        ** в обратном порядке, и узлы уходят раньше элементов, на которые ссылаются.
        **
        ** shared_ptr, not unique_ptr: the deleter is captured at creation, so the header gets by with
        ** a beresta::Document declaration without including Beresta. Declared before root: the fields are destroyed
        ** in reverse order, and the nodes go before the elements they refer to.
        */
        std::shared_ptr<beresta::Document> dom;
        std::unique_ptr<ui::Node> root;
        std::size_t nodeCount = 0;
        // Значения темы — для фрагментов, вставляемых во время работы / The theme values — for fragments inserted at run time
        TokenMap tokens;
        // Начальные данные моделей списков / The initial data of the list models
        std::vector<DataSet> data;
        // Команды <Command> окна (enabled меняет приложение) / The window <Command> commands (the application changes enabled)
        std::vector<ui::Command> commands;
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
    **
    ** parent — будущий родитель: если он построен из разметки, элемент фрагмента сразу встаёт в его
    ** DOM (последним ребёнком-элементом), и вставка попадает в сохранённую разметку; иначе (nullptr,
    ** узел из кода) узлы фрагмента не связаны с DOM.
    ** parent is the future parent: if it is built from the markup, the fragment element goes into its
    ** DOM at once (as the last element child), and the insertion gets into the saved markup; otherwise (nullptr,
    ** a node from code) the fragment nodes are not linked to the DOM.
    */
    std::unique_ptr<ui::Node> buildFragment(std::string_view text, UiDocument& document, const ui::Node* parent = nullptr);

    // ---- markup_schema.cpp ----

    // Встроенная схема: загружается один раз, неизменяема и разделяется потоками
    // The embedded schema: loaded once, immutable and shared between threads
    std::shared_ptr<const beresta::xsd::Schema> markupSchema();
}

#endif
