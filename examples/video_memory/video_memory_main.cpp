#include "gran/rhi/backend_factory.hpp"

#include <exception>
#include <iostream>

namespace
{
    // Печатаем имена полей API и значения в байтах.
    // Print API field names and values in bytes.
    void printMemory(const gran::rhi::VideoMemoryInfo& info)
    {
        std::cout << "budgetAvailable=" << std::boolalpha << info.budgetAvailable
                  << " budgetBytes=" << info.budgetBytes << " usageBytes=" << info.usageBytes
                  << " textureBytes=" << info.textureBytes << " bufferBytes=" << info.bufferBytes << '\n';
    }

    // Снимаем показания до и после создания текстуры в бэкенде без окна.
    // Sample memory before and after creating a texture in the offscreen backend.
    void run()
    {
        gran::rhi::BackendOptions options;
        options.width = 64;
        options.height = 64;
        auto backend = gran::rhi::createPlatformBackend(options);

        printMemory(backend->videoMemoryInfo());
        const auto slot = backend->createTexture({256, 256, gran::rhi::TextureFormat::RGBA8, "video_memory"});
        printMemory(backend->videoMemoryInfo());
        backend->releaseTexture(slot);
    }
}

int main()
{
    try
    {
        run();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
