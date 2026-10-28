#include "gran/render/image_store.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/image/image.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>

namespace gran::render
{
    // Каталог разметки — база для относительных путей / The markup directory is the base for relative paths
    ImageStore::ImageStore(rhi::RenderBackend& backend, std::string baseDirectory)
        : _backend(backend), _base(std::move(baseDirectory))
    {
    }

    // Текстуры возвращаются бэкенду / The textures are returned to the backend
    ImageStore::~ImageStore()
    {
        for (const auto& entry : _entries)
        {
            if (!entry.second.failed)
                _backend.releaseTexture(entry.second.slot);
        }
    }

    // Из кэша или загрузкой / From the cache or by loading
    const ImageEntry& ImageStore::get(const std::string& source)
    {
        auto found = _entries.find(source);

        if (found == _entries.end())
            found = _entries.emplace(source, load(source)).first;
        return found->second;
    }

    /*
    ** Чтение файла, декодирование (PNG или JPEG — по подписи), текстура RGBA8 с предумноженной альфой. Ошибка
    ** чтения, формата или нехватка слотов — запись failed (окно продолжает работать).
    **
    ** Reading the file, decoding (PNG or JPEG — by the signature), an RGBA8 texture with premultiplied alpha. A read
    ** or format error or running out of slots — a failed entry (the window keeps working).
    */
    ImageEntry ImageStore::load(const std::string& source)
    {
        std::filesystem::path path = std::filesystem::path(_base) / source;
        std::ifstream file(path, std::ios::binary);
        std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        ImageEntry entry;
        image::Image picture;

        entry.failed = true;
        if (!file && data.empty())
            return entry;
        try
        {
            picture = image::decodeImage(data.data(), data.size(), source);
            entry.slot = _backend.createTexture(rhi::TextureDesc{picture.width, picture.height, rhi::TextureFormat::RGBA8,
                "gran.image"});
        }
        catch (const GranException&)
        {
            return entry;
        }
        // Область — вся текстура только что созданного размера: обновление не может быть отвергнуто
        // The region is the whole texture of the size just created: the update cannot be rejected
        _backend.updateTexture(entry.slot, rhi::TextureRegion{0, 0, picture.width, picture.height},
            picture.pixels.data(), picture.width * 4);
        entry = ImageEntry{entry.slot, picture.width, picture.height, false};
        return entry;
    }
}
