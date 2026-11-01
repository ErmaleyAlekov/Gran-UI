#include "gran/data/internal/source_readers.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <filesystem>
#include <utility>

namespace gran::data::internal
{
    // Путь — UTF-8 (u8path), как у разметки; массив записей ищется сразу — ошибка пути видна при загрузке / The path is UTF-8 (u8path), as for the markup; the record array is found at once — a path error is visible at loading
    JsonListFetcher::JsonListFetcher(SourceSpec spec)
        : _spec(std::move(spec)), _file(std::filesystem::u8path(_spec.path), std::ios::binary), _reader(_file, _spec.path)
    {
        if (!_file)
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {_spec.path}));
        selectJsonArray(_reader, _spec.select);
    }

    bool JsonListFetcher::hasMore() const
    {
        return !_done;
    }

    // Порция 0 — весь массив / A portion of 0 is the whole array
    std::vector<Record> JsonListFetcher::next()
    {
        std::vector<Record> portion;

        while (!_done && (_spec.fetch == 0 || portion.size() < _spec.fetch))
        {
            JsonToken token = _reader.next();
            if (token == JsonToken::END_ARRAY)
            {
                _done = true;
                break;
            }
            TreeItem item;
            readJsonItem(_reader, token, _spec, item);
            portion.push_back(std::move(item.record));
        }
        return portion;
    }

    std::vector<TreeItem> readJsonTree(const SourceSpec& spec)
    {
        std::ifstream file(std::filesystem::u8path(spec.path), std::ios::binary);
        JsonReader reader(file, spec.path);
        std::vector<TreeItem> items;

        if (!file)
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {spec.path}));
        selectJsonArray(reader, spec.select);
        for (;;)
        {
            JsonToken token = reader.next();
            if (token == JsonToken::END_ARRAY)
                return items;
            TreeItem item;
            readJsonItem(reader, token, spec, item);
            items.push_back(std::move(item));
        }
    }
}
