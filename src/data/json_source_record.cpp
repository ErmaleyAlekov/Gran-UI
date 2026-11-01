#include "gran/data/internal/source_readers.hpp"

#include <utility>

namespace gran::data::internal
{
    namespace
    {
        void flatten(JsonReader& reader, const std::string& prefix, const SourceSpec& spec, TreeItem& item);

        // Массив детей узла дерева: каждый элемент — узел со своими полями и детьми / The children array of a tree node: each element is a node with its own fields and children
        void readChildren(JsonReader& reader, const SourceSpec& spec, TreeItem& item)
        {
            for (;;)
            {
                JsonToken token = reader.next();
                if (token == JsonToken::END_ARRAY)
                    return;
                TreeItem child;
                readJsonItem(reader, token, spec, child);
                item.children.push_back(std::move(child));
            }
        }

        /*
        ** Члены объекта становятся полями: вложенный объект разворачивается в «адрес.город», массивы
        ** пропускаются (кроме члена детей узла дерева на верхнем уровне записи), null — пустое поле.
        **
        ** The object members become fields: a nested object unfolds into "address.city", arrays
        ** are skipped (except the tree node children member at the top level of the record), null is an empty field.
        */
        void flatten(JsonReader& reader, const std::string& prefix, const SourceSpec& spec, TreeItem& item)
        {
            for (;;)
            {
                if (reader.next() != JsonToken::NAME)
                    return;
                std::string key = prefix + reader.value();
                JsonToken value = reader.next();
                bool children = prefix.empty() && !spec.children.empty() && key == spec.children;
                if (value == JsonToken::BEGIN_ARRAY && children)
                    readChildren(reader, spec, item);
                else if (value == JsonToken::BEGIN_OBJECT)
                    flatten(reader, key + ".", spec, item);
                else if (value == JsonToken::BEGIN_ARRAY)
                    reader.skip(value);
                else
                    item.record[key] = reader.value();
            }
        }
    }

    // Элемент — не объект: скаляр становится полем value, вложенный массив пропускается / An element that is not an object: a scalar becomes the value field, a nested array is skipped
    void readJsonItem(JsonReader& reader, JsonToken first, const SourceSpec& spec, TreeItem& item)
    {
        if (first == JsonToken::BEGIN_OBJECT)
            flatten(reader, std::string(), spec, item);
        else if (first == JsonToken::BEGIN_ARRAY)
            reader.skip(first);
        else
            item.record["value"] = reader.value();
        item.record = mapFields(spec, std::move(item.record));
    }

    // Поле без значения в источнике — пустое (как отсутствующий атрибут) / A field without a value in the source is empty (as an absent attribute)
    Record mapFields(const SourceSpec& spec, Record record)
    {
        Record result;

        if (spec.fields.empty())
            return record;
        for (const FieldSpec& field : spec.fields)
        {
            auto found = record.find(field.path);
            std::string& slot = result[field.name];
            if (found != record.end())
                slot = found->second;
        }
        return result;
    }
}
