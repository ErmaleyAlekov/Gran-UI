#include "gran/render/image_store.hpp"
#include "gran/res/resources.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/image/image.hpp"

#include "gran/utils/file_system.hpp"

namespace gran::render
{
    static ImageEntry upload(rhi::RenderBackend& backend, std::uint32_t slot, const image::Image& picture)
    {
        try
        {
            backend.updateTexture(slot, rhi::TextureRegion{0, 0, picture.width, picture.height},
                picture.pixels.data(), picture.width * 4);
        }
        catch (...)
        {
            backend.releaseTexture(slot);
            throw;
        }
        return ImageEntry{slot, picture.width, picture.height, false};
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
        std::string path = fs::joinPath(_base, source);
        std::vector<std::uint8_t> data;
        ImageEntry entry;
        image::Image picture;

        entry.failed = true;
        // Файл диска или ресурс res:/ (база окна из res:/ даёт путь «res:/…») / A disk file or a res:/ resource (a res:/ window base gives a "res:/…" path)
        if (!res::readFile(path, data))
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
        return upload(_backend, entry.slot, picture);
    }
}
