#ifndef GRAN_RENDER_IMAGE_STORE_HPP
#define GRAN_RENDER_IMAGE_STORE_HPP

#include "gran/rhi/render_backend.hpp"

#include <map>
#include <string>

namespace gran::render
{
    // Картинка на GPU: слот текстуры и собственный размер; failed — файл не прочитан
    // An image on the GPU: the texture slot and the intrinsic size; failed — the file was not read
    struct ImageEntry
    {
        std::uint32_t slot = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool failed = false;
    };

    /*
    ** Хранилище картинок окна: файл читается и декодируется при первом обращении, пиксели
    ** уходят на GPU, повторные обращения к тому же источнику — из кэша. Испорченный или
    ** отсутствующий файл запоминается как failed (блок картинки остаётся пустым, без исключения).
    **
    ** The window image store: a file is read and decoded on first access, the pixels go
    ** to the GPU, repeated accesses to the same source come from the cache. A damaged or
    ** missing file is remembered as failed (the image box stays empty, without an exception).
    */
    class ImageStore
    {
    public:
        // ---- image_store.cpp ----
        ImageStore(rhi::RenderBackend& backend, std::string baseDirectory);
        ~ImageStore();
        ImageStore(const ImageStore&) = delete;
        ImageStore& operator=(const ImageStore&) = delete;

        // Картинка по источнику (относительный путь — от каталога разметки) / An image by the source (a relative path is from the markup directory)
        const ImageEntry& get(const std::string& source);

    private:
        // Загрузка и отправка на GPU / Loading and sending to the GPU
        ImageEntry load(const std::string& source);

        rhi::RenderBackend& _backend;
        std::string _base;
        std::map<std::string, ImageEntry> _entries;
    };
}

#endif
