#include "gran/utils/mapped_file.hpp"
#include "gran/res/resources.hpp"

namespace gran
{
    /*
    ** Объект создаётся только успешным отображением. Ресурс res:/ отображать не нужно: его байты
    ** уже в памяти программы и живут до конца процесса — объект лишь ссылается на них.
    **
    ** The object is created only by a successful mapping. A res:/ resource needs no mapping: its bytes
    ** are already in the program memory and live until the process end — the object only refers to them.
    */
    std::shared_ptr<const MappedFile> MappedFile::open(const std::string& path)
    {
        std::shared_ptr<MappedFile> file(new MappedFile());
        res::ResourceView view;

        if (res::find(path, view))
        {
            file->_data = view.data;
            file->_size = view.size;
            file->_resource = true;
            return file;
        }
        if (!file->map(path))
            return nullptr;
        return file;
    }

    // Отображение снимается вместе с объектом; байты ресурса не принадлежат ему / The mapping is removed together with the object; the resource bytes do not belong to it
    MappedFile::~MappedFile()
    {
        if (!_resource)
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
