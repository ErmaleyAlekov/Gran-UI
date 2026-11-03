#ifndef GRAN_RES_INTERNAL_REGISTRY_HPP
#define GRAN_RES_INTERNAL_REGISTRY_HPP

#include "gran/res/resources.hpp"

#include <map>
#include <mutex>
#include <streambuf>

namespace gran::res::internal
{
    /*
    ** Реестр ресурсов процесса: нормализованный путь → байты. Блоки обёрток хранятся здесь же
    ** (владение) и не освобождаются до unmountAll: на их байты ссылаются виды.
    **
    ** The process resource registry: a normalized path → the bytes. The wrapper blocks are kept here too
    ** (ownership) and are not freed until unmountAll: the views refer to their bytes.
    */
    struct Registry
    {
        std::mutex mutex;
        std::map<std::string, ResourceView, std::less<>> entries;
        std::vector<std::unique_ptr<std::vector<unsigned char>>> blobs;
    };

    // ---- registry.cpp ----

    // Единственный реестр / The only registry
    Registry& registry() noexcept;

    /*
    ** Буфер потока над чужими байтами (без копии): чтение и перемотка seekg — всё, что нужно
    ** читателям шрифтов и JSON. Паттерн Adapter: память → std::streambuf.
    **
    ** A stream buffer over foreign bytes (without a copy): reading and the seekg rewinding — all the font
    ** and JSON readers need. The Adapter pattern: memory → std::streambuf.
    */
    class MemoryBuffer : public std::streambuf
    {
    public:
        // ---- streams.cpp ----
        MemoryBuffer(const unsigned char* data, std::size_t size);

    protected:
        pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode mode) override;
        pos_type seekpos(pos_type position, std::ios_base::openmode mode) override;
    };
}

#endif
