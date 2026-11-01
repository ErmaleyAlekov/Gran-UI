#ifndef GRAN_DATA_DATA_SOURCE_HPP
#define GRAN_DATA_DATA_SOURCE_HPP

#include "gran/data/list_model.hpp"
#include "gran/data/tree_model.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::data
{
    // Формат файла источника; AUTO — по расширению (.json — JSON, иначе XML) / The source file format; AUTO — by the extension (.json — JSON, otherwise XML)
    enum class SourceFormat : std::uint8_t { AUTO, JSON, XML };

    /*
    ** Поле записи из источника: имя поля и путь к значению. XML — выражение XPath 1.0 «Берёсты»
    ** относительно элемента записи (@price, title, sum(line/@qty)); JSON — имена через точку
    ** (адрес.город).
    **
    ** A record field from a source: the field name and the path to the value. XML — a Beresta XPath 1.0
    ** expression relative to the record element (@price, title, sum(line/@qty)); JSON — dot separated names
    ** (address.city).
    */
    struct FieldSpec
    {
        std::string name;
        std::string path;
    };

    /*
    ** Описание источника <Data source="…">: файл, путь к записям select (XML — шаги / и // с именами
    ** и *, без / в начале — как //; JSON — имена через точку, пусто — корень-массив), поля (без них —
    ** атрибуты и простые дочерние элементы XML, скалярные члены JSON с развёрткой вложенных объектов
    ** в «адрес.город»), имя детей для деревьев и размер порции ленивой подгрузки (0 — всё сразу).
    **
    ** A <Data source="…"> source description: the file, the path to the records select (XML — / and // steps with names
    ** and *, without a leading / — as //; JSON — dot separated names, empty — a root array), the fields (without them —
    ** the attributes and simple child elements of XML, the scalar JSON members with nested objects unfolded
    ** into "address.city"), the children name for trees and the lazy loading portion size (0 — everything at once).
    */
    struct SourceSpec
    {
        std::string path;
        SourceFormat format = SourceFormat::AUTO;
        std::string select;
        std::vector<FieldSpec> fields;
        std::string children;
        std::size_t fetch = 0;
    };

    // ---- data_source.cpp ----

    // Формат с учётом расширения файла / The format with the file extension taken into account
    SourceFormat sourceFormat(const SourceSpec& spec);

    /*
    ** Подгрузчик записей списка. XML разбирается потоково (SAX «Берёсты»): в памяти только текущая
    ** запись; с fetch > 0 разбор идёт в рабочем потоке, который после каждой порции ждёт следующего
    ** next() — подгрузка ленивая и детерминированная. Ошибки — DataException с файлом и позицией,
    ** нет файла — GranException(IO).
    **
    ** A list record fetcher. XML is parsed as a stream (Beresta SAX): only the current record is in memory;
    ** with fetch > 0 the parsing runs in a worker thread that waits for the next next() after each portion —
    ** loading is lazy and deterministic. Errors are DataException with the file and the position,
    ** a missing file is GranException(IO).
    */
    std::shared_ptr<ListFetcher> openListSource(const SourceSpec& spec);

    // Узлы дерева из источника (всегда целиком) / The tree nodes from a source (always whole)
    std::vector<TreeItem> readTreeSource(const SourceSpec& spec);

    // Заменить содержимое модели первой порцией; остаток — через подгрузчик модели / Replace the model contents with the first portion; the rest — through the model fetcher
    void loadList(ListModel& model, const SourceSpec& spec);

    // Добавить узлы детьми parent (рекурсивно) / Add the nodes as children of parent (recursively)
    void fillTree(TreeModel& tree, NodeId parent, std::vector<TreeItem> items);
}

#endif
