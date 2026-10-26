#include "gran/utils/mapped_file.hpp"

namespace gran
{
    // Объект создаётся только успешным отображением / The object is created only by a successful mapping
    std::shared_ptr<const MappedFile> MappedFile::open(const std::string& path)
    {
        std::shared_ptr<MappedFile> file(new MappedFile());

        if (!file->map(path))
            return nullptr;
        return file;
    }

    // Отображение снимается вместе с объектом / The mapping is removed together with the object
    MappedFile::~MappedFile()
    {
        unmap();
    }

    // Байты файла / The file bytes
    const std::uint8_t* MappedFile::data() const noexcept
    {
        return _data;
    }

    // Размер файла / The file size
    std::size_t MappedFile::size() const noexcept
    {
        return _size;
    }
}
