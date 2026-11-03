#include "gran/res/internal/registry.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstring>

namespace gran::res
{
    namespace
    {
        /*
        ** Читатель блока «GRANRES1»: число записей (u32), затем для каждой длина пути (u32), путь,
        ** длина данных (u64), данные; числа — little-endian. Выход за конец — испорченный блок.
        **
        ** The "GRANRES1" block reader: the entry count (u32), then for each the path length (u32), the path,
        ** the data length (u64), the data; the numbers are little-endian. Going past the end is a damaged block.
        */
        struct BlobReader
        {
            const unsigned char* data;
            std::size_t size;
            std::size_t position;

            // Число из bytes байт little-endian / A number of bytes little-endian bytes
            std::uint64_t number(std::size_t bytes)
            {
                std::uint64_t value = 0;

                if (size - position < bytes)
                    throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_BAD_BLOB));
                for (std::size_t index = 0; index < bytes; ++index)
                    value |= static_cast<std::uint64_t>(data[position + index]) << (8 * index);
                position += bytes;
                return value;
            }

            // Пропустить length байт и вернуть их начало / Skip length bytes and return their start
            const unsigned char* take(std::uint64_t length)
            {
                const unsigned char* start = data + position;

                if (size - position < length)
                    throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_BAD_BLOB));
                position += static_cast<std::size_t>(length);
                return start;
            }
        };
    }

    /*
    ** Блок копируется и разбирается целиком до подключения: испорченный блок не оставляет
    ** половины записей. Копия переходит во владение реестра.
    **
    ** The block is copied and parsed whole before mounting: a damaged block does not leave
    ** half of the entries. The copy goes into the registry ownership.
    */
    void mountBlob(const void* data, std::size_t size)
    {
        auto copy = std::make_unique<std::vector<unsigned char>>(static_cast<const unsigned char*>(data),
            static_cast<const unsigned char*>(data) + size);
        BlobReader reader{copy->data(), copy->size(), 8};
        std::map<std::string, ResourceView, std::less<>> entries;

        if (size < 8 || std::memcmp(copy->data(), "GRANRES1", 8) != 0)
            throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_BAD_BLOB));
        for (std::uint64_t count = reader.number(4); count > 0; --count)
        {
            std::uint64_t pathSize = reader.number(4);
            const unsigned char* path = reader.take(pathSize);
            std::uint64_t dataSize = reader.number(8);
            const unsigned char* bytes = reader.take(dataSize);
            entries[normalize(std::string(reinterpret_cast<const char*>(path), pathSize))]
                = ResourceView{bytes, static_cast<std::size_t>(dataSize)};
        }
        internal::Registry& state = internal::registry();
        std::lock_guard<std::mutex> lock(state.mutex);
        for (std::pair<const std::string, ResourceView>& entry : entries)
            state.entries[entry.first] = entry.second;
        state.blobs.push_back(std::move(copy));
    }
}
