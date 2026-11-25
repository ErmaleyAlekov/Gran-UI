#include "gran/render/image_store.hpp"
#include <new>
#include "gran/core/gran_exception.hpp"
#include <string_view>

#include "gran/utils/file_system.hpp"

#include <utility>

namespace gran::render
{
    namespace
    {
        // Сохраняем '..', корни дисков и префиксы ресурсов.
        // Preserve '..', drive roots and resource prefixes.
        std::string_view imageKey(const std::string& source)
        {
            std::size_t begin = 0;
            while (source.compare(begin, 2, "./") == 0)
                begin += 2;
            if (begin == source.size() || source.find_first_of("/\\", begin) == begin ||
                source.find(':', begin) != std::string::npos)
                begin = 0;
            return std::string_view(source).substr(begin);
        }
    }

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
        const auto key = imageKey(source);
        auto found = _entries.lower_bound(key);
        if (found == _entries.end() || found->first != key)
        {
            found = _entries.try_emplace(found, std::string(key));
            try
            {
                found->second = load(found->first);
            }
            catch (...)
            {
                _entries.erase(found);
                throw;
            }
        }
        return found->second;
    }

    // Ошибка запоминается пустой записью, как у картинок: окно продолжает работать / An error is remembered as an empty entry, as for images: the window keeps working
    const vector::Shape* ImageStore::shape(const std::string& source)
    try
    {
        auto found = _shapes.find(source);

        if (found == _shapes.end())
        {
            std::shared_ptr<const vector::Shape> loaded;
            try
            {
                loaded = vector::loadSvgFile(fs::joinPath(_base, source));
            }
            catch (const GranException&)
            {
                loaded.reset();
            }
            found = _shapes.emplace(source, std::move(loaded)).first;
        }
        return found->second.get();
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }
}
