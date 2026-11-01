#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <cstdlib>
#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // <Variable name type value>: тип по умолчанию — строка; значение тоже не подставляется темой / <Variable name type value>: the default type is a string; the value is not substituted by the theme either
        void readVariable(const beresta::Element& element, BuildContext& context)
        {
            data::Variable variable;
            const std::string* type = findAttribute(element, "type");
            const std::string* value = findAttribute(element, "value");

            variable.name = *findAttribute(element, "name");
            if (type != nullptr && *type == "number")
                variable.type = data::VariableType::NUMBER;
            else if (type != nullptr && *type == "boolean")
                variable.type = data::VariableType::BOOLEAN;
            if (value != nullptr)
                variable.value = *value;
            context.document.variables.push_back(std::move(variable));
        }

        // Режим выделения по слову схемы (по умолчанию extended) / The selection mode by the schema word (extended by default)
        data::SelectionMode selectionMode(const std::string* word) noexcept
        {
            if (word == nullptr)
                return data::SelectionMode::EXTENDED;
            if (*word == "none")
                return data::SelectionMode::NONE;
            if (*word == "single")
                return data::SelectionMode::SINGLE;
            if (*word == "multi")
                return data::SelectionMode::MULTI;
            return data::SelectionMode::EXTENDED;
        }

        /*
        ** Представления (<TreeList tree>, <SortFilter source>, <Selection model>): основа по имени и
        ** настройки; сами объекты создаются при передаче данных в модели (ListBindings::adoptData).
        **
        ** The views (<TreeList tree>, <SortFilter source>, <Selection model>): the base by name and
        ** the settings; the objects themselves are created when the data goes into the models (ListBindings::adoptData).
        */
        void readView(const beresta::Element& element, std::string_view kind, DataSet& set)
        {
            const char* base = "source";
            const std::string* expanded = findAttribute(element, "expanded");

            set.kind = DataKind::SORT_FILTER;
            if (kind == "TreeList")
            {
                set.kind = DataKind::TREE_LIST;
                base = "tree";
            }
            else if (kind == "Selection")
            {
                set.kind = DataKind::SELECTION;
                base = "model";
            }
            set.source = *findAttribute(element, base);
            if (const std::string* sort = findAttribute(element, "sort"))
                set.sort = *sort;
            if (const std::string* filter = findAttribute(element, "filter"))
                set.filter = *filter;
            if (expanded != nullptr)
                set.expanded = static_cast<std::size_t>(std::strtoul(expanded->c_str(), nullptr, 10));
            set.mode = selectionMode(findAttribute(element, "mode"));
        }
    }

    /*
    ** Данные — это текст приложения, а не оформление: значения темы ($имя) в записях не подставляются.
    ** <Data> и <Tree> читают записи или файл, представления — только свои настройки.
    **
    ** The data is the application text, not the styling: theme values ($name) are not substituted in the records.
    ** <Data> and <Tree> read the records or a file, the views — only their settings.
    */
    void readDataSet(const beresta::Element& data, BuildContext& context)
    {
        std::string_view kind = data.getLocalName();
        DataSet set;

        if (kind == "Variable")
        {
            readVariable(data, context);
            return;
        }
        set.name = *findAttribute(data, "name");
        if (kind == "Data")
            readListData(data, context, set);
        else if (kind == "Tree")
            readTreeData(data, context, set);
        else
            readView(data, kind, set);
        context.document.data.push_back(std::move(set));
    }
}
