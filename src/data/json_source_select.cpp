#include "gran/data/internal/source_readers.hpp"
#include "gran/core/i18n.hpp"

namespace gran::data::internal
{
    namespace
    {
        // Найти член объекта по имени, пропуская остальные (читатель встаёт перед его значением) / Find an object member by name skipping the others (the reader stops before its value)
        bool findMember(JsonReader& reader, const std::string& name)
        {
            for (;;)
            {
                JsonToken token = reader.next();
                if (token != JsonToken::NAME)
                    return false;
                if (reader.value() == name)
                    return true;
                reader.skip(reader.next());
            }
        }
    }

    /*
    ** "data.items": корень — объект, в нём член data, в нём член items — массив записей. Пустой путь —
    ** корень сам массив. Члены до нужного пропускаются без разбора в записи: файл читается один раз.
    **
    ** "data.items": the root is an object, the data member in it, the items member in that — the record array. An empty path —
    ** the root is an array itself. The members before the needed one are skipped without parsing into records: the file is read once.
    */
    void selectJsonArray(JsonReader& reader, const std::string& select)
    {
        JsonToken token = reader.next();
        std::size_t start = 0;

        while (!select.empty() && start <= select.size())
        {
            std::size_t dot = select.find('.', start);
            if (dot == std::string::npos)
                dot = select.size();
            if (token != JsonToken::BEGIN_OBJECT || !findMember(reader, select.substr(start, dot - start)))
                reader.fail(i18n::format(i18n::Message::DATA_SELECT_MISSING, {select}));
            token = reader.next();
            start = dot + 1;
        }
        if (token != JsonToken::BEGIN_ARRAY)
            reader.fail(i18n::format(i18n::Message::DATA_SELECT_NOT_ARRAY, {select}));
    }
}
