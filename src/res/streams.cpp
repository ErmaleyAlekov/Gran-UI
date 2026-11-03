#include "gran/res/internal/registry.hpp"

#include <filesystem>
#include <fstream>

namespace gran::res
{
    namespace
    {
        // Поток над видом ресурса: буфер живёт вместе с потоком / A stream over a resource view: the buffer lives together with the stream
        class ResourceStream : public std::istream
        {
        public:
            explicit ResourceStream(const ResourceView& view)
                : std::istream(nullptr), _buffer(view.data, view.size)
            {
                rdbuf(&_buffer);
            }

        private:
            internal::MemoryBuffer _buffer;
        };
    }

    namespace internal
    {
        // Область чтения — весь вид; запись не поддерживается / The read area is the whole view; writing is not supported
        MemoryBuffer::MemoryBuffer(const unsigned char* data, std::size_t size)
        {
            char* begin = const_cast<char*>(reinterpret_cast<const char*>(data));

            setg(begin, begin, begin + size);
        }

        // Перемотка от начала, текущего места или конца; за границами — ошибка / Rewinding from the start, the current place or the end; outside the bounds — an error
        MemoryBuffer::pos_type MemoryBuffer::seekoff(off_type offset, std::ios_base::seekdir direction,
            std::ios_base::openmode mode)
        {
            off_type base = 0;

            if ((mode & std::ios_base::in) == 0)
                return pos_type(off_type(-1));
            if (direction == std::ios_base::cur)
                base = gptr() - eback();
            else if (direction == std::ios_base::end)
                base = egptr() - eback();
            if (base + offset < 0 || base + offset > egptr() - eback())
                return pos_type(off_type(-1));
            setg(eback(), eback() + base + offset, egptr());
            return pos_type(base + offset);
        }

        // Абсолютная позиция — та же перемотка от начала / An absolute position — the same rewinding from the start
        MemoryBuffer::pos_type MemoryBuffer::seekpos(pos_type position, std::ios_base::openmode mode)
        {
            return seekoff(off_type(position), std::ios_base::beg, mode);
        }
    }

    // Ненайденный ресурс — поток в состоянии fail, как у несуществующего файла / A missing resource is a stream in the fail state, as for a missing file
    std::unique_ptr<std::istream> openStream(const std::filesystem::path& path)
    {
        ResourceView view;
        std::string text = path.u8string();
        std::unique_ptr<std::istream> stream;

        if (find(text, view))
            return std::make_unique<ResourceStream>(view);
        stream = std::make_unique<std::ifstream>(path, std::ios::binary);
        if (isResourcePath(text))
            stream->setstate(std::ios::failbit);
        return stream;
    }
}
